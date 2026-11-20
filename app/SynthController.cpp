#include "SynthController.h"

#include <QFile>
#include <QGuiApplication>
#include <QMetaObject>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>
#include <exception>

#include "midipiano/dsp/SoundFont.h"
#include "midipiano/midi/DemoSequencer.h"

namespace midipiano::app {

namespace {
constexpr int kFollowRoot = 36;  // C2: in key-follow-modus speelt deze toets het patroon ongewijzigd
std::uint8_t toMidiByte(int value) { return static_cast<std::uint8_t>(std::clamp(value, 0, 127)); }
}  // namespace

SynthController::SynthController(QObject* parent)
    : QObject(parent),
      source_(std::make_unique<midi::DemoSequencer>()),
      stepSequencer_(std::make_unique<midi::StepSequencer>()),
      bluetooth_(std::make_unique<BleMidiSource>()),
      tick_(new QTimer(this)) {
  // new QTimer(this): de controller is de Qt-parent, dus Qt verwijdert de timer automatisch samen
  // met de controller ("kill children"), zonder delete.
  QSettings settings;
  bufferMs_ = settings.value(QStringLiteral("audio/bufferMs"), 0).toInt();
  steadyTiming_ = settings.value(QStringLiteral("audio/steadyTiming"), true).toBool();
  openAudio();

  loadSoundFont();

  try {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/presets";
    store_ = std::make_unique<preset::PresetStore>(std::filesystem::path(dir.toStdU16String()));
    store_->installFactoryPresets();
    refreshPresets();
  } catch (const std::exception& e) {
    setStatus(QStringLiteral("Presets unavailable: %1").arg(QString::fromUtf8(e.what())));
  }

  // Vereiste 52: 52.useful usage of signals/slots
  // Elke 33 ms zendt de QTimer het signal timeout(); dat is verbonden met de slot onTick(), die de
  // oscilloscoop en de stemteller bijwerkt en zelf weer signals uitzendt (scopeUpdated,
  // activeVoicesChanged) waar WaveformView en de QML-labels op reageren. Zo weet de timer niets
  // van de scope, en de scope niets van de timer.
  connect(tick_, &QTimer::timeout, this, &SynthController::onTick);

  // App naar de achtergrond (home-knop, andere app): alles stilzetten en de geluidskaart vrijgeven.
  // Op de desktop gebeurt dat alleen bij minimaliseren; een ander venster aanklikken laat de klank aan.
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &SynthController::onApplicationStateChanged);

  // Bluetooth-noten volgen dezelfde weg als die van de demo-sequencer (polymorfie via MidiSource).
  bluetooth_->start([this](const midi::MidiMessage& message) { onLiveMessage(message); });
  updateTiming();
  tick_->start(33);
}

SynthController::~SynthController() {
  stepSequencer_->stop();
  if (source_) source_->stop();  // eerst de sequencer-thread stoppen, dan pas de synth afbreken
}

QVariantList SynthController::steps() const {
  QVariantList list;
  for (const auto& step : stepSequencer_->pattern()) {
    list.append(QVariantMap{{"active", step.active}, {"note", int(step.note)}, {"accent", step.accent}});
  }
  return list;
}

void SynthController::toggleStepSequencer() {
  if (stepSequencer_->isRunning()) {
    stepSequencer_->stop();
  } else {
    heldKeys_.clear();
    stepSequencer_->setTranspose(0);
    startStepSequencer();
  }
  emit stepSequencerRunningChanged();
}

void SynthController::startStepSequencer() {
  synth_.setTempo(stepSequencer_->bpm());
  updateTiming();
  stepSequencer_->start([this](const midi::MidiMessage& message) { onSourceMessage(message); });
}

// Key follow: zoals een arpeggiator of de "key trigger" van een groovebox. Elke nieuwe toets (gate)
// start het patroon opnieuw vanaf stap 1, getransponeerd: C2 speelt het zoals het is, D2 een toon
// hoger. Loslaten van de laatste toets stopt het; bij nog ingedrukte toetsen volgt het die zonder
// herstart.
void SynthController::setKeyFollow(bool on) {
  if (on == keyFollow_) return;
  keyFollow_ = on;
  heldKeys_.clear();
  if (stepSequencer_->isRunning()) {
    stepSequencer_->stop();
    emit stepSequencerRunningChanged();
  }
  stepSequencer_->setTranspose(0);
  emit keyFollowChanged();
}

void SynthController::followNoteOn(int note) {
  heldKeys_.erase(std::remove(heldKeys_.begin(), heldKeys_.end(), note), heldKeys_.end());
  heldKeys_.push_back(note);
  stepSequencer_->stop();
  stepSequencer_->setTranspose(note - kFollowRoot);
  startStepSequencer();
  emit stepSequencerRunningChanged();
}

void SynthController::followNoteOff(int note) {
  const auto it = std::find(heldKeys_.begin(), heldKeys_.end(), note);
  if (it == heldKeys_.end()) return;
  heldKeys_.erase(it);
  if (heldKeys_.empty()) {
    stepSequencer_->stop();
    emit stepSequencerRunningChanged();
  } else {
    stepSequencer_->setTranspose(heldKeys_.back() - kFollowRoot);
  }
}

void SynthController::onLiveMessage(const midi::MidiMessage& message) {
  // Noten van Bluetooth: in key-follow-modus sturen ze de sequencer, anders spelen ze gewoon.
  if (keyFollow_ && message.isNoteOn()) return followNoteOn(message.note());
  if (keyFollow_ && message.isNoteOff()) return followNoteOff(message.note());
  onSourceMessage(message);
}

void SynthController::setStep(int index, bool active, int note, bool accent) {
  if (index < 0 || index >= static_cast<int>(midi::StepSequencer::kSteps)) return;
  stepSequencer_->setStep(static_cast<std::size_t>(index), {active, toMidiByte(note), accent});
  emit stepsChanged();
}

void SynthController::clearSteps() {
  stepSequencer_->clear();
  emit stepsChanged();
}

void SynthController::resetSteps() {
  stepSequencer_->setPattern(midi::StepSequencer::defaultPattern());
  emit stepsChanged();
}

void SynthController::onApplicationStateChanged(Qt::ApplicationState state) {
  if (state == Qt::ApplicationSuspended || state == Qt::ApplicationHidden) {
    if (stepSequencer_->isRunning()) {
      stepSequencer_->stop();
      emit stepSequencerRunningChanged();
    }
    if (source_ && source_->isRunning()) {
      source_->stop();
      emit sequencerRunningChanged();
    }
    heldKeys_.clear();
    setSustain(false);
    synth_.allNotesOff();
    if (audio_) audio_->pause();
  } else if (state == Qt::ApplicationActive) {
    // Na slaapstand is de AAudio-stream op Android vaak dood: opnieuw starten lukt dan "zonder
    // fout" maar er komt geen geluid. Daarom openen we de geluidskaart gewoon helemaal opnieuw.
    if (!audio_ || audio_->isPaused()) reopenAudio();
  }
}

void SynthController::openAudio() {
  audio_.reset();  // eerst de oude stream sluiten, dan pas een nieuwe openen
  try {
    audio_ = std::make_unique<audio::AudioOutput>(synth_, static_cast<unsigned>(bufferMs_));
  } catch (const audio::AudioError& e) {
    setStatus(QStringLiteral("No sound: %1").arg(QString::fromUtf8(e.what())));
  }
}

void SynthController::reopenAudio() {
  synth_.allNotesOff();
  openAudio();
  failedReopens_ = audio_ ? 0 : failedReopens_ + 1;  // zonder geluidskaart niet eindeloos opnieuw proberen
  updateTiming();
  lastReopen_ = std::chrono::steady_clock::now();
  emit audioChanged();
}

void SynthController::updateTiming() {
  // Noten worden een vaste tijd vooruit ingepland: een audioblok plus wat speling. Zo valt elke
  // noot op zijn exacte sample, ongeacht wanneer het audioblok toevallig berekend wordt.
  const double block = audio_ ? audio_->blockMilliseconds() : 10.0;
  lookahead_ = std::chrono::milliseconds(static_cast<int>(block) + 5);
  auto timed = [this](const midi::MidiMessage& message, dsp::Synth::Clock::time_point when) {
    onTimedMessage(message, when);
  };
  if (!stepSequencer_->isRunning()) stepSequencer_->setTimedOutput(timed, lookahead_);
  // Bluetooth: de tijdstempels van de PSoC houden de afstand tussen noten gelijk; de vaste
  // vertraging (blok + 20 ms) vangt de wisselende radiovertraging op. Uit = meteen spelen.
  if (steadyTiming_) {
    const auto delay = std::chrono::milliseconds(static_cast<int>(block) + 20);
    bluetooth_->setTimedOutput([this, delay](const midi::MidiMessage& message, dsp::Synth::Clock::time_point when) {
      if (keyFollow_ && (message.isNoteOn() || message.isNoteOff())) return onLiveMessage(message);
      onTimedMessage(message, when + delay);
    });
  } else {
    bluetooth_->setTimedOutput(nullptr);
  }
}

void SynthController::setBufferMs(int ms) {
  ms = (ms == 5 || ms == 10 || ms == 20) ? ms : 0;
  if (ms == bufferMs_) return;
  bufferMs_ = ms;
  QSettings().setValue(QStringLiteral("audio/bufferMs"), ms);
  synth_.allNotesOff();
  openAudio();
  updateTiming();
  emit audioChanged();
}

void SynthController::setSteadyTiming(bool on) {
  if (on == steadyTiming_) return;
  steadyTiming_ = on;
  QSettings().setValue(QStringLiteral("audio/steadyTiming"), on);
  updateTiming();
  emit audioChanged();
}

QString SynthController::audioInfo() const {
  if (!audio_) return tr("No sound card");
  const double rate = audio_->sampleRate();
  const unsigned int frames = audio_->bufferFrames();
  return tr("%1 Hz, buffer %2 samples (%3 ms)").arg(rate, 0, 'f', 0).arg(frames).arg(1000.0 * frames / rate, 0, 'f', 1);
}

void SynthController::loadSoundFont() {
  // De pianosamples zitten als Qt-resource in de app (ook in de APK), dus we lezen ze met QFile en
  // geven de bytes aan de Qt-vrije core.
  QFile file(QStringLiteral(":/sounds/instruments.sf2"));
  try {
    if (!file.open(QIODevice::ReadOnly)) throw dsp::SoundFontError("instruments.sf2 ontbreekt in de app");
    const QByteArray bytes = file.readAll();
    const auto font = dsp::SoundFont::fromMemory(std::vector<char>(bytes.begin(), bytes.end()));
    for (int i = 0; i < font->presetCount(); ++i) {
      synth_.addInstrument(std::make_unique<dsp::SoundFontInstrument>(font, i, synth_.sampleRate()));
    }
    if (font->presetCount() > 0) synth_.selectInstrument(1);  // 0 = Synth, 1 = Grand Piano
  } catch (const dsp::SoundFontError& e) {
    setStatus(QStringLiteral("Piano samples unavailable: %1").arg(QString::fromUtf8(e.what())));
  }
  for (const std::string& name : synth_.instrumentNames()) instrumentNames_ << QString::fromStdString(name);
}

void SynthController::setInstrument(int index) {
  if (index < 0 || index >= instrumentNames_.size() || index == instrument()) return;
  synth_.selectInstrument(static_cast<std::size_t>(index));
  emit instrumentChanged();
}

void SynthController::noteOn(int note, int velocity) {
  if (keyFollow_) return followNoteOn(std::clamp(note, 0, 127));
  synth_.noteOn(toMidiByte(note), toMidiByte(velocity));
}

void SynthController::noteOff(int note) {
  if (keyFollow_) return followNoteOff(std::clamp(note, 0, 127));
  synth_.noteOff(toMidiByte(note));
}

void SynthController::allNotesOff() { synth_.allNotesOff(); }

void SynthController::setSustain(bool down) {
  if (down == synth_.sustain()) return;
  synth_.setSustain(down);
  emit sustainChanged();
}

double SynthController::param(const QString& name) const {
  // "seq_"-namen horen bij de step sequencer, niet bij de klank (zo werken dezelfde draaiknoppen).
  if (name == QLatin1String("seq_bpm")) return stepSequencer_->bpm();
  if (name == QLatin1String("seq_gate")) return stepSequencer_->gate();
  if (name == QLatin1String("seq_swing")) return stepSequencer_->swing();
  try {
    return synth_.patch().get(name.toStdString());
  } catch (const std::out_of_range&) {
    return 0.0;
  }
}

void SynthController::setParam(const QString& name, double value) {
  if (name.startsWith(QLatin1String("seq_"))) {
    const auto v = static_cast<float>(value);
    if (name == QLatin1String("seq_bpm")) {
      stepSequencer_->setBpm(v);
      synth_.setTempo(stepSequencer_->bpm());  // de echo volgt het tempo
    } else if (name == QLatin1String("seq_gate")) {
      stepSequencer_->setGate(v);
    } else if (name == QLatin1String("seq_swing")) {
      stepSequencer_->setSwing(v);
    }
    emit patchChanged();
    return;
  }
  preset::Patch patch = synth_.patch();
  try {
    patch.set(name.toStdString(), static_cast<float>(value));
  } catch (const std::out_of_range& e) {
    setStatus(QString::fromUtf8(e.what()));
    return;
  }
  synth_.applyPatch(patch);
  emit patchChanged();
}

int SynthController::waveform() const { return static_cast<int>(synth_.patch().waveform()); }

void SynthController::setWaveform(int waveform) {
  if (waveform < 0 || waveform >= static_cast<int>(dsp::Waveform::Count)) return;
  preset::Patch patch = synth_.patch();
  patch.setWaveform(static_cast<dsp::Waveform>(waveform));
  synth_.applyPatch(patch);
  emit patchChanged();
}

QStringList SynthController::waveformNames() const {
  QStringList names;
  for (int i = 0; i < static_cast<int>(dsp::Waveform::Count); ++i) {
    names << QString::fromStdString(dsp::waveformName(static_cast<dsp::Waveform>(i)));
  }
  return names;
}

bool SynthController::sequencerRunning() const { return source_ && source_->isRunning(); }

void SynthController::toggleSequencer() {
  if (!source_) return;
  if (source_->isRunning()) {
    source_->stop();
    synth_.allNotesOff();
  } else {
    // Vereiste 49: 49.useful usage of a lambda function
    // De MIDI-bron vraagt een functie die hij per bericht oproept. Een lambda die "this" vangt is
    // hier het kortst en duidelijkst: hij stuurt elk bericht door naar onSourceMessage() van deze
    // controller, zonder dat de sequencer iets van Qt of van SynthController hoeft te weten.
    source_->start([this](const midi::MidiMessage& message) { onSourceMessage(message); });
  }
  emit sequencerRunningChanged();
}

void SynthController::onSourceMessage(const midi::MidiMessage& message) {
  // Draait op de sequencer-thread (of de GUI-thread bij Bluetooth): de synth is thread-safe, de GUI niet. Daarom gaat het signal
  // via de Qt-eventlus (QueuedConnection) naar de GUI-thread.
  synth_.handle(message);
  const int note = message.note();
  const bool on = message.isNoteOn();
  if (message.isNoteOn() || message.isNoteOff()) {
    QMetaObject::invokeMethod(this, [this, note, on] { emit noteStateChanged(note, on); }, Qt::QueuedConnection);
  }
}

void SynthController::onTimedMessage(const midi::MidiMessage& message, dsp::Synth::Clock::time_point when) {
  // De synth zet de noot op het exacte sample van "when" (zie Synth::schedule).
  synth_.schedule(message, when);
  if (message.isNoteOn() || message.isNoteOff()) {
    const int note = message.note();
    const bool on = message.isNoteOn();
    QMetaObject::invokeMethod(this, [this, note, on] { emit noteStateChanged(note, on); }, Qt::QueuedConnection);
  }
}

void SynthController::savePreset(const QString& name) {
  if (!store_) return;
  const QString trimmed = name.trimmed();
  if (trimmed.isEmpty()) {
    setStatus("Give the preset a name first");
    return;
  }
  preset::Patch patch = synth_.patch();
  patch.setName(trimmed.toStdString());
  try {
    store_->save(patch);
    synth_.applyPatch(patch);
    patchName_ = trimmed;
    refreshPresets();
    setStatus(QStringLiteral("Saved \"%1\"").arg(trimmed));
    emit patchChanged();
  } catch (const preset::PresetError& e) {
    setStatus(QString::fromUtf8(e.what()));
  }
}

void SynthController::loadPreset(const QString& name) {
  if (!store_) return;
  try {
    synth_.applyPatch(store_->load(name.toStdString()));
    patchName_ = name;
    setStatus(QStringLiteral("Loaded \"%1\"").arg(name));
    emit patchChanged();
  } catch (const preset::PresetError& e) {
    setStatus(QString::fromUtf8(e.what()));  // een kapot bestand geeft een melding, geen crash
  }
}

void SynthController::refreshPresets() {
  presets_.clear();
  // Vereiste 38: 38.at least 4 useful const references for variables (4/4)
  // name verwijst naar elke naam in de lijst die list() teruggaf: geen kopie van elke string, en
  // const omdat we hem alleen omzetten naar een QString.
  for (const std::string& name : store_->list()) presets_ << QString::fromStdString(name);
  emit presetsChanged();
}

void SynthController::onTick() {
  // Waakhond: vraagt de geluidskaart al 1,5 s geen samples meer (stream weggevallen door slaapstand,
  // koptelefoon uitgetrokken, ...) terwijl de app actief is, dan openen we hem opnieuw. Hoogstens
  // om de 3 s, zodat een kapotte geluidskaart de app niet blijft herstarten.
  if (qGuiApp->applicationState() == Qt::ApplicationActive &&
      std::chrono::steady_clock::now() - lastReopen_ > std::chrono::seconds(3) &&
      ((audio_ && audio_->isStalled(std::chrono::milliseconds(1500))) || (!audio_ && failedReopens_ < 5))) {
    reopenAudio();
  }

  const int voices = static_cast<int>(synth_.activeVoiceCount());
  if (voices != activeVoices_) {
    activeVoices_ = voices;
    emit activeVoicesChanged();
  }
  const int step = stepSequencer_->currentStep();
  if (step != currentStep_) {
    currentStep_ = step;
    emit currentStepChanged();
  }
  emit scopeUpdated();
}

void SynthController::setStatus(const QString& text) {
  statusText_ = text;
  emit statusTextChanged();
}

}  // namespace midipiano::app

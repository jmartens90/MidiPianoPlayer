#include "SynthController.h"

#include <QFile>
#include <QMetaObject>
#include <QStandardPaths>

#include <algorithm>
#include <exception>

#include "midipiano/dsp/SoundFont.h"
#include "midipiano/midi/DemoSequencer.h"

namespace midipiano::app {

namespace {
std::uint8_t toMidiByte(int value) { return static_cast<std::uint8_t>(std::clamp(value, 0, 127)); }
}  // namespace

SynthController::SynthController(QObject* parent)
    : QObject(parent), source_(std::make_unique<midi::DemoSequencer>()), tick_(new QTimer(this)) {
  // new QTimer(this): de controller is de Qt-parent, dus Qt verwijdert de timer automatisch samen
  // met de controller ("kill children"), zonder delete.
  try {
    audio_ = std::make_unique<audio::AudioOutput>(synth_);
  } catch (const audio::AudioError& e) {
    setStatus(QStringLiteral("No sound: %1").arg(QString::fromUtf8(e.what())));
  }

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
  tick_->start(33);
}

SynthController::~SynthController() {
  if (source_) source_->stop();  // eerst de sequencer-thread stoppen, dan pas de synth afbreken
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

void SynthController::noteOn(int note, int velocity) { synth_.noteOn(toMidiByte(note), toMidiByte(velocity)); }

void SynthController::noteOff(int note) { synth_.noteOff(toMidiByte(note)); }

void SynthController::allNotesOff() { synth_.allNotesOff(); }

void SynthController::setSustain(bool down) {
  if (down == synth_.sustain()) return;
  synth_.setSustain(down);
  emit sustainChanged();
}

double SynthController::param(const QString& name) const {
  try {
    return synth_.patch().get(name.toStdString());
  } catch (const std::out_of_range&) {
    return 0.0;
  }
}

void SynthController::setParam(const QString& name, double value) {
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
  // Draait op de sequencer-thread: de synth is thread-safe, de GUI niet. Daarom gaat het signal
  // via de Qt-eventlus (QueuedConnection) naar de GUI-thread.
  synth_.handle(message);
  const int note = message.note();
  const bool on = message.isNoteOn();
  if (message.isNoteOn() || message.isNoteOff()) {
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
  const int voices = static_cast<int>(synth_.activeVoiceCount());
  if (voices != activeVoices_) {
    activeVoices_ = voices;
    emit activeVoicesChanged();
  }
  emit scopeUpdated();
}

void SynthController::setStatus(const QString& text) {
  statusText_ = text;
  emit statusTextChanged();
}

}  // namespace midipiano::app

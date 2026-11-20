#include "midipiano/dsp/Synth.h"

#include <algorithm>
#include <cmath>
#include <functional>

#include "midipiano/dsp/VoiceSynth.h"

namespace midipiano::dsp {

Synth::Synth(float sampleRate) : sampleRate_(sampleRate), delay_(sampleRate), reverb_(sampleRate) {
  instruments_.push_back(std::make_unique<VoiceSynth>(sampleRate));  // instrument 0: de synth
  scratch_.resize(4096);
  applyReverbSettings();
  applyDelaySettings();
}

void Synth::handle(const midi::MidiMessage& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  handleLocked(message);
}

void Synth::handleLocked(const midi::MidiMessage& message) {
  if (message.isNoteOn()) {
    noteOnLocked(message.note(), message.velocity());
  } else if (message.isNoteOff()) {
    noteOffLocked(message.note());
  } else if (message.type() == 0xB0 && message.note() == 64) {  // CC 64 = sustainpedaal
    setSustainLocked(message.velocity() >= 64);
  } else if (message.type() == 0xB0 && message.note() == 123) {  // CC 123 = All Notes Off
    allNotesOffLocked();
  }
}

void Synth::schedule(const midi::MidiMessage& message, Clock::time_point when) {
  std::lock_guard<std::mutex> lock(mutex_);
  scheduled_.push({when, scheduleCounter_++, message});
}

void Synth::noteOn(std::uint8_t note, std::uint8_t velocity) {
  std::lock_guard<std::mutex> lock(mutex_);
  noteOnLocked(note, velocity);
}

void Synth::noteOnLocked(std::uint8_t note, std::uint8_t velocity) {
  if (note < sustained_.size()) sustained_.reset(note);  // opnieuw aangeslagen: toets is weer ingedrukt
  instruments_[current_]->noteOn(note, velocity);
}

void Synth::noteOff(std::uint8_t note) {
  std::lock_guard<std::mutex> lock(mutex_);
  noteOffLocked(note);
}

void Synth::noteOffLocked(std::uint8_t note) {
  if (sustain_ && note < sustained_.size()) {
    sustained_.set(note);  // pedaal ingedrukt: de noot blijft klinken tot het pedaal los komt
    return;
  }
  // Aan alle instrumenten: een noot die nog op het vorige instrument werd gestart, moet ook stoppen.
  for (const auto& instrument : instruments_) instrument->noteOff(note);
}

void Synth::allNotesOff() {
  std::lock_guard<std::mutex> lock(mutex_);
  allNotesOffLocked();
}

void Synth::allNotesOffLocked() {
  sustained_.reset();
  while (!scheduled_.empty()) scheduled_.pop();  // geplande noten vervallen ook
  for (const auto& instrument : instruments_) instrument->allNotesOff();
}

void Synth::setSustain(bool down) {
  std::lock_guard<std::mutex> lock(mutex_);
  setSustainLocked(down);
}

void Synth::setSustainLocked(bool down) {
  sustain_ = down;
  if (down) return;
  for (std::size_t note = 0; note < sustained_.size(); ++note) {
    if (!sustained_.test(note)) continue;
    for (const auto& instrument : instruments_) instrument->noteOff(static_cast<std::uint8_t>(note));
  }
  sustained_.reset();
}

bool Synth::sustain() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return sustain_;
}

void Synth::renderInstruments(float* out, std::size_t frames) {
  // Vereiste 5: 5.useful and correct polymorphism (explain why)
  // instruments_ bevat zowel de VoiceSynth als SoundFontInstruments (piano, orgel, ...), allemaal
  // via een Instrument-pointer. render() roept bij elk dezelfde virtuele functie op en C++ kiest
  // tijdens het draaien de juiste: golfvormen berekenen of samples afspelen. Zo klinkt een losgelaten
  // pianonoot ook nog uit als je intussen naar de synth overschakelt.
  for (const auto& instrument : instruments_) {
    if (instrument->activeVoiceCount() == 0) continue;
    instrument->render(scratch_.data(), frames);
    for (std::size_t i = 0; i < frames; ++i) out[i] += scratch_[i];
  }
}

void Synth::render(float* out, std::size_t frames) {
  render(out, frames, Clock::now());
}

void Synth::render(float* out, std::size_t frames, Clock::time_point blockStart) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (scratch_.size() < frames) scratch_.resize(frames);  // gebeurt hooguit een keer
  std::fill(out, out + frames, 0.0f);

  // Geplande noten (Bluetooth met tijdstempels, step sequencer) starten op het juiste sample binnen
  // dit blok in plaats van aan het begin van het volgende blok. Zonder dit schuift elke noot 0 tot
  // 10 ms (een blok) op, en dat hoor je als een onregelmatig ritme. We knippen het blok daarom op
  // bij elk gepland bericht: eerst renderen tot dat sample, dan het bericht toepassen, verder renderen.
  const double samplesPerSecond = sampleRate_;
  auto sampleIndexOf = [&](Clock::time_point when) -> std::size_t {
    const double seconds = std::chrono::duration<double>(when - blockStart).count();
    if (seconds <= 0.0) return 0;
    return static_cast<std::size_t>(std::min<double>(seconds * samplesPerSecond, static_cast<double>(frames)));
  };
  std::size_t position = 0;
  while (position < frames) {
    while (!scheduled_.empty() && sampleIndexOf(scheduled_.top().when) <= position) {
      handleLocked(scheduled_.top().message);
      scheduled_.pop();
    }
    const std::size_t next = scheduled_.empty() ? frames : std::max(position + 1, sampleIndexOf(scheduled_.top().when));
    renderInstruments(out + position, next - position);
    position = next;
  }

  delay_.process(out, frames);    // echo voor de galm: de echo's krijgen ook galm
  reverb_.process(out, frames);  // ook als er geen noten meer zijn: dan sterft de galm uit
  const float volume = patch_.get("volume");
  for (std::size_t i = 0; i < frames; ++i) {
    out[i] = std::tanh(out[i] * volume);  // zachte begrenzing: een vol akkoord clipt niet
    scope_.push(out[i]);
  }
}

// Vereiste 43: 43.2 useful (modern) call-by-references
// render() vult de vector van de aanroeper rechtstreeks (std::vector<float>&): geen kopie van een
// buffer van duizenden samples bij elke aanroep. Tweede voorbeeld: scopeSnapshot(out) hieronder.
void Synth::render(std::vector<float>& buffer) { render(buffer.data(), buffer.size()); }

void Synth::scopeSnapshot(std::vector<float>& out) const {
  std::lock_guard<std::mutex> lock(mutex_);
  scope_.copyTo(out);
}

void Synth::setSampleRate(float sampleRate) {
  std::lock_guard<std::mutex> lock(mutex_);
  sampleRate_ = sampleRate;
  delay_.setSampleRate(sampleRate);
  reverb_.setSampleRate(sampleRate);
  for (const auto& instrument : instruments_) instrument->setSampleRate(sampleRate);
}

float Synth::sampleRate() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return sampleRate_;
}

std::size_t Synth::addInstrument(std::unique_ptr<Instrument> instrument) {
  std::lock_guard<std::mutex> lock(mutex_);
  instrument->setSampleRate(sampleRate_);
  instrument->applyPatch(patch_);
  instruments_.push_back(std::move(instrument));
  return instruments_.size() - 1;
}

void Synth::selectInstrument(std::size_t index) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (index >= instruments_.size() || index == current_) return;
  // Wat op het oude instrument nog vastgehouden wordt, loslaten (het klinkt nog natuurlijk uit). Zo
  // kan een noot waarvan de noteOff verloren ging nooit blijven hangen na het wisselen.
  instruments_[current_]->allNotesOff();
  sustained_.reset();
  current_ = index;
}

std::size_t Synth::currentInstrument() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return current_;
}

std::vector<std::string> Synth::instrumentNames() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> names;
  for (const auto& instrument : instruments_) names.push_back(instrument->name());
  return names;
}

bool Synth::currentUsesPatch() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return instruments_[current_]->usesPatch();
}

void Synth::applyPatch(const preset::Patch& patch) {
  std::lock_guard<std::mutex> lock(mutex_);
  patch_ = patch;
  applyReverbSettings();
  applyDelaySettings();
  // Vereiste 38: 38.at least 4 useful const references for variables (1/4)
  // instrument is een const-referentie naar elke unique_ptr in de vector: een unique_ptr kan je niet
  // kopieren, en const garandeert dat we de lijst zelf hier niet wijzigen.
  for (const auto& instrument : instruments_) instrument->applyPatch(patch_);
}

void Synth::applyReverbSettings() {
  dsp::Reverb::Settings settings;
  settings.mix = patch_.get("reverb");
  settings.predelay = patch_.get("reverb_predelay");
  settings.decay = patch_.get("reverb_decay");
  settings.damping = patch_.get("reverb_damp");
  settings.lowCutHz = patch_.get("reverb_lowcut");
  settings.highCutHz = patch_.get("reverb_highcut");
  reverb_.setSettings(settings);
}

void Synth::setTempo(float bpm) {
  std::lock_guard<std::mutex> lock(mutex_);
  tempo_ = std::max(bpm, 1.0f);
  applyDelaySettings();
}

void Synth::applyDelaySettings() {
  dsp::Delay::Settings settings;
  settings.mix = patch_.get("delay_mix");
  // Gesynchroniseerd: tijd = aantal tellen x lengte van een tel (60 / BPM). 0,75 tel = gepunteerde achtste.
  settings.time = patch_.get("delay_sync") >= 0.5f ? patch_.get("delay_beats") * 60.0f / tempo_
                                                   : patch_.get("delay_time");
  settings.feedback = patch_.get("delay_feedback");
  settings.toneHz = patch_.get("delay_tone");
  delay_.setSettings(settings);
}

preset::Patch Synth::patch() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return patch_;
}

std::size_t Synth::activeVoiceCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::size_t count = 0;
  for (const auto& instrument : instruments_) count += instrument->activeVoiceCount();
  return count;
}

}  // namespace midipiano::dsp

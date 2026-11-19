#include "midipiano/dsp/Synth.h"

#include <algorithm>
#include <cmath>

#include "midipiano/dsp/VoiceSynth.h"

namespace midipiano::dsp {

Synth::Synth(float sampleRate) : sampleRate_(sampleRate) {
  instruments_.push_back(std::make_unique<VoiceSynth>(sampleRate));  // instrument 0: de synth
  scratch_.resize(4096);
}

void Synth::handle(const midi::MidiMessage& message) {
  if (message.isNoteOn()) {
    noteOn(message.note(), message.velocity());
  } else if (message.isNoteOff()) {
    noteOff(message.note());
  } else if (message.type() == 0xB0 && message.note() == 123) {  // CC 123 = All Notes Off
    allNotesOff();
  }
}

void Synth::noteOn(std::uint8_t note, std::uint8_t velocity) {
  std::lock_guard<std::mutex> lock(mutex_);
  instruments_[current_]->noteOn(note, velocity);
}

void Synth::noteOff(std::uint8_t note) {
  std::lock_guard<std::mutex> lock(mutex_);
  // Aan alle instrumenten: een noot die nog op het vorige instrument werd gestart, moet ook stoppen.
  for (const auto& instrument : instruments_) instrument->noteOff(note);
}

void Synth::allNotesOff() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& instrument : instruments_) instrument->allNotesOff();
}

void Synth::render(float* out, std::size_t frames) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (scratch_.size() < frames) scratch_.resize(frames);  // gebeurt hooguit een keer
  std::fill(out, out + frames, 0.0f);
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
  if (index < instruments_.size()) current_ = index;
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
  // Vereiste 38: 38.at least 4 useful const references for variables (1/4)
  // instrument is een const-referentie naar elke unique_ptr in de vector: een unique_ptr kan je niet
  // kopieren, en const garandeert dat we de lijst zelf hier niet wijzigen.
  for (const auto& instrument : instruments_) instrument->applyPatch(patch_);
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

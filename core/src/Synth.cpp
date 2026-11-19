#include "midipiano/dsp/Synth.h"

#include <algorithm>
#include <cmath>

namespace midipiano::dsp {

Synth::Synth(float sampleRate) : sampleRate_(sampleRate) {
  for (Voice& voice : voices_) applyPatchToVoice(voice);
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

// Vereiste 29: 29.useful member function
// noteOn() is het hart van de polyfonie: het zoekt een vrije stem, en als alle 8 bezet zijn, neemt
// het de oudste (voice stealing). Zo kan je akkoorden spelen en blijft een nieuwe noot altijd hoorbaar.
void Synth::noteOn(std::uint8_t note, std::uint8_t velocity) {
  std::lock_guard<std::mutex> lock(mutex_);
  Voice* voice = findFreeVoice();
  if (voice == nullptr) voice = &oldestVoice();
  voice->start(note, velocity, ++clock_);
}

void Synth::noteOff(std::uint8_t note) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (Voice& voice : voices_) {
    if (voice.isHeld() && voice.note() == note) voice.release();
  }
}

void Synth::allNotesOff() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (Voice& voice : voices_) voice.release();
}

void Synth::render(float* out, std::size_t frames) {
  std::lock_guard<std::mutex> lock(mutex_);
  const float volume = patch_.get("volume");
  for (std::size_t i = 0; i < frames; ++i) {
    float mix = 0.0f;
    for (Voice& voice : voices_) mix += voice.render(sampleRate_);
    // Zachte begrenzing zodat een akkoord van 8 noten niet clipt.
    const float sample = std::tanh(mix * volume * 0.6f);
    out[i] = sample;
    scope_.push(sample);
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
}

float Synth::sampleRate() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return sampleRate_;
}

void Synth::applyPatch(const preset::Patch& patch) {
  std::lock_guard<std::mutex> lock(mutex_);
  patch_ = patch;
  for (Voice& voice : voices_) applyPatchToVoice(voice);
}

preset::Patch Synth::patch() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return patch_;
}

std::size_t Synth::activeVoiceCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<std::size_t>(
      std::count_if(voices_.begin(), voices_.end(), [](const Voice& v) { return v.isActive(); }));
}

// Vereiste 46: 46.useful usage of nullptr
// findFreeVoice() geeft nullptr terug als alle 8 stemmen bezig zijn. noteOn() test daarop en kiest
// dan de oudste stem. Zo betekent nullptr duidelijk "geen vrije stem" in plaats van een magisch getal.
Voice* Synth::findFreeVoice() {
  for (Voice& voice : voices_) {
    if (!voice.isActive()) return &voice;
  }
  return nullptr;
}

Voice& Synth::oldestVoice() {
  return *std::min_element(voices_.begin(), voices_.end(), [](const Voice& a, const Voice& b) {
    return a.startedAt() < b.startedAt();
  });
}

void Synth::applyPatchToVoice(Voice& voice) {
  // Vereiste 38: 38.at least 4 useful const references for variables (1/4)
  // p is een const-referentie naar patch_: kortere code zonder de hele Patch (met zijn std::map) te
  // kopieren, en const garandeert dat we de patch hier niet per ongeluk wijzigen.
  const preset::Patch& p = patch_;
  voice.setWaveform(p.waveform());
  voice.envelope().setAttack(p.get("attack"));
  voice.envelope().setDecay(p.get("decay"));
  voice.envelope().setSustain(p.get("sustain"));
  voice.envelope().setRelease(p.get("release"));
  voice.filter().setCutoff(p.get("cutoff"));
}

}  // namespace midipiano::dsp

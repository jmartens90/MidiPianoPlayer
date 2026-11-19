#include "Voice.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace midipiano {

Voice::Voice(std::unique_ptr<Oscillator> oscillator) : oscillator_(std::move(oscillator)) {}

void Voice::noteOn(int midiNote) {
  // MIDI-noot 69 is A4 = 440 Hz; elke noot hoger is een halve toon (factor 2^(1/12)).
  frequencyHz_ = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
  oscillator_->reset();
  active_ = true;
}

void Voice::noteOff() { active_ = false; }

float Voice::render(float sampleRate) {
  if (!active_) return 0.0f;
  return volume_ * oscillator_->next(frequencyHz_, sampleRate);
}

void Voice::setVolume(float volume) { volume_ = std::clamp(volume, 0.0f, 1.0f); }

}  // namespace midipiano

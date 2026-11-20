#include "midipiano/dsp/Modulation.h"

#include <cmath>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

std::vector<std::string> modTargetNames() {
  return {"Off", "Pitch", "Cutoff", "Resonance", "Position", "Drive", "Volume"};
}

std::vector<std::string> Lfo::shapeNames() { return {"Sine", "Triangle", "Saw", "Square", "Random"}; }

float Lfo::advance(float seconds) {
  phase_ += rateHz_ * seconds;
  if (phase_ >= 1.0f) {
    phase_ -= std::floor(phase_);
    // Nieuwe toevalswaarde aan het begin van elke periode (eenvoudige LCG, geen rand() in de audio-thread).
    seed_ = seed_ * 1664525u + 1013904223u;
    held_ = static_cast<float>(seed_ >> 8) / static_cast<float>(1u << 24) * 2.0f - 1.0f;
  }
  switch (shape_) {
    case Shape::Sine: value_ = std::sin(2.0f * util::kPi * phase_); break;
    case Shape::Triangle: value_ = phase_ < 0.5f ? 4.0f * phase_ - 1.0f : 3.0f - 4.0f * phase_; break;
    case Shape::Saw: value_ = 1.0f - 2.0f * phase_; break;  // dalend: zoals een "pluk" op de cutoff
    case Shape::Square: value_ = phase_ < 0.5f ? 1.0f : -1.0f; break;
    case Shape::Random: value_ = held_; break;
    default: value_ = 0.0f;
  }
  return value_;
}

}  // namespace midipiano::dsp

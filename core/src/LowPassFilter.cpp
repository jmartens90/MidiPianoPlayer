#include "midipiano/dsp/LowPassFilter.h"

#include <algorithm>
#include <cmath>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

float LowPassFilter::process(float input, float sampleRate) {
  // Een-polige filter: y += a * (x - y), met a afhankelijk van cutoff en samplerate.
  const float cutoff = std::clamp(cutoffHz_, 20.0f, sampleRate * 0.45f);
  const float a = 1.0f - std::exp(-2.0f * util::kPi * cutoff / sampleRate);
  state_ += a * (input - state_);
  return state_;
}

}  // namespace midipiano::dsp

#include "midipiano/dsp/LowPassFilter.h"

#include <algorithm>
#include <cmath>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

void LowPassFilter::setResonance(float amount) { resonance_ = std::clamp(amount, 0.0f, 1.0f); }

void LowPassFilter::setDrive(float drive) { drive_ = std::clamp(drive, 1.0f, 10.0f); }

float LowPassFilter::process(float input, float sampleRate, float cutoffHz) {
  const float cutoff = std::clamp(cutoffHz, 20.0f, sampleRate * 0.45f);
  const float g = 1.0f - std::exp(-2.0f * util::kPi * cutoff / sampleRate);
  // Terugkoppeling: rond k = 4 begint een analoge ladderfilter zelf te oscilleren. Digitaal is er bij
  // hoge cutoff meer nodig (de trappen lopen een beetje achter), vandaar de deling door (1 - g).
  // We gaan er net over; de tanh houdt het begrensd.
  const float k = resonance_ * 4.4f / (1.0f - std::min(g, 0.6f));
  // Bij veel resonantie verliest een ladderfilter bas; een beetje extra ingangsversterking compenseert.
  const float x = std::tanh(drive_ * input * (1.0f + 2.2f * resonance_) - k * stage_[3]);
  stage_[0] += g * (x - std::tanh(stage_[0]));
  stage_[1] += g * (std::tanh(stage_[0]) - std::tanh(stage_[1]));
  stage_[2] += g * (std::tanh(stage_[1]) - std::tanh(stage_[2]));
  stage_[3] += g * (std::tanh(stage_[2]) - std::tanh(stage_[3]));
  return stage_[3];
}

}  // namespace midipiano::dsp

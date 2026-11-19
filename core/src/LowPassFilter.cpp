#include "midipiano/dsp/LowPassFilter.h"

#include <algorithm>
#include <cmath>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

namespace {
// Snelle benadering van tanh (Pade): bijna even goed voor audio, maar zonder exp(). Buiten +-3 is
// tanh praktisch +-1.
inline float fastTanh(float x) {
  x = std::clamp(x, -3.0f, 3.0f);
  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
}  // namespace

void LowPassFilter::setCutoff(float hz) {
  cutoffHz_ = hz;
  coeffHz_ = -1.0f;  // opnieuw berekenen bij het volgende sample
}

void LowPassFilter::setResonance(float amount) { resonance_ = std::clamp(amount, 0.0f, 1.0f); }

void LowPassFilter::setDrive(float drive) { drive_ = std::clamp(drive, 1.0f, 10.0f); }

void LowPassFilter::setModulatedCutoff(float hz, float sampleRate) { updateCoefficients(hz, sampleRate); }

void LowPassFilter::updateCoefficients(float hz, float sampleRate) {
  if (hz == coeffHz_ && sampleRate == coeffRate_ && resonance_ == coeffResonance_) return;
  coeffHz_ = hz;
  coeffRate_ = sampleRate;
  coeffResonance_ = resonance_;
  const float cutoff = std::clamp(hz, 20.0f, sampleRate * 0.45f);
  g_ = 1.0f - std::exp(-2.0f * util::kPi * cutoff / sampleRate);
  // Terugkoppeling: rond k = 4 begint een analoge ladderfilter zelf te oscilleren. Digitaal is er bij
  // hoge cutoff meer nodig (de trappen lopen een beetje achter), vandaar de deling door (1 - g).
  k_ = resonance_ * 4.4f / (1.0f - std::min(g_, 0.6f));
}

float LowPassFilter::process(float input, float sampleRate) {
  if (coeffHz_ < 0.0f || coeffRate_ != sampleRate) updateCoefficients(cutoffHz_, sampleRate);
  else if (coeffResonance_ != resonance_) updateCoefficients(coeffHz_, sampleRate);
  // Bij veel resonantie verliest een ladderfilter bas; een beetje extra ingangsversterking compenseert.
  const float x = fastTanh(drive_ * input * (1.0f + 2.2f * resonance_) - k_ * stage_[3]);
  stage_[0] += g_ * (x - fastTanh(stage_[0]));
  const float t0 = fastTanh(stage_[0]);
  stage_[1] += g_ * (t0 - fastTanh(stage_[1]));
  const float t1 = fastTanh(stage_[1]);
  stage_[2] += g_ * (t1 - fastTanh(stage_[2]));
  const float t2 = fastTanh(stage_[2]);
  stage_[3] += g_ * (t2 - fastTanh(stage_[3]));
  return stage_[3];
}

}  // namespace midipiano::dsp

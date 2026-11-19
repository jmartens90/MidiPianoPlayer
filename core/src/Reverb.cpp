#include "midipiano/dsp/Reverb.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

namespace {
// Lengtes (in samples bij 44,1 kHz) uit Freeverb: onderling ondeelbaar, zodat de echo's van de
// verschillende filters niet samenvallen en de galm niet "metalig" klinkt.
constexpr std::array<int, 8> kCombTuning{1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
constexpr std::array<int, 4> kAllpassTuning{556, 441, 341, 225};
constexpr float kInputGain = 0.015f;
constexpr float kMaxPredelay = 0.25f;  // seconden

float onePoleCoeff(float hz, float sampleRate) {
  return 1.0f - std::exp(-2.0f * util::kPi * std::clamp(hz, 10.0f, sampleRate * 0.45f) / sampleRate);
}
}  // namespace

float Reverb::Comb::process(float in, float feedback, float damp) {
  const float out = line[pos];
  store = out * (1.0f - damp) + store * damp;
  line[pos] = in + store * feedback;
  if (++pos >= line.size()) pos = 0;
  return out;
}

float Reverb::Allpass::process(float in) {
  const float buffered = line[pos];
  line[pos] = in + buffered * 0.5f;
  if (++pos >= line.size()) pos = 0;
  return buffered - in;
}

Reverb::Reverb(float sampleRate) { setSampleRate(sampleRate); }

void Reverb::setSampleRate(float sampleRate) {
  sampleRate_ = sampleRate;
  const float scale = sampleRate / 44100.0f;
  combs_.assign(kCombTuning.size(), Comb{});
  for (std::size_t i = 0; i < combs_.size(); ++i)
    combs_[i].line.assign(static_cast<std::size_t>(kCombTuning[i] * scale) + 1, 0.0f);
  allpasses_.assign(kAllpassTuning.size(), Allpass{});
  for (std::size_t i = 0; i < allpasses_.size(); ++i)
    allpasses_[i].line.assign(static_cast<std::size_t>(kAllpassTuning[i] * scale) + 1, 0.0f);
  predelayLine_.assign(static_cast<std::size_t>(kMaxPredelay * sampleRate) + 1, 0.0f);
  predelayPos_ = 0;
  updateCoefficients();
}

void Reverb::setSettings(const Settings& settings) {
  settings_ = settings;
  settings_.mix = std::clamp(settings_.mix, 0.0f, 1.0f);
  settings_.predelay = std::clamp(settings_.predelay, 0.0f, kMaxPredelay);
  settings_.decay = std::clamp(settings_.decay, 0.0f, 1.0f);
  settings_.damping = std::clamp(settings_.damping, 0.0f, 1.0f);
  updateCoefficients();
}

void Reverb::setAmount(float mix) {
  Settings settings = settings_;
  settings.mix = mix;
  setSettings(settings);
}

void Reverb::updateCoefficients() {
  feedback_ = 0.70f + 0.285f * settings_.decay;  // 0,985 = bijna eindeloos, maar nog stabiel
  predelaySamples_ = std::min(static_cast<std::size_t>(settings_.predelay * sampleRate_), predelayLine_.size() - 1);
  lowCutCoeff_ = onePoleCoeff(settings_.lowCutHz, sampleRate_);
  highCutCoeff_ = onePoleCoeff(settings_.highCutHz, sampleRate_);
}

void Reverb::clear() {
  for (auto& comb : combs_) {
    std::fill(comb.line.begin(), comb.line.end(), 0.0f);
    comb.store = 0.0f;
  }
  for (auto& allpass : allpasses_) std::fill(allpass.line.begin(), allpass.line.end(), 0.0f);
  std::fill(predelayLine_.begin(), predelayLine_.end(), 0.0f);
  lowCutState_ = highCutState_ = 0.0f;
}

void Reverb::process(float* buffer, std::size_t frames) {
  if (settings_.mix <= 0.0f) return;
  const float wet = settings_.mix * 3.0f;
  const float dry = 1.0f - settings_.mix * 0.4f;
  const float damp = settings_.damping * 0.7f;
  const std::size_t lineSize = predelayLine_.size();
  for (std::size_t i = 0; i < frames; ++i) {
    // Predelay: schrijf nu, lees wat predelaySamples_ geleden binnenkwam.
    predelayLine_[predelayPos_] = buffer[i];
    const float delayed = predelayLine_[(predelayPos_ + lineSize - predelaySamples_) % lineSize];
    if (++predelayPos_ >= lineSize) predelayPos_ = 0;

    // Pre-filter: low cut (ingang min zijn eigen laagdoorlaat = hoogdoorlaat).
    lowCutState_ += lowCutCoeff_ * (delayed - lowCutState_);
    const float in = (delayed - lowCutState_) * kInputGain;

    float sum = 0.0f;
    for (auto& comb : combs_) sum += comb.process(in, feedback_, damp);
    for (auto& allpass : allpasses_) sum = allpass.process(sum);

    // Post-filter: high cut op de galm.
    highCutState_ += highCutCoeff_ * (sum - highCutState_);
    buffer[i] = buffer[i] * dry + highCutState_ * wet;
  }
}

}  // namespace midipiano::dsp

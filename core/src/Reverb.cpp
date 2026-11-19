#include "midipiano/dsp/Reverb.h"

#include <algorithm>
#include <array>

namespace midipiano::dsp {

namespace {
// Lengtes (in samples bij 44,1 kHz) uit Freeverb: onderling ondeelbaar, zodat de echo's van de
// verschillende filters niet samenvallen en de galm niet "metalig" klinkt.
constexpr std::array<int, 8> kCombTuning{1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
constexpr std::array<int, 4> kAllpassTuning{556, 441, 341, 225};
constexpr float kRoomFeedback = 0.84f;
constexpr float kDamping = 0.25f;
constexpr float kInputGain = 0.015f;
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
  const float scale = sampleRate / 44100.0f;
  combs_.assign(kCombTuning.size(), Comb{});
  for (std::size_t i = 0; i < combs_.size(); ++i)
    combs_[i].line.assign(static_cast<std::size_t>(kCombTuning[i] * scale) + 1, 0.0f);
  allpasses_.assign(kAllpassTuning.size(), Allpass{});
  for (std::size_t i = 0; i < allpasses_.size(); ++i)
    allpasses_[i].line.assign(static_cast<std::size_t>(kAllpassTuning[i] * scale) + 1, 0.0f);
}

void Reverb::setAmount(float amount) { amount_ = std::clamp(amount, 0.0f, 1.0f); }

void Reverb::clear() {
  for (auto& comb : combs_) {
    std::fill(comb.line.begin(), comb.line.end(), 0.0f);
    comb.store = 0.0f;
  }
  for (auto& allpass : allpasses_) std::fill(allpass.line.begin(), allpass.line.end(), 0.0f);
}

void Reverb::process(float* buffer, std::size_t frames) {
  if (amount_ <= 0.0f) return;
  const float wet = amount_ * 3.0f;
  const float dry = 1.0f - amount_ * 0.4f;
  for (std::size_t i = 0; i < frames; ++i) {
    const float in = buffer[i] * kInputGain;
    float sum = 0.0f;
    for (auto& comb : combs_) sum += comb.process(in, kRoomFeedback, kDamping);
    for (auto& allpass : allpasses_) sum = allpass.process(sum);
    buffer[i] = buffer[i] * dry + sum * wet;
  }
}

}  // namespace midipiano::dsp

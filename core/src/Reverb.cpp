#include "midipiano/dsp/Reverb.h"

#include <algorithm>
#include <cmath>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

namespace {
// Alle lengtes uit Dattorro's artikel, in samples bij 29761 Hz (zijn samplerate). Ze zijn onderling
// ondeelbaar, zodat de echo's niet samenvallen.
constexpr float kDattorroRate = 29761.0f;
constexpr float kDiffuser[4] = {142.0f, 107.0f, 379.0f, 277.0f};
constexpr float kModAllpass[2] = {672.0f, 908.0f};
constexpr float kDelayA[2] = {4453.0f, 4217.0f};
constexpr float kTankAllpass[2] = {1800.0f, 2656.0f};
constexpr float kDelayB[2] = {3720.0f, 3163.0f};
// Aftakkingen voor de uitgang: 7 per kanaal, uit Dattorro's tabel. Volgorde: L0..L6, R0..R6.
constexpr float kTaps[14] = {266, 2974, 1913, 1996, 1990, 187, 1066, 353, 3627, 1228, 2673, 2111, 335, 121};
// Vroege reflecties: tijd in ms (bij grootte 1) en sterkte; even = links, oneven = rechts.
constexpr float kEarlyMs[8] = {7.0f, 11.0f, 19.0f, 23.0f, 31.0f, 41.0f, 53.0f, 67.0f};
constexpr float kEarlyGain[8] = {0.55f, 0.5f, 0.42f, 0.4f, 0.32f, 0.27f, 0.2f, 0.15f};
constexpr float kMaxPredelay = 0.25f;  // seconden
constexpr float kMaxSize = 1.6f;       // grootste schaal van de tank t.o.v. Dattorro's lengtes
constexpr float kExcursion = 16.0f;    // modulatiediepte in samples bij 29761 Hz
constexpr float kLfoHz = 0.7f;

float onePoleCoeff(float hz, float sampleRate) {
  return 1.0f - std::exp(-2.0f * util::kPi * std::clamp(hz, 10.0f, sampleRate * 0.45f) / sampleRate);
}

std::size_t nextPowerOfTwo(std::size_t n) {
  std::size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}
}  // namespace

void Reverb::DelayLine::allocate(std::size_t maxSamples) {
  buffer_.assign(nextPowerOfTwo(maxSamples + 2), 0.0f);
  mask_ = buffer_.size() - 1;
  pos_ = 0;
}

void Reverb::DelayLine::clear() { std::fill(buffer_.begin(), buffer_.end(), 0.0f); }

float Reverb::DelayLine::readFrac(float delay) const {
  const auto whole = static_cast<std::size_t>(delay);
  const float frac = delay - static_cast<float>(whole);
  const float a = read(whole);
  const float b = read(whole + 1);
  return a + (b - a) * frac;
}

float Reverb::Allpass::process(float in, float delay, float gain) {
  // Lattice-vorm: v = in - g * vertraagd, uit = vertraagd + g * v.
  const float delayed = line.readFrac(delay - 1.0f);
  const float v = in - gain * delayed;
  line.write(v);
  return delayed + gain * v;
}

Reverb::Reverb(float sampleRate) { setSampleRate(sampleRate); }

std::size_t Reverb::scaled(float samplesAt29k, bool sized) const {
  const float size = sized ? 0.4f + settings_.size * (kMaxSize - 0.4f) : 1.0f;
  return std::max<std::size_t>(2, static_cast<std::size_t>(samplesAt29k * size * sampleRate_ / kDattorroRate));
}

void Reverb::setSampleRate(float sampleRate) {
  sampleRate_ = sampleRate;
  const float scale = sampleRate / kDattorroRate;
  auto maxLen = [&](float samples, bool sized) {
    return static_cast<std::size_t>(samples * scale * (sized ? kMaxSize : 1.0f) + kExcursion * scale * 2.0f) + 4;
  };
  predelay_.allocate(static_cast<std::size_t>((kMaxPredelay + 0.15f) * sampleRate));
  for (int i = 0; i < 4; ++i) diffusers_[i].line.allocate(maxLen(kDiffuser[i], false));
  for (int s = 0; s < 2; ++s) {
    modAllpass_[s].line.allocate(maxLen(kModAllpass[s], true));
    delayA_[s].allocate(maxLen(kDelayA[s], true));
    tankAllpass_[s].line.allocate(maxLen(kTankAllpass[s], true));
    delayB_[s].allocate(maxLen(kDelayB[s], true));
  }
  clear();
  updateCoefficients();
}

void Reverb::setSettings(const Settings& settings) {
  settings_ = settings;
  settings_.mix = std::clamp(settings_.mix, 0.0f, 1.0f);
  settings_.predelay = std::clamp(settings_.predelay, 0.0f, kMaxPredelay);
  settings_.decay = std::clamp(settings_.decay, 0.0f, 1.0f);
  settings_.damping = std::clamp(settings_.damping, 0.0f, 1.0f);
  settings_.size = std::clamp(settings_.size, 0.0f, 1.0f);
  settings_.width = std::clamp(settings_.width, 0.0f, 1.0f);
  updateCoefficients();
}

void Reverb::setAmount(float mix) {
  Settings settings = settings_;
  settings.mix = mix;
  setSettings(settings);
}

void Reverb::updateCoefficients() {
  const float scale = sampleRate_ / kDattorroRate;
  predelaySamples_ = static_cast<std::size_t>(settings_.predelay * sampleRate_);
  for (int i = 0; i < 4; ++i) diffuserDelay_[i] = kDiffuser[i] * scale;
  for (int s = 0; s < 2; ++s) {
    modDelay_[s] = static_cast<float>(scaled(kModAllpass[s], true));
    delayALength_[s] = scaled(kDelayA[s], true);
    tankAllpassDelay_[s] = static_cast<float>(scaled(kTankAllpass[s], true));
    delayBLength_[s] = scaled(kDelayB[s], true);
  }
  for (std::size_t i = 0; i < taps_.size(); ++i) taps_[i] = scaled(kTaps[i], true);
  const float roomScale = 0.4f + settings_.size * 1.2f;
  for (std::size_t i = 0; i < earlyTaps_.size(); ++i)
    earlyTaps_[i] = predelaySamples_ + static_cast<std::size_t>(kEarlyMs[i] * 0.001f * roomScale * sampleRate_);
  excursion_ = kExcursion * scale;
  decay_ = 0.2f + 0.78f * settings_.decay;  // 0,98: heel lang, maar nog stabiel
  lowCutCoeff_ = onePoleCoeff(settings_.lowCutHz, sampleRate_);
  highCutCoeff_ = onePoleCoeff(settings_.highCutHz, sampleRate_);
  lfoStep_ = 2.0f * util::kPi * kLfoHz / sampleRate_;
}

void Reverb::clear() {
  predelay_.clear();
  for (auto& d : diffusers_) d.line.clear();
  for (int s = 0; s < 2; ++s) {
    modAllpass_[s].line.clear();
    delayA_[s].clear();
    tankAllpass_[s].line.clear();
    delayB_[s].clear();
    tankFeedback_[s] = dampState_[s] = highCutState_[s] = 0.0f;
  }
  lowCutState_ = 0.0f;
}

void Reverb::process(const float* in, float* left, float* right, std::size_t frames) {
  if (settings_.mix <= 0.0f) {
    if (left != in) std::copy(in, in + frames, left);
    if (right != in) std::copy(in, in + frames, right);
    return;
  }
  const float wet = settings_.mix * 1.6f;
  const float dry = 1.0f - settings_.mix * 0.4f;
  const float damp = settings_.damping * 0.8f;
  const float earlyLevel = 0.35f;
  for (std::size_t i = 0; i < frames; ++i) {
    const float x = in[i];
    // Predelay en low cut. De vroege reflecties lezen verder terug in dezelfde lijn.
    lowCutState_ += lowCutCoeff_ * (x - lowCutState_);
    predelay_.write(x - lowCutState_);
    const float delayed = predelay_.read(predelaySamples_);
    float earlyL = 0.0f, earlyR = 0.0f;
    for (std::size_t t = 0; t < earlyTaps_.size(); t += 2) {
      earlyL += predelay_.read(earlyTaps_[t]) * kEarlyGain[t];
      earlyR += predelay_.read(earlyTaps_[t + 1]) * kEarlyGain[t + 1];
    }

    // Diffusie: vier allpasses maken van elke puls een dichte wolk.
    float diffused = delayed * 0.5f;
    diffused = diffusers_[0].process(diffused, diffuserDelay_[0], 0.75f);
    diffused = diffusers_[1].process(diffused, diffuserDelay_[1], 0.75f);
    diffused = diffusers_[2].process(diffused, diffuserDelay_[2], 0.625f);
    diffused = diffusers_[3].process(diffused, diffuserDelay_[3], 0.625f);

    // Tank: elke helft krijgt de ingang plus de uitgang van de andere helft (kruiselings).
    lfoPhase_ += lfoStep_;
    if (lfoPhase_ > 2.0f * util::kPi) lfoPhase_ -= 2.0f * util::kPi;
    const float lfo[2] = {std::sin(lfoPhase_), std::cos(lfoPhase_ * 0.83f)};
    float halfOut[2];
    for (int s = 0; s < 2; ++s) {
      float v = diffused + tankFeedback_[1 - s] * decay_;
      v = modAllpass_[s].process(v, modDelay_[s] + excursion_ * (1.0f + lfo[s]), -0.7f);
      delayA_[s].write(v);
      v = delayA_[s].read(delayALength_[s]);
      dampState_[s] += (1.0f - damp) * (v - dampState_[s]);  // hoge tonen sterven sneller uit
      v = tankAllpass_[s].process(dampState_[s] * decay_, tankAllpassDelay_[s], 0.5f);
      delayB_[s].write(v);
      halfOut[s] = delayB_[s].read(delayBLength_[s]);
    }
    tankFeedback_[0] = halfOut[0];
    tankFeedback_[1] = halfOut[1];

    // Uitgang: elk kanaal leest op zeven plaatsen in beide helften (Dattorro's tabel).
    const DelayLine& a0 = delayA_[0];
    const DelayLine& a1 = delayA_[1];
    const DelayLine& b0 = delayB_[0];
    const DelayLine& b1 = delayB_[1];
    const DelayLine& p0 = tankAllpass_[0].line;
    const DelayLine& p1 = tankAllpass_[1].line;
    float wetL = a1.read(taps_[0]) + a1.read(taps_[1]) - p1.read(taps_[2]) + b1.read(taps_[3]) -
                 a0.read(taps_[4]) - p0.read(taps_[5]) - b0.read(taps_[6]);
    float wetR = a0.read(taps_[7]) + a0.read(taps_[8]) - p0.read(taps_[9]) + b0.read(taps_[10]) -
                 a1.read(taps_[11]) - p1.read(taps_[12]) - b1.read(taps_[13]);
    wetL = wetL * 0.6f + earlyL * earlyLevel;
    wetR = wetR * 0.6f + earlyR * earlyLevel;

    // Breedte: mid/side. 0 = beide kanalen hetzelfde, 1 = volledig verschillend.
    const float mid = 0.5f * (wetL + wetR);
    const float side = 0.5f * (wetL - wetR) * settings_.width;
    wetL = mid + side;
    wetR = mid - side;

    // Post-filter: high cut op de galm.
    highCutState_[0] += highCutCoeff_ * (wetL - highCutState_[0]);
    highCutState_[1] += highCutCoeff_ * (wetR - highCutState_[1]);
    left[i] = x * dry + highCutState_[0] * wet;
    right[i] = x * dry + highCutState_[1] * wet;
  }
}

void Reverb::process(float* buffer, std::size_t frames) {
  std::vector<float> right(frames);
  process(buffer, buffer, right.data(), frames);
  for (std::size_t i = 0; i < frames; ++i) buffer[i] = 0.5f * (buffer[i] + right[i]);
}

}  // namespace midipiano::dsp

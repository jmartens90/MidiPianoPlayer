#include "midipiano/dsp/Wavetable.h"

#include <algorithm>
#include <cmath>
#include <complex>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

namespace {

using Complex = std::complex<float>;

// Iteratieve radix-2 FFT (Cooley-Tukey). inverse = true geeft de inverse FFT (zonder 1/N).
void fft(std::vector<Complex>& a, bool inverse) {
  const std::size_t n = a.size();
  for (std::size_t i = 1, j = 0; i < n; ++i) {  // bit-reversal permutatie
    std::size_t bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[i], a[j]);
  }
  for (std::size_t len = 2; len <= n; len <<= 1) {
    const float angle = 2.0f * util::kPi / static_cast<float>(len) * (inverse ? 1.0f : -1.0f);
    const Complex step(std::cos(angle), std::sin(angle));
    for (std::size_t i = 0; i < n; i += len) {
      Complex w(1.0f, 0.0f);
      for (std::size_t k = 0; k < len / 2; ++k) {
        const Complex u = a[i + k];
        const Complex v = a[i + k + len / 2] * w;
        a[i + k] = u + v;
        a[i + k + len / 2] = u - v;
        w *= step;
      }
    }
  }
}

}  // namespace

Wavetable::Wavetable(std::string name, const ShapeFunction& shape)
    : name_(std::move(name)), data_(kFrames * kMipLevels * kSize) {
  std::vector<Complex> spectrum(kSize);
  std::vector<Complex> work(kSize);
  for (std::size_t frame = 0; frame < kFrames; ++frame) {
    const float t = static_cast<float>(frame) / static_cast<float>(kFrames - 1);
    for (std::size_t i = 0; i < kSize; ++i)
      spectrum[i] = Complex(shape(static_cast<float>(i) / kSize, t), 0.0f);
    fft(spectrum, false);
    spectrum[0] = 0.0f;  // geen DC-offset

    // Normaliseer op de volle versie, zodat elke frame even luid is.
    float peak = 0.0f;
    for (std::size_t level = 0; level < kMipLevels; ++level) {
      const std::size_t maxHarmonic = (kSize / 2) >> level;  // 1024, 512, ..., 1
      for (std::size_t k = 0; k < kSize; ++k) {
        const std::size_t harmonic = k <= kSize / 2 ? k : kSize - k;
        work[k] = harmonic <= maxHarmonic && harmonic < kSize / 2 ? spectrum[k] : Complex(0.0f, 0.0f);
      }
      fft(work, true);
      float* out = &data_[(frame * kMipLevels + level) * kSize];
      for (std::size_t i = 0; i < kSize; ++i) out[i] = work[i].real() / kSize;
      if (level == 0)
        for (std::size_t i = 0; i < kSize; ++i) peak = std::max(peak, std::fabs(out[i]));
    }
    const float gain = peak > 1e-6f ? 1.0f / peak : 1.0f;
    for (std::size_t level = 0; level < kMipLevels; ++level) {
      float* out = &data_[(frame * kMipLevels + level) * kSize];
      for (std::size_t i = 0; i < kSize; ++i) out[i] *= gain;
    }
  }
}

std::size_t Wavetable::mipLevelFor(float frequencyHz, float sampleRate) {
  // Hoeveel boventonen passen er onder de halve samplerate? Kies het eerste niveau met niet meer.
  const float allowed = sampleRate * 0.5f / std::max(frequencyHz, 1.0f);
  std::size_t level = 0;
  while (level + 1 < kMipLevels && static_cast<float>((kSize / 2) >> level) > allowed) ++level;
  return level;
}

float Wavetable::read(float phase, float position, std::size_t mipLevel) const {
  const float framePos = std::clamp(position, 0.0f, 1.0f) * (kFrames - 1);
  const std::size_t frameA = static_cast<std::size_t>(framePos);
  const std::size_t frameB = std::min(frameA + 1, kFrames - 1);
  const float frameMix = framePos - static_cast<float>(frameA);

  const float index = phase * kSize;
  const std::size_t i0 = static_cast<std::size_t>(index) % kSize;
  const std::size_t i1 = (i0 + 1) % kSize;
  const float frac = index - std::floor(index);

  const float* a = table(frameA, mipLevel);
  const float* b = table(frameB, mipLevel);
  const float sampleA = a[i0] + (a[i1] - a[i0]) * frac;
  const float sampleB = b[i0] + (b[i1] - b[i0]) * frac;
  return sampleA + (sampleB - sampleA) * frameMix;
}

void WavetableOscillator::prepare(float frequencyHz, float sampleRate) {
  if (frequencyHz == lastFrequency_ && sampleRate == lastSampleRate_) return;  // meestal: zelfde noot
  lastFrequency_ = frequencyHz;
  lastSampleRate_ = sampleRate;
  mipLevel_ = Wavetable::mipLevelFor(frequencyHz, sampleRate);
}

void WavetableOscillator::setPosition(float position) { position_ = std::clamp(position, 0.0f, 1.0f); }

float WavetableOscillator::sample(float phase) const { return table_->read(phase, position_, mipLevel_); }

}  // namespace midipiano::dsp

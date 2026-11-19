#include "midipiano/dsp/Waveforms.h"

#include <cmath>
#include <stdexcept>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

float SineOscillator::sample(float phase) const { return std::sin(2.0f * util::kPi * phase); }

float SawOscillator::sample(float phase) const { return 2.0f * phase - 1.0f; }

float SquareOscillator::sample(float phase) const { return phase < 0.5f ? 1.0f : -1.0f; }

float TriangleOscillator::sample(float phase) const {
  return phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
}

std::unique_ptr<Oscillator> OscillatorFactory::create(Waveform waveform) {
  switch (waveform) {
    case Waveform::Sine: return std::make_unique<SineOscillator>();
    case Waveform::Saw: return std::make_unique<SawOscillator>();
    case Waveform::Square: return std::make_unique<SquareOscillator>();
    case Waveform::Triangle: return std::make_unique<TriangleOscillator>();
    case Waveform::Count: break;
  }
  return nullptr;
}

std::string waveformName(Waveform waveform) {
  const auto oscillator = OscillatorFactory::create(waveform);
  return oscillator ? oscillator->name() : "Unknown";
}

Waveform waveformFromName(const std::string& name) {
  for (int i = 0; i < static_cast<int>(Waveform::Count); ++i) {
    const auto waveform = static_cast<Waveform>(i);
    if (waveformName(waveform) == name) return waveform;
  }
  throw std::invalid_argument("onbekende golfvorm: " + name);
}

}  // namespace midipiano::dsp

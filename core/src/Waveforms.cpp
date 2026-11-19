#include "midipiano/dsp/Waveforms.h"

#include <array>
#include <cmath>
#include <mutex>
#include <stdexcept>

#include "midipiano/dsp/Wavetable.h"

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

float SineOscillator::sample(float phase) const { return std::sin(2.0f * util::kPi * phase); }

float SawOscillator::sample(float phase) const { return 2.0f * phase - 1.0f; }

float SquareOscillator::sample(float phase) const { return phase < 0.5f ? 1.0f : -1.0f; }

float TriangleOscillator::sample(float phase) const {
  return phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
}

namespace {

constexpr float kTwoPi = 2.0f * util::kPi;

float fract(float x) { return x - std::floor(x); }

// De vorm van elke wavetable: x = fase in de periode, t = plaats in de tabel (Position-knop).
Wavetable::ShapeFunction shapeFor(Waveform waveform) {
  switch (waveform) {
    case Waveform::Analog:  // sinus -> driehoek -> zaagtand -> blok
      return [](float x, float t) {
        const std::array<float, 4> shapes{std::sin(kTwoPi * x), x < 0.5f ? 4.0f * x - 1.0f : 3.0f - 4.0f * x,
                                          2.0f * x - 1.0f, x < 0.5f ? 1.0f : -1.0f};
        const float pos = t * 3.0f;
        const std::size_t i = std::min<std::size_t>(static_cast<std::size_t>(pos), 2);
        const float mix = pos - static_cast<float>(i);
        return shapes[i] + (shapes[i + 1] - shapes[i]) * mix;
      };
    case Waveform::Harmonics:  // additief: hoe verder, hoe meer boventonen erbij
      return [](float x, float t) {
        const float count = 1.0f + t * 63.0f;
        float sum = 0.0f;
        for (int h = 1; h <= 64; ++h) {
          const float weight = std::clamp(count - static_cast<float>(h) + 1.0f, 0.0f, 1.0f);
          if (weight > 0.0f) sum += weight * std::sin(kTwoPi * h * x) / static_cast<float>(h);
        }
        return sum;
      };
    case Waveform::PWM:  // blokgolf die steeds smaller wordt (pulsbreedte 50% -> 5%)
      return [](float x, float t) { return x < 0.5f - 0.45f * t ? 1.0f : -1.0f; };
    case Waveform::Sync:  // "hard sync": een snellere zaagtand die elke periode herstart
      return [](float x, float t) { return 2.0f * fract(x * (1.0f + 7.0f * t)) - 1.0f; };
    case Waveform::Formant:  // sinus-burst in een venster: klinkt als een klinker die opengaat
      return [](float x, float t) {
        return std::sin(kTwoPi * (1.0f + 11.0f * t) * x) * 0.5f * (1.0f - std::cos(kTwoPi * x));
      };
    case Waveform::Fold:  // wavefolder: een sinus die steeds harder over zichzelf plooit
      return [](float x, float t) { return std::sin(0.5f * util::kPi * (1.0f + 7.0f * t) * std::sin(kTwoPi * x)); };
    default:
      return nullptr;
  }
}

// Elke wavetable wordt maar een keer berekend en gedeeld door alle stemmen (shared_ptr).
std::shared_ptr<const Wavetable> sharedWavetable(Waveform waveform) {
  static std::mutex mutex;
  static std::array<std::shared_ptr<const Wavetable>, static_cast<std::size_t>(Waveform::Count)> cache;
  std::lock_guard<std::mutex> lock(mutex);
  auto& slot = cache[static_cast<std::size_t>(waveform)];
  if (!slot) {
    const auto shape = shapeFor(waveform);
    if (!shape) return nullptr;
    slot = std::make_shared<const Wavetable>(waveformName(waveform), shape);
  }
  return slot;
}

const char* wavetableName(Waveform waveform) {
  switch (waveform) {
    case Waveform::Analog: return "Analog";
    case Waveform::Harmonics: return "Harmonics";
    case Waveform::PWM: return "PWM";
    case Waveform::Sync: return "Sync";
    case Waveform::Formant: return "Formant";
    case Waveform::Fold: return "Fold";
    default: return nullptr;
  }
}

}  // namespace

bool isWavetable(Waveform waveform) { return wavetableName(waveform) != nullptr; }

std::unique_ptr<Oscillator> OscillatorFactory::create(Waveform waveform) {
  if (isWavetable(waveform)) {
    auto table = sharedWavetable(waveform);
    return table ? std::make_unique<WavetableOscillator>(std::move(table)) : nullptr;
  }
  switch (waveform) {
    case Waveform::Sine: return std::make_unique<SineOscillator>();
    case Waveform::Saw: return std::make_unique<SawOscillator>();
    case Waveform::Square: return std::make_unique<SquareOscillator>();
    case Waveform::Triangle: return std::make_unique<TriangleOscillator>();
    default: break;  // wavetables staan hierboven, Count is geen golfvorm
  }
  return nullptr;
}

std::string waveformName(Waveform waveform) {
  if (const char* name = wavetableName(waveform)) return name;
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

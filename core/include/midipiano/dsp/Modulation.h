#ifndef MIDIPIANO_DSP_MODULATION_H
#define MIDIPIANO_DSP_MODULATION_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace midipiano::dsp {

// Waar een modulatiebron (LFO of modulatie-envelope) aan draait: dezelfde knoppen als op het
// Synth-paneel. "None" = de bron staat uit.
enum class ModTarget : std::uint8_t { None, Pitch, Cutoff, Resonance, Position, Drive, Volume, Count };
constexpr std::size_t kModTargets = static_cast<std::size_t>(ModTarget::Count);
std::vector<std::string> modTargetNames();

// Per bestemming de opgetelde modulatie (-1..1 per bron), zo doorgegeven aan elke stem.
using ModValues = std::array<float, kModTargets>;

// Laagfrequente oscillator: een traag golfje (0,05 tot 20 Hz) dat een knop heen en weer draait.
// Sine = vibrato/zweven, Triangle = gelijkmatig, Saw = oplopen en terugvallen, Square = trillen
// tussen twee waarden, Random = sample & hold (elke periode een nieuwe toevalswaarde).
class Lfo {
 public:
  enum class Shape : std::uint8_t { Sine, Triangle, Saw, Square, Random, Count };
  static std::vector<std::string> shapeNames();

  void setRate(float hz) { rateHz_ = hz; }
  void setShape(Shape shape) { shape_ = shape; }
  void reset() { phase_ = 0.0f; }
  // Schuift seconds verder en geeft de nieuwe waarde (-1..1).
  float advance(float seconds);
  float value() const { return value_; }

 private:
  float phase_ = 0.0f;  // 0..1
  float rateHz_ = 1.0f;
  Shape shape_ = Shape::Sine;
  float value_ = 0.0f;
  float held_ = 0.0f;  // laatste toevalswaarde (Random)
  std::uint32_t seed_ = 12345u;
};

// Knopstand (-1..1) naar modulatiediepte: kwadratisch, zodat kleine waarden (vibrato) fijn instelbaar zijn.
inline float modDepth(float knob) { return knob * (knob < 0.0f ? -knob : knob); }

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_MODULATION_H

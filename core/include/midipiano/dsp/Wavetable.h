#ifndef MIDIPIANO_DSP_WAVETABLE_H
#define MIDIPIANO_DSP_WAVETABLE_H

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "midipiano/dsp/Oscillator.h"

namespace midipiano::dsp {

// Een wavetable is een reeks van kFrames enkele golfperiodes ("frames") die geleidelijk van vorm
// veranderen. Met de Position-knop schuif je door die frames; tussen twee frames wordt gemengd, dus
// de klank verandert vloeiend (morphen). Dat is het principe van synths als de PPG Wave of Serum.
//
// Tegen aliasing (valse, metalige tonen bij hoge noten) heeft elke frame kMipLevels versies: voor
// elk octaaf een versie met alleen de boventonen die nog onder de halve samplerate passen. Die
// versies worden een keer berekend met een FFT, bij het opstarten.
class Wavetable {
 public:
  static constexpr std::size_t kSize = 2048;   // samples per periode
  static constexpr std::size_t kFrames = 8;
  static constexpr std::size_t kMipLevels = 11;  // 1024, 512, ..., 1 boventonen

  // Vorm van een frame: x = fase 0..1, t = plaats in de tabel 0..1. Geeft een sample terug.
  using ShapeFunction = std::function<float(float x, float t)>;

  Wavetable(std::string name, const ShapeFunction& shape);

  const std::string& name() const { return name_; }
  // Leest de tabel op fase 0..1 en positie 0..1, uit anti-aliasing-versie mipLevel.
  float read(float phase, float position, std::size_t mipLevel) const;
  static std::size_t mipLevelFor(float frequencyHz, float sampleRate);

 private:
  const float* table(std::size_t frame, std::size_t level) const {
    return &data_[(frame * kMipLevels + level) * kSize];
  }

  std::string name_;
  std::vector<float> data_;  // kFrames x kMipLevels x kSize, achter elkaar
};

// Oscillator die een wavetable afspeelt. Erft van Oscillator, net als de vier klassieke golfvormen,
// dus Voice merkt het verschil niet: hij roept dezelfde next() op.
class WavetableOscillator final : public Oscillator {
 public:
  explicit WavetableOscillator(std::shared_ptr<const Wavetable> table) : table_(std::move(table)) {}

  void prepare(float frequencyHz, float sampleRate) override;
  void setPosition(float position) override;
  float sample(float phase) const override;
  std::string name() const override { return table_->name(); }

 private:
  std::shared_ptr<const Wavetable> table_;  // gedeeld door alle stemmen: 1x geheugen
  float position_ = 0.0f;
  std::size_t mipLevel_ = 0;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_WAVETABLE_H

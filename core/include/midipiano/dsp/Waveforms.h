#ifndef MIDIPIANO_DSP_WAVEFORMS_H
#define MIDIPIANO_DSP_WAVEFORMS_H

#include <cstdint>
#include <memory>
#include <string>

#include "midipiano/dsp/Oscillator.h"

namespace midipiano::dsp {

// Vereiste 37: 37.2 useful unsigned chars or other better usage of memory efficient type
// Er zijn maar 4 golfvormen, dus een enum class op std::uint8_t (1 byte) volstaat in plaats van
// een int (4 bytes). Zie ook MidiMessage, dat MIDI-bytes als std::uint8_t opslaat.
enum class Waveform : std::uint8_t { Sine, Saw, Square, Triangle, Count };

std::string waveformName(Waveform waveform);
Waveform waveformFromName(const std::string& name);  // gooit std::invalid_argument

// Vereiste 4: 4.useful and correct inheritance (explain why)
// De vier golfvormen erven van Oscillator: ze hergebruiken next() en reset() (fase opschuiven) en
// voegen alleen hun eigen formule toe in sample(). Zonder overerving zou elke klasse die fase-code
// opnieuw moeten schrijven.
class SineOscillator final : public Oscillator {
 public:
  float sample(float phase) const override;
  std::string name() const override { return "Sine"; }
};

class SawOscillator final : public Oscillator {
 public:
  float sample(float phase) const override;
  std::string name() const override { return "Saw"; }
};

class SquareOscillator final : public Oscillator {
 public:
  float sample(float phase) const override;
  std::string name() const override { return "Square"; }
};

class TriangleOscillator final : public Oscillator {
 public:
  float sample(float phase) const override;
  std::string name() const override { return "Triangle"; }
};

// Maakt de juiste oscillator voor een golfvorm (Factory). Geeft nullptr bij een ongeldige waarde.
class OscillatorFactory {
 public:
  static std::unique_ptr<Oscillator> create(Waveform waveform);
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_WAVEFORMS_H

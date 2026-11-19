#ifndef MIDIPIANO_WAVEFORMS_H
#define MIDIPIANO_WAVEFORMS_H

#include "Oscillator.h"

namespace midipiano {

// Vereiste 4: 4.useful and correct inheritance (explain why)
// SineOscillator en SquareOscillator erven van Oscillator: ze hergebruiken de fase-logica uit next()
// en voegen alleen hun eigen vorm toe in sample(). Zonder overerving zou elke golfvorm die
// fase-code opnieuw moeten schrijven.
class SineOscillator final : public Oscillator {
 public:
  float sample(float phase) const override;
};

class SquareOscillator final : public Oscillator {
 public:
  float sample(float phase) const override;
};

}  // namespace midipiano

#endif  // MIDIPIANO_WAVEFORMS_H

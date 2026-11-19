#ifndef MIDIPIANO_DSP_OSCILLATOR_H
#define MIDIPIANO_DSP_OSCILLATOR_H

// Vereiste 13: 13.correct protections (for header files)
// Elke header in dit project begint met #ifndef/#define en eindigt met #endif (een include guard).
// Als twee headers allebei Oscillator.h includen, wordt de klasse toch maar een keer gedefinieerd.

#include <string>

namespace midipiano::dsp {

// Vereiste 7: 7.useful and correct base class
// Oscillator is de basisklasse van alle golfvormen. Ze bevat de code die voor elke vorm hetzelfde
// is: de fase bijhouden en per sample opschuiven met frequentie / samplerate (next() en reset()).
// De afgeleide klassen voegen alleen hun vorm toe, zodat die fase-logica maar op een plek staat.
class Oscillator {
 public:
  virtual ~Oscillator() = default;

  // Vereiste 9: 9.useful and correct virtual function
  // sample() is virtueel: Voice roept altijd hetzelfde next() aan, en tijdens het draaien kiest C++
  // de sample() van de gekozen golfvorm. Een nieuwe golfvorm toevoegen is dus een nieuwe klasse,
  // niet een extra if/else in Voice.
  virtual float sample(float phase) const = 0;
  virtual std::string name() const = 0;

  // Geeft het volgende sample en schuift de fase (0..1) op.
  float next(float frequencyHz, float sampleRate) {
    const float out = sample(phase_);
    phase_ += frequencyHz / sampleRate;
    if (phase_ >= 1.0f) phase_ -= static_cast<float>(static_cast<int>(phase_));
    return out;
  }

  void reset() { phase_ = 0.0f; }

 private:
  float phase_ = 0.0f;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_OSCILLATOR_H

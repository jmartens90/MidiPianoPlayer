#ifndef MIDIPIANO_OSCILLATOR_H
#define MIDIPIANO_OSCILLATOR_H

namespace midipiano {

// Vereiste 8: 8.useful and correct abstract base class
// Oscillator is een abstracte basisklasse: een "toon" bestaat niet zonder vorm (sinus, blok, ...).
// Ze legt vast wat elke oscillator moet kunnen (een volgend sample geven) en bevat de gedeelde
// fase-logica, zodat Voice met elke vorm kan werken zonder te weten welke het is.
class Oscillator {
 public:
  virtual ~Oscillator() = default;

  // Vereiste 9: 9.useful and correct virtual function
  // sample() is puur virtueel: elke golfvorm berekent zijn eigen waarde uit dezelfde fase (0..1).
  // Zo voegt een nieuwe golfvorm toevoegen alleen een nieuwe klasse toe, geen if/else in Voice.
  virtual float sample(float phase) const = 0;

  // Geeft het volgende sample en schuift de fase op. Gedeeld door alle golfvormen.
  float next(float frequencyHz, float sampleRate) {
    const float out = sample(phase_);
    phase_ += frequencyHz / sampleRate;
    if (phase_ >= 1.0f) phase_ -= 1.0f;
    return out;
  }

  void reset() { phase_ = 0.0f; }

 private:
  float phase_ = 0.0f;
};

}  // namespace midipiano

#endif  // MIDIPIANO_OSCILLATOR_H

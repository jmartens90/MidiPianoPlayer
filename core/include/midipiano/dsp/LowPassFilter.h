#ifndef MIDIPIANO_DSP_LOWPASSFILTER_H
#define MIDIPIANO_DSP_LOWPASSFILTER_H

#include <array>

namespace midipiano::dsp {

// Resonante 24 dB/octaaf low-pass filter in de stijl van een Moog-ladderfilter: vier een-polige
// trappen achter elkaar, met terugkoppeling van de uitgang naar de ingang (resonantie). Elke trap
// verzadigt (een snelle benadering van tanh), net als de transistors in een analoge filter. Met veel
// resonantie gaat de filter fluiten en zelf oscilleren; met drive duw je het signaal harder in die
// verzadiging (vuil, "nasty").
//
// De coefficienten (exp, deling) worden alleen herberekend als cutoff of samplerate verandert, niet
// bij elk sample: op een gsm scheelt dat veel rekenwerk.
class LowPassFilter {
 public:
  void setCutoff(float hz);
  float cutoff() const { return cutoffHz_; }
  void setResonance(float amount);  // 0 = geen, 1 = zelfoscillatie
  float resonance() const { return resonance_; }
  void setDrive(float drive);       // 1 = proper, 10 = zwaar vervormd
  float drive() const { return drive_; }

  // Gemoduleerde cutoff (filter-envelope), los van de ingestelde cutoff. Voice roept dit om de
  // 16 samples op.
  void setModulatedCutoff(float hz, float sampleRate);

  float process(float input, float sampleRate);
  void reset() { stage_.fill(0.0f); }

 private:
  void updateCoefficients(float hz, float sampleRate);

  float cutoffHz_ = 8000.0f;
  float resonance_ = 0.0f;
  float drive_ = 1.0f;
  // Huidige coefficienten en voor welke cutoff/samplerate ze gelden.
  float g_ = 0.0f;
  float k_ = 0.0f;
  float coeffHz_ = -1.0f;
  float coeffRate_ = -1.0f;
  float coeffResonance_ = -1.0f;
  std::array<float, 4> stage_{};
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_LOWPASSFILTER_H

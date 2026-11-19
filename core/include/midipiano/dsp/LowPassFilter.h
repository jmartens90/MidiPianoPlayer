#ifndef MIDIPIANO_DSP_LOWPASSFILTER_H
#define MIDIPIANO_DSP_LOWPASSFILTER_H

#include <array>

namespace midipiano::dsp {

// Resonante 24 dB/octaaf low-pass filter in de stijl van een Moog-ladderfilter: vier een-polige
// trappen achter elkaar, met terugkoppeling van de uitgang naar de ingang (resonantie). Elke trap
// verzadigt met tanh, net als de transistors in een analoge filter. Met veel resonantie gaat de filter
// fluiten en zelf oscilleren; met drive duw je het signaal harder in die verzadiging (vuil, "nasty").
class LowPassFilter {
 public:
  void setCutoff(float hz) { cutoffHz_ = hz; }
  float cutoff() const { return cutoffHz_; }
  void setResonance(float amount);  // 0 = geen, 1 = zelfoscillatie
  float resonance() const { return resonance_; }
  void setDrive(float drive);       // 1 = proper, 10 = zwaar vervormd
  float drive() const { return drive_; }

  float process(float input, float sampleRate) { return process(input, sampleRate, cutoffHz_); }
  float process(float input, float sampleRate, float cutoffHz);  // met gemoduleerde cutoff
  void reset() { stage_.fill(0.0f); }

 private:
  float cutoffHz_ = 8000.0f;
  float resonance_ = 0.0f;
  float drive_ = 1.0f;
  std::array<float, 4> stage_{};
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_LOWPASSFILTER_H

#ifndef MIDIPIANO_DSP_LOWPASSFILTER_H
#define MIDIPIANO_DSP_LOWPASSFILTER_H

namespace midipiano::dsp {

// Eenvoudige een-polige low-pass filter: dempt hoge tonen, zodat een zaagtand of blokgolf zachter
// en "warmer" klinkt naarmate de cutoff lager staat.
class LowPassFilter {
 public:
  void setCutoff(float hz) { cutoffHz_ = hz; }
  float cutoff() const { return cutoffHz_; }
  float process(float input, float sampleRate);
  void reset() { state_ = 0.0f; }

 private:
  float cutoffHz_ = 8000.0f;
  float state_ = 0.0f;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_LOWPASSFILTER_H

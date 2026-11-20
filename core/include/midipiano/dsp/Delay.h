#ifndef MIDIPIANO_DSP_DELAY_H
#define MIDIPIANO_DSP_DELAY_H

#include <cstddef>
#include <vector>

namespace midipiano::dsp {

// Echo (delay): het signaal komt na "time" seconden terug, en via de terugkoppeling (feedback) nog
// eens en nog eens, telkens zachter. In de terugkoppeling zit een high-cut (tone), zodat elke herhaling
// donkerder wordt, zoals bij een bandecho. Als je de tijd verdraait, glijdt de vertraging er zacht
// naartoe (geen klikken, wel het typische "tape"-toonhoogte-effect).
class Delay {
 public:
  static constexpr float kMaxTime = 2.0f;  // seconden

  struct Settings {
    float mix = 0.0f;        // 0 = uit, 1 = echo even luid als het origineel
    float time = 0.375f;     // seconden
    float feedback = 0.4f;   // 0..0,95: hoeveel van de echo opnieuw terugkomt
    float toneHz = 4000.0f;  // high cut in de terugkoppeling
  };

  explicit Delay(float sampleRate = 48000.0f);

  void setSampleRate(float sampleRate);
  void setSettings(const Settings& settings);
  const Settings& settings() const { return settings_; }
  void clear();

  void process(float* buffer, std::size_t frames);  // ter plaatse: droog + echo

 private:
  float sampleRate_ = 48000.0f;
  Settings settings_;
  std::vector<float> line_;
  std::size_t writePos_ = 0;
  float targetDelay_ = 0.0f;   // in samples
  float currentDelay_ = 0.0f;  // glijdt naar targetDelay_
  float toneCoeff_ = 1.0f;
  float toneState_ = 0.0f;
  bool running_ = false;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_DELAY_H

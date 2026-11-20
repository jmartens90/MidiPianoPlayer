#ifndef MIDIPIANO_DSP_REVERB_H
#define MIDIPIANO_DSP_REVERB_H

#include <array>
#include <cstddef>
#include <vector>

namespace midipiano::dsp {

// Stereo plaatgalm naar het ontwerp van Jon Dattorro ("Effect Design, Part 1", 1997), het klassieke
// algoritme van veel hardware-galmen. Het signaal loopt zo:
//
//   ingang -> predelay -> low cut -> vroege reflecties ----------------------------+
//                                 -> 4 diffusers (allpass) -> "tank": twee helften   |
//                                    die elkaar voeden, met trage modulatie en       |
//                                    demping -> 7 aftakkingen per kanaal -> high cut +-> L/R
//
// Waarom dit minder steriel klinkt dan Freeverb:
// - Stereo: links en rechts lezen op andere plaatsen in de tank, dus elk oor hoort een andere galm.
// - Modulatie: twee vertragingen in de tank wiebelen traag (een fractie van een ms). Zo blijven
//   er geen vaste "metalige" resonanties hangen in de staart.
// - Vroege reflecties: eerst een paar losse echo's van de "muren", dan pas de dichte staart; dat
//   geeft het oor een ruimte-gevoel.
class Reverb {
 public:
  struct Settings {
    float mix = 0.25f;        // 0 = droog, 1 = veel galm
    float predelay = 0.02f;   // seconden
    float decay = 0.6f;       // 0 = korte galm, 1 = heel lang
    float damping = 0.3f;     // hoe snel hoge tonen in de galm uitsterven
    float lowCutHz = 150.0f;  // pre-filter: high-pass op de ingang van de galm
    float highCutHz = 7000.0f;  // post-filter: low-pass op de galm
    float size = 0.6f;        // 0 = kleine kamer, 1 = grote zaal (lengte van alle vertragingen)
    float width = 1.0f;       // 0 = mono, 1 = breed stereo
  };

  explicit Reverb(float sampleRate = 48000.0f);

  void setSampleRate(float sampleRate);
  void setSettings(const Settings& settings);
  const Settings& settings() const { return settings_; }
  void setAmount(float mix);
  float amount() const { return settings_.mix; }
  void clear();

  // Mono in, stereo uit: left/right = droog signaal + galm. in mag gelijk zijn aan left of right.
  void process(const float* in, float* left, float* right, std::size_t frames);
  // Mono: bewerkt de buffer ter plaatse (gemiddelde van links en rechts).
  void process(float* buffer, std::size_t frames);

 private:
  // Vertragingslijn met een lengte die een macht van twee is (snel omlopen met een masker) en
  // lezen op een niet-gehele afstand (lineaire interpolatie, nodig voor de modulatie).
  class DelayLine {
   public:
    void allocate(std::size_t maxSamples);
    void clear();
    void write(float x) {
      pos_ = (pos_ + 1) & mask_;
      buffer_[pos_] = x;
    }
    float read(std::size_t delay) const { return buffer_[(pos_ - delay) & mask_]; }
    float readFrac(float delay) const;

   private:
    std::vector<float> buffer_{0.0f};
    std::size_t mask_ = 0;
    std::size_t pos_ = 0;
  };
  // Allpass: laat alle frequenties door maar smeert de fase uit = maakt echo's dichter.
  struct Allpass {
    DelayLine line;
    float process(float in, float delay, float gain);
  };

  void updateCoefficients();
  std::size_t scaled(float samplesAt29k, bool sized) const;

  float sampleRate_ = 48000.0f;
  Settings settings_;
  DelayLine predelay_;
  std::array<Allpass, 4> diffusers_;
  // De tank: per helft een gemoduleerde allpass, een vertraging, een allpass en nog een vertraging.
  Allpass modAllpass_[2];
  DelayLine delayA_[2];
  Allpass tankAllpass_[2];
  DelayLine delayB_[2];
  float tankFeedback_[2] = {0.0f, 0.0f};
  float dampState_[2] = {0.0f, 0.0f};
  float highCutState_[2] = {0.0f, 0.0f};
  float lowCutState_ = 0.0f;
  float lfoPhase_ = 0.0f;
  // Afgeleide waarden (in samples bij de huidige samplerate en grootte).
  std::size_t predelaySamples_ = 0;
  float diffuserDelay_[4] = {};
  float modDelay_[2] = {};
  std::size_t delayALength_[2] = {};
  float tankAllpassDelay_[2] = {};
  std::size_t delayBLength_[2] = {};
  std::array<std::size_t, 14> taps_{};
  std::array<std::size_t, 8> earlyTaps_{};
  float excursion_ = 0.0f;
  float decay_ = 0.5f;
  float lowCutCoeff_ = 0.0f;
  float highCutCoeff_ = 1.0f;
  float lfoStep_ = 0.0f;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_REVERB_H

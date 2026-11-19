#ifndef MIDIPIANO_DSP_REVERB_H
#define MIDIPIANO_DSP_REVERB_H

#include <cstddef>
#include <vector>

namespace midipiano::dsp {

// Galm volgens het Freeverb-ontwerp (Schroeder/Moorer). Het signaal loopt zo:
//
//   ingang -> predelay -> pre-filter (low cut) -> 8 kamfilters (met demping) -> 4 allpass
//          -> post-filter (high cut) -> gemengd met het droge signaal
//
// De kamfilters bootsen de vele weerkaatsingen in een zaal na, de allpass-filters maken die dichter
// zodat je geen losse echo's hoort. Predelay is de tijd tot de eerste weerkaatsing (grote zaal =
// langer). De low cut houdt de galm vrij van modderige bas, de high cut maakt hem donkerder.
class Reverb {
 public:
  struct Settings {
    float mix = 0.25f;        // 0 = droog, 1 = veel galm
    float predelay = 0.02f;   // seconden
    float decay = 0.6f;       // 0 = kleine kamer, 1 = kathedraal
    float damping = 0.3f;     // hoe snel hoge tonen in de galm uitsterven
    float lowCutHz = 150.0f;  // pre-filter: high-pass op de ingang van de galm
    float highCutHz = 7000.0f;  // post-filter: low-pass op de galm
  };

  explicit Reverb(float sampleRate = 48000.0f);

  void setSampleRate(float sampleRate);
  void setSettings(const Settings& settings);
  const Settings& settings() const { return settings_; }
  void setAmount(float mix);
  float amount() const { return settings_.mix; }
  void clear();

  // Bewerkt de buffer ter plaatse: droog signaal + galm.
  void process(float* buffer, std::size_t frames);

 private:
  // Vertragingslijn met terugkoppeling en een eenvoudig laagdoorlaatfilter (hoge tonen sterven
  // sneller uit, zoals in een echte ruimte).
  struct Comb {
    std::vector<float> line;
    std::size_t pos = 0;
    float store = 0.0f;
    float process(float in, float feedback, float damp);
  };
  struct Allpass {
    std::vector<float> line;
    std::size_t pos = 0;
    float process(float in);
  };

  void updateCoefficients();

  float sampleRate_ = 48000.0f;
  Settings settings_;
  std::vector<Comb> combs_;
  std::vector<Allpass> allpasses_;
  std::vector<float> predelayLine_;
  std::size_t predelayPos_ = 0;
  std::size_t predelaySamples_ = 0;
  float lowCutCoeff_ = 0.0f;   // een-polig, voor de high-pass
  float lowCutState_ = 0.0f;
  float highCutCoeff_ = 1.0f;  // een-polig low-pass
  float highCutState_ = 0.0f;
  float feedback_ = 0.84f;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_REVERB_H

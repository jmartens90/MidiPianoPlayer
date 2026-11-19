#ifndef MIDIPIANO_DSP_REVERB_H
#define MIDIPIANO_DSP_REVERB_H

#include <cstddef>
#include <vector>

namespace midipiano::dsp {

// Galm volgens het Freeverb-ontwerp (Schroeder/Moorer): acht parallelle kamfilters met demping
// bootsen de vele weerkaatsingen in een zaal na, vier allpass-filters erachter maken die
// weerkaatsingen dichter zodat je geen losse echo's hoort. Een droge sample-piano klinkt hiermee
// alsof hij in een kamer staat in plaats van rechtstreeks in je oor.
class Reverb {
 public:
  explicit Reverb(float sampleRate = 48000.0f);

  void setSampleRate(float sampleRate);
  void setAmount(float amount);  // 0 = droog, 1 = veel galm
  float amount() const { return amount_; }
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

  float amount_ = 0.0f;
  std::vector<Comb> combs_;
  std::vector<Allpass> allpasses_;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_REVERB_H

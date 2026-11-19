#ifndef MIDIPIANO_AUDIO_AUDIOOUTPUT_H
#define MIDIPIANO_AUDIO_AUDIOOUTPUT_H

#include <memory>
#include <stdexcept>

#include "midipiano/dsp/Synth.h"

struct ma_device;  // uit miniaudio.h; hier alleen vooraf gedeclareerd zodat de header klein blijft

namespace midipiano::audio {

class AudioError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Opent de geluidskaart en laat die de synth "afspelen": de geluidskaart vraagt telkens om een
// blok samples (op haar eigen audio-thread) en AudioOutput haalt die bij Synth::render().
class AudioOutput {
 public:
  explicit AudioOutput(dsp::Synth& synth);  // gooit AudioError als er geen geluidskaart is

  // Vereiste 24: 24.at least 2 destructors
  // De geluidskaart blijft anders de callback oproepen met een pointer naar een object dat niet meer
  // bestaat (crash). De destructor stopt en sluit het apparaat (ma_device_uninit) voor het geheugen
  // vrijkomt.
  ~AudioOutput();

  AudioOutput(const AudioOutput&) = delete;
  AudioOutput& operator=(const AudioOutput&) = delete;

  float sampleRate() const;
  unsigned int bufferFrames() const;

 private:
  static void dataCallback(ma_device* device, void* output, const void* input, unsigned int frameCount);

  dsp::Synth& synth_;
  std::unique_ptr<ma_device> device_;
};

}  // namespace midipiano::audio

#endif  // MIDIPIANO_AUDIO_AUDIOOUTPUT_H

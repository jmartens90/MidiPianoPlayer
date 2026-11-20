#ifndef MIDIPIANO_AUDIO_AUDIOOUTPUT_H
#define MIDIPIANO_AUDIO_AUDIOOUTPUT_H

#include <atomic>
#include <chrono>
#include <cstdint>
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
  // periodMs = grootte van een audioblok in ms (0 = automatisch: 10 ms op Android, anders zo klein
  // als het systeem toelaat). Gooit AudioError als er geen geluidskaart is.
  explicit AudioOutput(dsp::Synth& synth, unsigned int periodMs = 0);

  // Vereiste 24: 24.at least 2 destructors
  // De geluidskaart blijft anders de callback oproepen met een pointer naar een object dat niet meer
  // bestaat (crash). De destructor stopt en sluit het apparaat (ma_device_uninit) voor het geheugen
  // vrijkomt.
  ~AudioOutput();

  AudioOutput(const AudioOutput&) = delete;
  AudioOutput& operator=(const AudioOutput&) = delete;

  // Geluidskaart even stilleggen (app op de achtergrond) en weer starten.
  void pause();
  void resume();
  bool isPaused() const { return paused_; }

  // true als de geluidskaart al "limit" lang geen blok meer gevraagd heeft terwijl hij zou moeten
  // spelen. Op Android gebeurt dat na slaapstand: de AAudio-stream is dan stilletjes afgesloten.
  bool isStalled(std::chrono::milliseconds limit) const;

  float sampleRate() const;
  unsigned int bufferFrames() const;
  double blockMilliseconds() const { return 1000.0 * bufferFrames() / sampleRate(); }

 private:
  static void dataCallback(ma_device* device, void* output, const void* input, unsigned int frameCount);

  dsp::Synth& synth_;
  std::unique_ptr<ma_device> device_;
  bool paused_ = false;
  // Tijdstip (steady_clock, in ns) van de laatste callback, of van de start. Atomair: de
  // audio-thread schrijft, de GUI-thread leest.
  std::atomic<std::int64_t> lastCallbackNs_{0};
};

}  // namespace midipiano::audio

#endif  // MIDIPIANO_AUDIO_AUDIOOUTPUT_H

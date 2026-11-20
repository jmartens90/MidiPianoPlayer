#include "midipiano/audio/AudioOutput.h"

#include <miniaudio.h>

#include <string>

namespace midipiano::audio {

// Vereiste 58: 58.useful (non trivial) usage of an external library (not Qt)
// miniaudio praat met de geluidskaart op elk platform (WASAPI op Windows, Core Audio op macOS,
// PulseAudio/ALSA op Linux, AAudio op Android) met dezelfde code. We openen een playback-apparaat met
// lage latency, laten miniaudio de samplerate kiezen, en miniaudio roept vanaf zijn eigen
// audio-thread dataCallback() op telkens de geluidskaart een nieuw blok samples nodig heeft.
// Een tweede externe bibliotheek, TinySoundFont, speelt de pianosamples af (zie SoundFont.cpp).
AudioOutput::AudioOutput(dsp::Synth& synth) : synth_(synth), device_(std::make_unique<ma_device>()) {
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 1;
  config.sampleRate = 0;  // 0 = de eigen samplerate van de geluidskaart (meestal 48000)
  config.performanceProfile = ma_performance_profile_low_latency;
#ifdef __ANDROID__
  // Op een gsm mag de buffer iets groter zijn: 10 ms per blok geeft de CPU genoeg tijd voor een
  // volle akkoord met galm, zonder te kraken. Merkbare vertraging is dat nog niet.
  config.periodSizeInMilliseconds = 10;
#endif
  config.dataCallback = &AudioOutput::dataCallback;
  // Vereiste 28: 28.useful usage of "this" (if the code does not work without it)
  // miniaudio is een C-bibliotheek: de callback is een gewone (statische) functie zonder object.
  // We geven "this" mee als pUserData; in dataCallback() halen we daarmee ons eigen AudioOutput-
  // object terug. Zonder this weet de callback niet welke synth hij moet afspelen.
  config.pUserData = this;

  if (ma_device_init(nullptr, &config, device_.get()) != MA_SUCCESS) {
    throw AudioError("geen geluidskaart gevonden");
  }
  synth_.setSampleRate(static_cast<float>(device_->sampleRate));
  if (ma_device_start(device_.get()) != MA_SUCCESS) {
    ma_device_uninit(device_.get());
    throw AudioError("kan de geluidskaart niet starten");
  }
}

AudioOutput::~AudioOutput() { ma_device_uninit(device_.get()); }

void AudioOutput::pause() {
  if (paused_) return;
  ma_device_stop(device_.get());
  paused_ = true;
}

void AudioOutput::resume() {
  if (!paused_) return;
  if (ma_device_start(device_.get()) == MA_SUCCESS) paused_ = false;
}

float AudioOutput::sampleRate() const { return static_cast<float>(device_->sampleRate); }

unsigned int AudioOutput::bufferFrames() const { return device_->playback.internalPeriodSizeInFrames; }

void AudioOutput::dataCallback(ma_device* device, void* output, const void* /*input*/, unsigned int frameCount) {
  auto* self = static_cast<AudioOutput*>(device->pUserData);
  self->synth_.render(static_cast<float*>(output), frameCount);
}

}  // namespace midipiano::audio

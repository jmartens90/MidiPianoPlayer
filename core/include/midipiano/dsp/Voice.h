#ifndef MIDIPIANO_DSP_VOICE_H
#define MIDIPIANO_DSP_VOICE_H

#include <cstdint>
#include <memory>

#include "midipiano/dsp/Envelope.h"
#include "midipiano/dsp/LowPassFilter.h"
#include "midipiano/dsp/Modulation.h"
#include "midipiano/dsp/Oscillator.h"
#include "midipiano/dsp/Waveforms.h"

namespace midipiano::dsp {

// Een Voice speelt een noot: oscillator -> resonante filter (door de envelope gemoduleerd) -> envelope -> volume.
class Voice {
 public:
  Voice() : Voice(Waveform::Sine) {}
  explicit Voice(Waveform waveform);

  void start(std::uint8_t note, std::uint8_t velocity, std::uint64_t startedAt);
  void release();
  float render(float sampleRate);
  // Om de 16 samples (control rate): LFO-waarden + de eigen modulatie-envelope toepassen op toonhoogte,
  // filter, positie en volume. envTarget/envAmount: waar de modulatie-envelope aan draait.
  void control(const ModValues& lfo, ModTarget envTarget, float envAmount, float sampleRate);

  void setWaveform(Waveform waveform);
  Envelope& envelope() { return envelope_; }
  LowPassFilter& filter() { return filter_; }
  Envelope& modEnvelope() { return modEnvelope_; }
  void setResonance(float amount) { baseResonance_ = amount; filter_.setResonance(amount); }
  void setDrive(float drive) { baseDrive_ = drive; filter_.setDrive(drive); }
  void setFilterEnvAmount(float amount) { filterEnvAmount_ = amount; }
  void setWavetablePosition(float position);

  bool isActive() const { return !envelope_.isIdle(); }
  bool isHeld() const { return held_; }
  std::uint8_t note() const { return note_; }
  std::uint64_t startedAt() const { return startedAt_; }

 private:
  // Vereiste 6: 6.useful and correct object composition (explain why)
  // Een Voice *bestaat uit* een oscillator, een filter en een envelope (en *is* er geen van).
  // Elk onderdeel doet een ding en is apart te testen; Voice zet ze alleen in de juiste volgorde.
  // Daardoor kan de golfvorm tijdens het spelen wisselen zonder de envelope of filter te raken.
  //
  // Vereiste 40: 40.2 useful and correct smart pointers
  // Voice is de enige eigenaar van zijn oscillator, dus std::unique_ptr: bij een nieuwe golfvorm of
  // bij het verdwijnen van de Voice wordt de oude oscillator automatisch verwijderd, zonder delete.
  // (De tweede smart pointer: SynthController bezit AudioOutput en zijn MIDI-bron als unique_ptr.)
  std::unique_ptr<Oscillator> oscillator_;
  LowPassFilter filter_;
  Envelope envelope_;
  Envelope modEnvelope_{0.01f, 0.5f, 0.0f, 0.3f};  // tweede envelope, alleen voor modulatie

  // Vereiste 31: 31.useful member variabel
  // note_ onthoudt welke toets deze Voice speelt, zodat Synth::noteOff() bij het loslaten van C4
  // precies de Voice(s) met note_ == 60 kan vinden en loslaten.
  std::uint8_t note_ = 0;
  float filterEnvAmount_ = 0.0f;
  float wavetablePosition_ = 0.0f;
  float baseResonance_ = 0.0f;
  float baseDrive_ = 1.0f;
  float pitchFactor_ = 1.0f;  // door modulatie (vibrato)
  float ampFactor_ = 1.0f;    // door modulatie (tremolo)
  Waveform waveform_;
  float gain_ = 0.0f;
  float frequencyHz_ = 0.0f;
  std::uint64_t startedAt_ = 0;
  bool held_ = false;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_VOICE_H

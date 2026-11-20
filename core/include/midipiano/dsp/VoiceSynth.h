#ifndef MIDIPIANO_DSP_VOICESYNTH_H
#define MIDIPIANO_DSP_VOICESYNTH_H

#include <array>
#include <cstddef>
#include <cstdint>

#include "midipiano/dsp/Instrument.h"
#include "midipiano/dsp/Modulation.h"
#include "midipiano/dsp/Voice.h"

namespace midipiano::dsp {

// De subtractieve synth: 8 stemmen (oscillator -> filter -> envelope), ingesteld met een Patch.
class VoiceSynth final : public Instrument {
 public:
  static constexpr std::size_t kMaxVoices = 8;

  explicit VoiceSynth(float sampleRate);

  std::string name() const override { return "Synth"; }
  void noteOn(std::uint8_t note, std::uint8_t velocity) override;
  void noteOff(std::uint8_t note) override;
  void allNotesOff() override;
  void render(float* out, std::size_t frames) override;
  void setSampleRate(float sampleRate) override { sampleRate_ = sampleRate; }
  std::size_t activeVoiceCount() const override;

  bool usesPatch() const override { return true; }
  void applyPatch(const preset::Patch& patch) override;

 private:
  Voice* findFreeVoice();
  Voice& oldestVoice();

  // Vereiste 45: 45.useful container class
  // std::array<Voice, 8>: het aantal stemmen staat vast, dus geen vector die tijdens het spelen
  // kan herallocaten (dat zou in de audio-thread een hapering geven). Zie ook std::map in Patch en
  // std::vector<std::unique_ptr<Instrument>> in Synth.
  std::array<Voice, kMaxVoices> voices_;
  float sampleRate_;
  // Modulatie: twee LFO's voor alle stemmen samen, plus per stem een modulatie-envelope (in Voice).
  struct LfoSlot {
    Lfo lfo;
    ModTarget target = ModTarget::None;
    float amount = 0.0f;
    bool retrigger = false;  // elke nieuwe noot start de LFO opnieuw
  };
  std::array<LfoSlot, 2> lfos_;
  ModTarget envTarget_ = ModTarget::None;
  float envAmount_ = 0.0f;
  std::uint32_t controlCounter_ = 0;
  std::uint64_t clock_ = 0;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_VOICESYNTH_H

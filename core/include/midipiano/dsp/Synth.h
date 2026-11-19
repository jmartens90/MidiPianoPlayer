#ifndef MIDIPIANO_DSP_SYNTH_H
#define MIDIPIANO_DSP_SYNTH_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include "midipiano/dsp/Voice.h"
#include "midipiano/midi/MidiMessage.h"
#include "midipiano/preset/Patch.h"
#include "midipiano/util/RingBuffer.h"

namespace midipiano::dsp {

// Vereiste 2: 2.useful and correct abstraction (explain why)
// Synth verbergt alles wat nodig is om een noot te laten klinken (8 voices, stemtoewijzing,
// envelopes, filter, volume, de lock tussen threads). De rest van het programma kent alleen
// noteOn(), noteOff(), handle(MidiMessage) en render(). Het toetsenbord, de demo-sequencer en
// straks BLE-MIDI moeten dus niets weten van voices of samples.
//
// Thread-veilig: noteOn/noteOff/applyPatch komen van de GUI- of sequencer-thread, render() van de
// audio-thread. Een mutex beschermt de gedeelde toestand.
class Synth {
 public:
  static constexpr std::size_t kMaxVoices = 8;
  static constexpr std::size_t kScopeSize = 1024;

  explicit Synth(float sampleRate = 48000.0f);

  void handle(const midi::MidiMessage& message);

  // Vereiste 30: 30.default values in function definition
  // Een toets op het scherm heeft geen aanslagsterkte, dus velocity is standaard 100. De sequencer
  // en (straks) BLE-MIDI geven wel een eigen velocity mee.
  void noteOn(std::uint8_t note, std::uint8_t velocity = 100);
  void noteOff(std::uint8_t note);
  void allNotesOff();

  void render(float* out, std::size_t frames);
  void render(std::vector<float>& buffer);

  void setSampleRate(float sampleRate);
  float sampleRate() const;

  void applyPatch(const preset::Patch& patch);
  preset::Patch patch() const;

  void scopeSnapshot(std::vector<float>& out) const;
  std::size_t activeVoiceCount() const;

 private:
  Voice* findFreeVoice();
  Voice& oldestVoice();
  void applyPatchToVoice(Voice& voice);

  mutable std::mutex mutex_;

  // Vereiste 45: 45.useful container class
  // std::array<Voice, 8>: het aantal stemmen staat vast, dus geen vector die tijdens het spelen
  // kan herallocaten (dat zou in de audio-thread een hapering geven). Zie ook std::map in Patch.
  std::array<Voice, kMaxVoices> voices_;
  preset::Patch patch_;
  float sampleRate_;
  std::uint64_t clock_ = 0;
  util::RingBuffer<float> scope_{kScopeSize};
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_SYNTH_H

#ifndef MIDIPIANO_DSP_SYNTH_H
#define MIDIPIANO_DSP_SYNTH_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "midipiano/dsp/Instrument.h"
#include "midipiano/midi/MidiMessage.h"
#include "midipiano/preset/Patch.h"
#include "midipiano/util/RingBuffer.h"

namespace midipiano::dsp {

// Vereiste 2: 2.useful and correct abstraction (explain why)
// Synth verbergt alles wat nodig is om een noot te laten klinken: welke instrumenten er zijn
// (de synth of een piano uit samples), welk er gekozen is, de mix, het volume, de oscilloscoop en
// de lock tussen threads. De rest van het programma kent alleen noteOn(), noteOff(),
// handle(MidiMessage) en render(). Het toetsenbord, de demo-sequencer en straks BLE-MIDI moeten dus
// niets weten van stemmen, samples of TinySoundFont.
//
// Thread-veilig: noteOn/noteOff/applyPatch komen van de GUI- of sequencer-thread, render() van de
// audio-thread. Een mutex beschermt de gedeelde toestand.
class Synth {
 public:
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

  std::size_t addInstrument(std::unique_ptr<Instrument> instrument);
  void selectInstrument(std::size_t index);
  std::size_t currentInstrument() const;
  std::vector<std::string> instrumentNames() const;
  bool currentUsesPatch() const;

  void applyPatch(const preset::Patch& patch);
  preset::Patch patch() const;

  void scopeSnapshot(std::vector<float>& out) const;
  std::size_t activeVoiceCount() const;

 private:
  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<Instrument>> instruments_;
  std::size_t current_ = 0;
  preset::Patch patch_;
  float sampleRate_;
  std::vector<float> scratch_;
  util::RingBuffer<float> scope_{kScopeSize};
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_SYNTH_H

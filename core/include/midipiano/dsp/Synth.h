#ifndef MIDIPIANO_DSP_SYNTH_H
#define MIDIPIANO_DSP_SYNTH_H

#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "midipiano/dsp/Delay.h"
#include "midipiano/dsp/Instrument.h"
#include "midipiano/dsp/Reverb.h"
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

  using Clock = std::chrono::steady_clock;

  explicit Synth(float sampleRate = 48000.0f);

  void handle(const midi::MidiMessage& message);
  // Plant een bericht op een exact tijdstip: render() past het toe op het juiste sample van het blok
  // dat dat tijdstip bevat. Voor bronnen die vooruit weten wanneer een noot moet klinken.
  void schedule(const midi::MidiMessage& message, Clock::time_point when);

  // Vereiste 30: 30.default values in function definition
  // Een toets op het scherm heeft geen aanslagsterkte, dus velocity is standaard 100. De sequencer
  // en (straks) BLE-MIDI geven wel een eigen velocity mee.
  void noteOn(std::uint8_t note, std::uint8_t velocity = 100);
  void noteOff(std::uint8_t note);
  void allNotesOff();

  // Sustainpedaal (MIDI CC 64): zolang het ingedrukt is, blijven losgelaten noten klinken. Bij het
  // loslaten van het pedaal stoppen ze allemaal, behalve de toetsen die je nog vasthoudt.
  void setSustain(bool down);
  bool sustain() const;

  // Tempo (van de step sequencer): de echo kan erop synchroniseren (delay_sync, delay_beats in tellen).
  void setTempo(float bpm);

  void render(float* out, std::size_t frames);
  void render(float* out, std::size_t frames, Clock::time_point blockStart);
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
  // *Locked: de mutex is al vast (vanuit render() bij geplande berichten, of vanuit de publieke versie).
  void handleLocked(const midi::MidiMessage& message);
  void noteOnLocked(std::uint8_t note, std::uint8_t velocity);
  void noteOffLocked(std::uint8_t note);
  void allNotesOffLocked();
  void setSustainLocked(bool down);
  void renderInstruments(float* out, std::size_t frames);

  struct Scheduled {
    Clock::time_point when;
    std::uint64_t order;  // bij gelijke tijd: in volgorde van aankomst
    midi::MidiMessage message;
    bool operator>(const Scheduled& other) const {
      return when != other.when ? when > other.when : order > other.order;
    }
  };

  void applyReverbSettings();  // patch_ -> reverb_ (mutex moet al vast zijn)
  void applyDelaySettings();   // patch_ + tempo_ -> delay_

  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<Instrument>> instruments_;
  std::size_t current_ = 0;
  preset::Patch patch_;
  float sampleRate_;
  std::vector<float> scratch_;
  Delay delay_;
  Reverb reverb_;
  float tempo_ = 120.0f;
  std::priority_queue<Scheduled, std::vector<Scheduled>, std::greater<Scheduled>> scheduled_;
  std::uint64_t scheduleCounter_ = 0;
  bool sustain_ = false;
  std::bitset<128> sustained_;  // noten die losgelaten zijn maar door het pedaal blijven klinken
  util::RingBuffer<float> scope_{kScopeSize};
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_SYNTH_H

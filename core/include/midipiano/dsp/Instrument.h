#ifndef MIDIPIANO_DSP_INSTRUMENT_H
#define MIDIPIANO_DSP_INSTRUMENT_H

#include <cstddef>
#include <cstdint>
#include <string>

#include "midipiano/preset/Patch.h"

namespace midipiano::dsp {

// Iets dat noten omzet in geluid. Er zijn twee soorten: VoiceSynth (zelf berekende golfvormen) en
// SoundFontInstrument (opgenomen samples, bv. een vleugel). Synth werkt alleen met deze interface,
// dus het toetsenbord, de sequencer en straks BLE werken met elk instrument zonder aanpassing.
class Instrument {
 public:
  virtual ~Instrument() = default;

  virtual std::string name() const = 0;
  virtual void noteOn(std::uint8_t note, std::uint8_t velocity) = 0;
  virtual void noteOff(std::uint8_t note) = 0;
  virtual void allNotesOff() = 0;
  // Schrijft frames samples in out (overschrijft, telt niet op).
  virtual void render(float* out, std::size_t frames) = 0;
  virtual void setSampleRate(float sampleRate) = 0;
  virtual std::size_t activeVoiceCount() const = 0;

  // Alleen de synth gebruikt golfvorm, ADSR en cutoff; samples hebben hun eigen klank.
  virtual bool usesPatch() const { return false; }
  virtual void applyPatch(const preset::Patch& /*patch*/) {}
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_INSTRUMENT_H

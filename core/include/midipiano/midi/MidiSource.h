#ifndef MIDIPIANO_MIDI_MIDISOURCE_H
#define MIDIPIANO_MIDI_MIDISOURCE_H

#include <functional>
#include <string>

#include "midipiano/midi/MidiMessage.h"

namespace midipiano::midi {

// Vereiste 8: 8.useful and correct abstract base class
// MidiSource is "iets dat MIDI-noten aanlevert". Er bestaat geen algemene bron: het is altijd een
// concrete, zoals de DemoSequencer of de BleMidiSource (de PSoC 6 via Bluetooth, in app/). Daarom is
// de klasse abstract (alleen pure virtuele functies). SynthController geeft beide bronnen dezelfde
// callback; de synth merkt niet of een noot uit de demo of van de hardware komt.
class MidiSource {
 public:
  using Callback = std::function<void(const MidiMessage&)>;

  virtual ~MidiSource() = default;
  virtual void start(Callback onMessage) = 0;
  virtual void stop() = 0;
  virtual bool isRunning() const = 0;
  virtual std::string name() const = 0;
};

}  // namespace midipiano::midi

#endif  // MIDIPIANO_MIDI_MIDISOURCE_H

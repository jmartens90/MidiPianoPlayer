#include "midipiano/midi/MidiMessage.h"

namespace midipiano::midi {

MidiMessage MidiMessage::noteOn(std::uint8_t channel, std::uint8_t note, std::uint8_t velocity) {
  return MidiMessage(static_cast<std::uint8_t>(0x90 | (channel & 0x0F)), note & 0x7F, velocity & 0x7F);
}

MidiMessage MidiMessage::noteOff(std::uint8_t channel, std::uint8_t note) {
  return MidiMessage(static_cast<std::uint8_t>(0x80 | (channel & 0x0F)), note & 0x7F, 0);
}

std::ostream& operator<<(std::ostream& os, const MidiMessage& message) {
  const int channel = message.channel() + 1;  // muzikanten tellen kanalen vanaf 1
  if (message.isNoteOn()) {
    os << "NoteOn ch" << channel << " note " << int(message.note()) << " vel " << int(message.velocity());
  } else if (message.isNoteOff()) {
    os << "NoteOff ch" << channel << " note " << int(message.note());
  } else {
    os << "Midi 0x" << std::hex << int(message.status()) << std::dec << ' ' << int(message.note()) << ' '
       << int(message.velocity());
  }
  return os;
}

}  // namespace midipiano::midi

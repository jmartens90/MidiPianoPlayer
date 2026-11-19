#ifndef MIDIPIANO_MIDI_BLEMIDIPARSER_H
#define MIDIPIANO_MIDI_BLEMIDIPARSER_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "midipiano/midi/MidiMessage.h"

namespace midipiano::midi {

// Zet BLE-MIDI-pakketten om in MidiMessages. Dit is het formaat dat een Bluetooth-MIDI-toestel (zoals
// de PSoC 6) stuurt, volgens de "MIDI over Bluetooth Low Energy"-specificatie:
//
//   [header] [timestamp] [status] [data] [data] [timestamp] [status] [data] ...
//
// - header: bit 7 = 1, bit 6 = 0, bits 5..0 = bovenste bits van de tijdstempel
// - elk bericht begint met een tijdstempel-byte (bit 7 = 1) gevolgd door een statusbyte (bit 7 = 1)
// - "running status": een volgend bericht met dezelfde status mag de statusbyte (en zelfs de
//   tijdstempel) weglaten, dan volgen er gewoon databytes
// - System Exclusive (F0 ... F7) mag over meerdere pakketten lopen; die slaan we over
//
// De parser kent Qt niet en is daardoor zonder Bluetooth te testen (zie tests/BleMidiTests.cpp).
// Hij onthoudt de running status tussen pakketten, zoals de specificatie vraagt.
class BleMidiParser {
 public:
  std::vector<MidiMessage> parse(const std::uint8_t* data, std::size_t length);
  std::vector<MidiMessage> parse(const std::vector<std::uint8_t>& packet) {
    return parse(packet.data(), packet.size());
  }
  void reset();

 private:
  static std::size_t dataLength(std::uint8_t status);

  std::uint8_t runningStatus_ = 0;
  std::uint8_t data_[2] = {0, 0};
  std::size_t dataCount_ = 0;
  bool inSysEx_ = false;
};

}  // namespace midipiano::midi

#endif  // MIDIPIANO_MIDI_BLEMIDIPARSER_H

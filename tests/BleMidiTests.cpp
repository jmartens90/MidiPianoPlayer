#include <vector>

#include "TestFramework.h"
#include "midipiano/midi/BleMidiParser.h"

using namespace midipiano;
using Packet = std::vector<std::uint8_t>;

// Pakketten zoals een BLE-MIDI-toestel (de PSoC 6) ze stuurt: header, tijdstempel, status, data.
TEST(ble_parser_reads_a_single_note_on) {
  midi::BleMidiParser parser;
  const auto messages = parser.parse(Packet{0x80, 0x80, 0x90, 60, 100});
  CHECK(messages.size() == 1);
  CHECK(messages[0] == midi::MidiMessage::noteOn(0, 60, 100));
}

TEST(ble_parser_handles_several_messages_and_running_status) {
  midi::BleMidiParser parser;
  // Note On C4, dan met tijdstempel + running status E4, dan zonder tijdstempel G4, dan Note Off C4.
  const auto messages =
      parser.parse(Packet{0x80, 0x81, 0x90, 60, 100, 0x82, 64, 90, 67, 80, 0x83, 0x80, 60, 0});
  CHECK(messages.size() == 4);
  CHECK(messages[1] == midi::MidiMessage::noteOn(0, 64, 90));
  CHECK(messages[2] == midi::MidiMessage::noteOn(0, 67, 80));
  CHECK(messages[3].isNoteOff());
}

TEST(ble_parser_keeps_running_status_between_packets) {
  midi::BleMidiParser parser;
  parser.parse(Packet{0x80, 0x80, 0x93, 60, 100});  // kanaal 4
  const auto messages = parser.parse(Packet{0x80, 0x80, 62, 100});
  CHECK(messages.size() == 1);
  CHECK(messages[0] == midi::MidiMessage::noteOn(3, 62, 100));
}

TEST(ble_parser_skips_realtime_sysex_and_bad_packets) {
  midi::BleMidiParser parser;
  // Clock (F8) en een SysEx tussen de noten mogen de noten niet verstoren.
  const auto messages = parser.parse(
      Packet{0x80, 0x80, 0xF8, 0x80, 0xF0, 0x7E, 0x01, 0x02, 0x81, 0xF7, 0x82, 0xB0, 64, 127, 0x83, 0xC0, 5});
  CHECK(messages.size() == 2);
  CHECK(messages[0] == midi::MidiMessage(0xB0, 64, 127));  // sustainpedaal in
  CHECK(messages[1] == midi::MidiMessage(0xC0, 5, 0));     // program change: 1 databyte
  CHECK(parser.parse(Packet{0x00, 0x90, 60, 100}).empty());  // geen geldige header
  CHECK(parser.parse(Packet{}).empty());
}

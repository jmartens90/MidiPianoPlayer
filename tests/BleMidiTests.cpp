#include <vector>

#include "TestFramework.h"
#include <chrono>
#include <cmath>

#include "midipiano/midi/BleClockSync.h"
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

TEST(ble_parser_reads_timestamps_and_wraparound) {
  midi::BleMidiParser parser;
  // Header 0x81 = hoge bits 1. Tijdstempels: laag 0x7E (=126) en daarna laag 0x02 -> rondgelopen.
  const auto messages = parser.parseTimed(Packet{0x81, 0xFE, 0x90, 60, 100, 0x82, 0x80, 60, 0}.data(), 9);
  CHECK(messages.size() == 2);
  CHECK(messages[0].timestampMs == (1 << 7) + 126);
  CHECK(messages[1].timestampMs == (2 << 7) + 2);  // 4 ms later, over de 128-grens
}

TEST(ble_clock_sync_removes_bluetooth_jitter) {
  midi::BleClockSync sync;
  using namespace std::chrono;
  const auto t0 = midi::BleClockSync::Clock::now();
  // De PSoC speelt om de 10 ms een noot (tijdstempels 8180, 8190, 8, 18: rondloop na 8191), maar
  // Bluetooth levert ze met 7 tot 22 ms vertraging af.
  const int stamps[4] = {8180, 8190, 8, 18};
  const int delays[4] = {7, 22, 9, 15};
  midi::BleClockSync::Clock::time_point local[4];
  for (int i = 0; i < 4; ++i)
    local[i] = sync.toLocal(stamps[i], t0 + milliseconds(i * 10 + delays[i]));
  // Na het eerste pakket (7 ms, het snelste) liggen de lokale tijden weer precies 10 ms uit elkaar.
  for (int i = 1; i < 4; ++i) {
    const double gap = duration<double, std::milli>(local[i] - local[i - 1]).count();
    CHECK(std::abs(gap - 10.0) < 0.5);
  }
}

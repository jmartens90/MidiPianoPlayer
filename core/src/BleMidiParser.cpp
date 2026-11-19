#include "midipiano/midi/BleMidiParser.h"

namespace midipiano::midi {

std::size_t BleMidiParser::dataLength(std::uint8_t status) {
  switch (status & 0xF0) {
    case 0xC0:  // Program Change
    case 0xD0:  // Channel Pressure
      return 1;
    case 0xF0:
      if (status == 0xF1 || status == 0xF3) return 1;  // MTC quarter frame, Song Select
      if (status == 0xF2) return 2;                     // Song Position
      return 0;                                         // Tune Request, realtime, ...
    default:  // Note Off/On, Aftertouch, Control Change, Pitch Bend
      return 2;
  }
}

void BleMidiParser::reset() {
  runningStatus_ = 0;
  dataCount_ = 0;
  inSysEx_ = false;
}

std::vector<MidiMessage> BleMidiParser::parse(const std::uint8_t* data, std::size_t length) {
  std::vector<MidiMessage> messages;
  // Een geldig pakket heeft minstens een header en begint met 10xxxxxx.
  if (data == nullptr || length < 2 || (data[0] & 0xC0) != 0x80) return messages;

  // Na de header (en na elk volledig bericht) is een byte met bit 7 = 1 een tijdstempel.
  bool expectTimestamp = true;
  for (std::size_t i = 1; i < length; ++i) {
    const std::uint8_t byte = data[i];

    if (inSysEx_) {
      // SysEx overslaan tot F7. Voor F7 staat nog een tijdstempel-byte; realtime-bytes mogen ertussen.
      if (byte == 0xF7) {
        inSysEx_ = false;
        expectTimestamp = true;
      }
      continue;
    }

    if (byte & 0x80) {
      if (expectTimestamp) {  // tijdstempel: we spelen alles meteen af, dus enkel overslaan
        expectTimestamp = false;
        continue;
      }
      if (byte >= 0xF8) {  // realtime (clock, start, stop): 1 byte, raakt running status niet
        expectTimestamp = true;
        continue;
      }
      if (byte == 0xF0) {
        inSysEx_ = true;
        runningStatus_ = 0;
        continue;
      }
      runningStatus_ = byte;
      dataCount_ = 0;
      if (dataLength(byte) == 0) {  // bv. Tune Request: geen data, niets voor de synth
        if (byte >= 0xF0) runningStatus_ = 0;
        expectTimestamp = true;
      }
      continue;
    }

    // Databyte (bit 7 = 0): hoort bij de laatste status (eventueel via running status).
    if (runningStatus_ == 0) continue;  // data zonder status: ongeldig, negeren
    data_[dataCount_++] = byte;
    if (dataCount_ < dataLength(runningStatus_)) continue;

    if (runningStatus_ < 0xF0) {  // alleen kanaalberichten gaan naar de synth
      messages.emplace_back(runningStatus_, data_[0], dataCount_ > 1 ? data_[1] : std::uint8_t{0});
    } else {
      runningStatus_ = 0;  // system common: geen running status
    }
    dataCount_ = 0;
    expectTimestamp = true;
  }
  return messages;
}

}  // namespace midipiano::midi

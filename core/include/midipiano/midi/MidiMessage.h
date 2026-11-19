#ifndef MIDIPIANO_MIDI_MIDIMESSAGE_H
#define MIDIPIANO_MIDI_MIDIMESSAGE_H

#include <cstdint>
#include <ostream>

namespace midipiano::midi {

// Een MIDI-bericht van drie bytes, bv. "Note On, kanaal 1, noot 60, velocity 100" = 0x90 60 100.
// Alles wat noten maakt (schermtoetsenbord, demo-sequencer, straks de PSoC via BLE) stuurt een
// MidiMessage naar de synth, zodat er maar een weg is waarlangs noten binnenkomen.
class MidiMessage {
 public:
  // Vereiste 21: 21.at least 2 default constructors
  // Een leeg bericht (alle bytes 0) is nodig om bv. een array of buffer van berichten te kunnen maken
  // die pas later wordt ingevuld.
  MidiMessage() : status_(0), data1_(0), data2_(0) {}

  // Vereiste 22: 22.at least 2 parameterized constructors
  // Maakt een bericht rechtstreeks uit de drie bytes zoals ze over de kabel of via BLE binnenkomen.
  // Vereiste 25: 25.member initialization in constructors (the stuff behind a colon)
  // De drie bytes worden in de initializer list gezet: ze krijgen meteen hun waarde in plaats van
  // eerst 0 en daarna een toekenning in de body.
  MidiMessage(std::uint8_t status, std::uint8_t data1, std::uint8_t data2)
      : status_(status), data1_(data1), data2_(data2) {}

  // Vereiste 23: 23.at least 2 copy constructors
  // De sequencer-thread maakt een bericht en geeft een kopie door aan de synth; de kopie moet los
  // staan van het origineel, dat de sequencer meteen hergebruikt voor de volgende stap.
  MidiMessage(const MidiMessage& other)
      : status_(other.status_), data1_(other.data1_), data2_(other.data2_) {}
  MidiMessage& operator=(const MidiMessage& other) = default;

  static MidiMessage noteOn(std::uint8_t channel, std::uint8_t note, std::uint8_t velocity);
  static MidiMessage noteOff(std::uint8_t channel, std::uint8_t note);

  // Volgens de MIDI-standaard is "Note On met velocity 0" ook een Note Off.
  bool isNoteOn() const { return type() == 0x90 && data2_ > 0; }
  bool isNoteOff() const { return type() == 0x80 || (type() == 0x90 && data2_ == 0); }

  std::uint8_t type() const { return status_ & 0xF0; }
  std::uint8_t channel() const { return status_ & 0x0F; }
  std::uint8_t note() const { return data1_; }
  std::uint8_t velocity() const { return data2_; }
  std::uint8_t status() const { return status_; }

  bool operator==(const MidiMessage& other) const {
    return status_ == other.status_ && data1_ == other.data1_ && data2_ == other.data2_;
  }

  // Schrijft het bericht leesbaar uit, bv. "NoteOn ch1 note 60 vel 100" (handig in logs en tests).
  friend std::ostream& operator<<(std::ostream& os, const MidiMessage& message);

 private:
  // Vereiste 37: 37.2 useful unsigned chars or other better usage of memory efficient type
  // Een MIDI-byte is altijd 0..255, dus std::uint8_t (unsigned char) in plaats van int: een bericht
  // is zo 3 bytes in plaats van 12, en het past precies bij de bytes die de PSoC straks via BLE stuurt.
  std::uint8_t status_;
  std::uint8_t data1_;
  std::uint8_t data2_;
};

}  // namespace midipiano::midi

#endif  // MIDIPIANO_MIDI_MIDIMESSAGE_H

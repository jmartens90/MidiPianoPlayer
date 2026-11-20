#ifndef MIDIPIANO_MIDI_BLECLOCKSYNC_H
#define MIDIPIANO_MIDI_BLECLOCKSYNC_H

#include <chrono>
#include <cstdint>

namespace midipiano::midi {

// Zet BLE-MIDI-tijdstempels (de klok van de zender, in ms, 13 bits) om naar de klok van de gsm.
//
// Waarom: Bluetooth stuurt pakketten alleen op vaste "connection events" (om de 7,5 tot 30 ms). Twee
// noten die de PSoC 10 ms na elkaar speelt, kunnen dus samen of juist 30 ms uit elkaar aankomen. Met
// de tijdstempel weten we wanneer de noot echt gespeeld werd. We schatten het verschil tussen beide
// klokken met het snelste pakket (het minimum van aankomst - tijdstempel): dat pakket had het minst
// vertraging. Elke noot krijgt zo een tijd met dezelfde vaste vertraging, en het ritme blijft strak.
class BleClockSync {
 public:
  using Clock = std::chrono::steady_clock;

  // Geeft het lokale tijdstip waarop de noot (volgens de zender) gespeeld werd, uitgedrukt als het
  // vroegste moment waarop zo'n pakket kan aankomen.
  Clock::time_point toLocal(int timestampMs, Clock::time_point arrival);
  void reset() { synced_ = false; }

 private:
  bool synced_ = false;
  int lastTimestamp_ = 0;
  std::int64_t unwrappedMs_ = 0;      // tijdstempel zonder rondlopen
  double offsetMs_ = 0.0;             // lokale ms - zender-ms (minimum, kruipt langzaam omhoog)
  Clock::time_point lastArrival_{};
};

}  // namespace midipiano::midi

#endif  // MIDIPIANO_MIDI_BLECLOCKSYNC_H

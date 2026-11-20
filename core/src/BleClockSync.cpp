#include "midipiano/midi/BleClockSync.h"

#include <cmath>

namespace midipiano::midi {

BleClockSync::Clock::time_point BleClockSync::toLocal(int timestampMs, Clock::time_point arrival) {
  const double arrivalMs = std::chrono::duration<double, std::milli>(arrival.time_since_epoch()).count();
  timestampMs &= 0x1FFF;

  // Na een lange pauze (of bij de eerste noot) opnieuw synchroniseren.
  if (!synced_ || arrival - lastArrival_ > std::chrono::seconds(4)) {
    synced_ = true;
    lastTimestamp_ = timestampMs;
    unwrappedMs_ = timestampMs;
    offsetMs_ = arrivalMs - static_cast<double>(unwrappedMs_);
  } else {
    // Verschil met de vorige tijdstempel, rekening houdend met rondlopen na 8192 ms.
    int delta = (timestampMs - lastTimestamp_) & 0x1FFF;
    if (delta > 4096) delta -= 8192;  // iets ouder dan het vorige bericht
    lastTimestamp_ = timestampMs;
    unwrappedMs_ += delta;

    const double sample = arrivalMs - static_cast<double>(unwrappedMs_);
    if (sample < offsetMs_) {
      offsetMs_ = sample;  // sneller pakket gezien: dat is de betere schatting
    } else {
      offsetMs_ += (sample - offsetMs_) * 0.002;  // klokken drijven wat uit elkaar: langzaam volgen
    }
    if (sample - offsetMs_ > 500.0) offsetMs_ = sample;  // onwaarschijnlijk: opnieuw beginnen
  }
  lastArrival_ = arrival;

  const double localMs = static_cast<double>(unwrappedMs_) + offsetMs_;
  return Clock::time_point(std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double, std::milli>(localMs)));
}

}  // namespace midipiano::midi

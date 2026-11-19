#ifndef MIDIPIANO_VOICE_H
#define MIDIPIANO_VOICE_H

#include <memory>

#include "Oscillator.h"

namespace midipiano {

// Een Voice is een spelende noot: een oscillator plus toonhoogte en volume.
class Voice {
 public:
  explicit Voice(std::unique_ptr<Oscillator> oscillator);

  void noteOn(int midiNote);
  void noteOff();
  float render(float sampleRate);

  // Vereiste 3: 3.useful and correct encapsulation (explain why)
  // volume_ is private en kan alleen via setVolume() veranderen, dat de waarde tussen 0 en 1 houdt.
  // Zo kan niemand van buitenaf een volume van bv. 5 instellen, wat de WAV-uitvoer zou laten clippen.
  void setVolume(float volume);
  float volume() const { return volume_; }
  bool isActive() const { return active_; }

 private:
  // Vereiste 6: 6.useful and correct object composition (explain why)
  // Een Voice *heeft* een Oscillator (en is er geen). Daardoor kan dezelfde Voice-klasse een sinus-
  // of blokgolf spelen: de klank wordt bij het aanmaken gekozen door een andere oscillator mee te geven.
  std::unique_ptr<Oscillator> oscillator_;
  float frequencyHz_ = 0.0f;
  float volume_ = 0.5f;
  bool active_ = false;
};

}  // namespace midipiano

#endif  // MIDIPIANO_VOICE_H

#ifndef MIDIPIANO_DSP_ENVELOPE_H
#define MIDIPIANO_DSP_ENVELOPE_H

#include <cstdint>

namespace midipiano::dsp {

// ADSR-envelope: hoe het volume van een noot verloopt.
// Attack (aanzwellen), Decay (zakken), Sustain (niveau zolang de toets ingedrukt is), Release
// (uitsterven na het loslaten). Zonder envelope klikt elke noot hard aan en uit.
class Envelope {
 public:
  enum class Stage : std::uint8_t { Idle, Attack, Decay, Sustain, Release };

  // Vereiste 26: 26.constructor forwarding
  // De default constructor schrijft de standaardwaarden niet opnieuw uit, maar stuurt door naar de
  // constructor met parameters. Zo staat de initialisatie-logica op een plek.
  // Vereiste 21: 21.at least 2 default constructors
  // Envelope() geeft een bruikbare piano-achtige envelope, zodat elke Voice zonder extra code klinkt.
  Envelope() : Envelope(0.01f, 0.3f, 0.6f, 0.4f) {}

  // Vereiste 22: 22.at least 2 parameterized constructors
  // Met deze constructor maak je meteen een envelope met een eigen karakter, bv. een trage pad.
  Envelope(float attackS, float decayS, float sustainLevel, float releaseS);

  void noteOn();
  void noteOff();
  float next(float sampleRate);  // volgende versterking 0..1

  void setAttack(float seconds);
  void setDecay(float seconds);
  void setSustain(float level);
  void setRelease(float seconds);
  float attack() const { return attack_; }
  float decay() const { return decay_; }
  float sustain() const { return sustain_; }
  float release() const { return release_; }

  Stage stage() const { return stage_; }
  bool isIdle() const { return stage_ == Stage::Idle; }
  float level() const { return level_; }

 private:
  float attack_;
  float decay_;
  float sustain_;
  float release_;
  Stage stage_ = Stage::Idle;
  float level_ = 0.0f;
  float releaseStep_ = 0.0f;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_ENVELOPE_H

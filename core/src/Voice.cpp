#include "midipiano/dsp/Voice.h"

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

// Vereiste 25: 25.member initialization in constructors (the stuff behind a colon)
// De oscillator wordt in de initializer list aangemaakt: een unique_ptr kan je niet "eerst leeg
// maken en dan toekennen" zonder extra werk, en zo is de Voice meteen na de constructor speelbaar.
Voice::Voice(Waveform waveform) : oscillator_(OscillatorFactory::create(waveform)), waveform_(waveform) {}

void Voice::start(std::uint8_t note, std::uint8_t velocity, std::uint64_t startedAt) {
  note_ = note;
  gain_ = static_cast<float>(velocity) / 127.0f;
  frequencyHz_ = util::midiToHz(note);
  startedAt_ = startedAt;
  held_ = true;
  if (envelope_.isIdle()) {
    oscillator_->reset();
    filter_.reset();
  }
  envelope_.noteOn();
}

void Voice::release() {
  held_ = false;
  envelope_.noteOff();
}

float Voice::render(float sampleRate) {
  if (!isActive()) return 0.0f;
  // Vereiste 5: 5.useful and correct polymorphism (explain why)
  // oscillator_ is een Oscillator*, maar wijst naar een Sine-, Saw-, Square- of TriangleOscillator.
  // next() roept de virtuele sample() op, en C++ kiest tijdens het draaien de juiste golfvorm.
  // Voice hoeft dus niet te weten welke golfvorm de gebruiker koos, en kan er tijdens het spelen
  // van wisselen.
  const float raw = oscillator_->next(frequencyHz_, sampleRate);
  return filter_.process(raw, sampleRate) * envelope_.next(sampleRate) * gain_;
}

void Voice::setWaveform(Waveform waveform) {
  if (waveform == waveform_) return;  // geen nieuwe oscillator bij elke draai aan een knop
  auto oscillator = OscillatorFactory::create(waveform);
  if (!oscillator) return;
  oscillator_ = std::move(oscillator);  // de oude oscillator wordt automatisch verwijderd
  waveform_ = waveform;
}

}  // namespace midipiano::dsp

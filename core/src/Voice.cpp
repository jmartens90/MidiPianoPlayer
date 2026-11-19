#include "midipiano/dsp/Voice.h"

#include <cmath>

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
  // oscillator_ wijst naar een Sine-, Saw-, Square- of TriangleOscillator; next() roept de virtuele
  // sample() op, dus Voice hoeft niet te weten welke golfvorm de gebruiker koos.
  const float raw = oscillator_->next(frequencyHz_, sampleRate);
  const float level = envelope_.next(sampleRate);
  // Filter-envelope: de envelope opent de filter mee, tot 5 octaven boven de cutoff. Met veel
  // resonantie geeft dat het typische "acid"-gekwaak. Om de 16 samples is snel genoeg voor het oor
  // en spaart een exp2() per sample uit.
  if ((controlCounter_++ & 15u) == 0)
    filter_.setModulatedCutoff(filter_.cutoff() * std::exp2(filterEnvAmount_ * 5.0f * level), sampleRate);
  return filter_.process(raw, sampleRate) * level * gain_;
}

void Voice::setWaveform(Waveform waveform) {
  if (waveform == waveform_) return;  // geen nieuwe oscillator bij elke draai aan een knop
  auto oscillator = OscillatorFactory::create(waveform);
  if (!oscillator) return;
  oscillator_ = std::move(oscillator);  // de oude oscillator wordt automatisch verwijderd
  oscillator_->setPosition(wavetablePosition_);
  waveform_ = waveform;
}

void Voice::setWavetablePosition(float position) {
  wavetablePosition_ = position;
  oscillator_->setPosition(position);  // virtueel: alleen een WavetableOscillator doet er iets mee
}

}  // namespace midipiano::dsp

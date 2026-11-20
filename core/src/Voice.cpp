#include "midipiano/dsp/Voice.h"

#include <algorithm>
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
  modEnvelope_.noteOn();
}

void Voice::release() {
  held_ = false;
  envelope_.noteOff();
  modEnvelope_.noteOff();
}

float Voice::render(float sampleRate) {
  if (!isActive()) return 0.0f;
  // oscillator_ wijst naar een Sine-, Saw-, Square- of TriangleOscillator; next() roept de virtuele
  // sample() op, dus Voice hoeft niet te weten welke golfvorm de gebruiker koos.
  const float raw = oscillator_->next(frequencyHz_ * pitchFactor_, sampleRate);
  const float level = envelope_.next(sampleRate);
  return filter_.process(raw, sampleRate) * level * gain_ * ampFactor_;
}

void Voice::control(const ModValues& lfo, ModTarget envTarget, float envAmount, float sampleRate) {
  if (!isActive()) return;
  ModValues mod = lfo;
  // De modulatie-envelope loopt op control rate: 16 samples per stap, dus sampleRate / 16.
  const float envLevel = modEnvelope_.next(sampleRate / 16.0f);
  mod[static_cast<std::size_t>(envTarget)] += envAmount * envLevel;
  auto at = [&mod](ModTarget target) { return mod[static_cast<std::size_t>(target)]; };

  // Filter-envelope (Env->Filt) en modulatie samen: tot 5 octaven boven of onder de cutoff. Om de
  // 16 samples is snel genoeg voor het oor en spaart een exp2() per sample uit.
  filter_.setModulatedCutoff(
      filter_.cutoff() * std::exp2(filterEnvAmount_ * 5.0f * envelope_.level() + at(ModTarget::Cutoff) * 5.0f), sampleRate);
  filter_.setResonance(baseResonance_ + at(ModTarget::Resonance));
  filter_.setDrive(baseDrive_ * std::exp2(at(ModTarget::Drive) * 3.0f));
  pitchFactor_ = std::exp2(at(ModTarget::Pitch));  // modulatie 1 = een octaaf
  ampFactor_ = std::max(0.0f, 1.0f + at(ModTarget::Volume));
  if (at(ModTarget::Position) != 0.0f || envTarget == ModTarget::Position)
    oscillator_->setPosition(std::clamp(wavetablePosition_ + at(ModTarget::Position), 0.0f, 1.0f));
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

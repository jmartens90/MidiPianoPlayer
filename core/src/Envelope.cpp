#include "midipiano/dsp/Envelope.h"

#include <algorithm>

namespace midipiano::dsp {

namespace {
constexpr float kMinTime = 0.001f;  // 1 ms: korter geeft een hoorbare klik
}

Envelope::Envelope(float attackS, float decayS, float sustainLevel, float releaseS)
    : attack_(std::max(attackS, kMinTime)),
      decay_(std::max(decayS, kMinTime)),
      sustain_(std::clamp(sustainLevel, 0.0f, 1.0f)),
      release_(std::max(releaseS, kMinTime)) {}

void Envelope::noteOn() { stage_ = Stage::Attack; }  // level_ blijft staan: geen klik bij herstart

void Envelope::noteOff() {
  if (stage_ == Stage::Idle) return;
  stage_ = Stage::Release;
  releaseStep_ = 0.0f;  // wordt in next() berekend uit het huidige niveau
}

float Envelope::next(float sampleRate) {
  switch (stage_) {
    case Stage::Idle:
      level_ = 0.0f;
      break;
    case Stage::Attack:
      level_ += 1.0f / (attack_ * sampleRate);
      if (level_ >= 1.0f) {
        level_ = 1.0f;
        stage_ = Stage::Decay;
      }
      break;
    case Stage::Decay:
      level_ -= (1.0f - sustain_) / (decay_ * sampleRate);
      if (level_ <= sustain_) {
        level_ = sustain_;
        stage_ = Stage::Sustain;
      }
      break;
    case Stage::Sustain:
      level_ = sustain_;
      break;
    case Stage::Release:
      // Lineair van het huidige niveau naar 0 in release_ seconden.
      if (releaseStep_ == 0.0f) releaseStep_ = std::max(level_, 0.0001f) / (release_ * sampleRate);
      level_ -= releaseStep_;
      if (level_ <= 0.0f) {
        level_ = 0.0f;
        stage_ = Stage::Idle;
      }
      break;
  }
  return level_;
}

void Envelope::setAttack(float seconds) { attack_ = std::max(seconds, kMinTime); }
void Envelope::setDecay(float seconds) { decay_ = std::max(seconds, kMinTime); }
void Envelope::setSustain(float level) { sustain_ = std::clamp(level, 0.0f, 1.0f); }
void Envelope::setRelease(float seconds) { release_ = std::max(seconds, kMinTime); }

}  // namespace midipiano::dsp

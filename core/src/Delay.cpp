#include "midipiano/dsp/Delay.h"

#include <algorithm>
#include <cmath>

#include "midipiano/util/MathUtil.h"

namespace midipiano::dsp {

Delay::Delay(float sampleRate) { setSampleRate(sampleRate); }

void Delay::setSampleRate(float sampleRate) {
  sampleRate_ = sampleRate;
  line_.assign(static_cast<std::size_t>(kMaxTime * sampleRate) + 4, 0.0f);
  writePos_ = 0;
  setSettings(settings_);
  currentDelay_ = targetDelay_;
}

void Delay::setSettings(const Settings& settings) {
  settings_ = settings;
  settings_.mix = std::clamp(settings_.mix, 0.0f, 1.0f);
  settings_.time = std::clamp(settings_.time, 0.01f, kMaxTime);
  settings_.feedback = std::clamp(settings_.feedback, 0.0f, 0.95f);
  targetDelay_ = settings_.time * sampleRate_;
  const float tone = std::clamp(settings_.toneHz, 200.0f, sampleRate_ * 0.45f);
  toneCoeff_ = 1.0f - std::exp(-2.0f * util::kPi * tone / sampleRate_);
  if (!running_) currentDelay_ = targetDelay_;  // alleen glijden als de echo al loopt
}

void Delay::clear() {
  std::fill(line_.begin(), line_.end(), 0.0f);
  toneState_ = 0.0f;
}

void Delay::process(float* buffer, std::size_t frames) {
  if (settings_.mix <= 0.0f) {
    // Uit: de lijn toch bijhouden zou rekenwerk kosten; wel de tijd bijwerken voor de volgende keer.
    currentDelay_ = targetDelay_;
    running_ = false;
    return;
  }
  running_ = true;
  const std::size_t size = line_.size();
  for (std::size_t i = 0; i < frames; ++i) {
    currentDelay_ += (targetDelay_ - currentDelay_) * 0.0005f;  // zacht glijden bij het verdraaien
    // Lezen tussen twee samples in (lineaire interpolatie), want de vertraging is geen geheel getal.
    const float readPos = static_cast<float>(writePos_) - currentDelay_ + static_cast<float>(size);
    const std::size_t i0 = static_cast<std::size_t>(readPos) % size;
    const std::size_t i1 = (i0 + 1) % size;
    const float frac = readPos - std::floor(readPos);
    const float echo = line_[i0] + (line_[i1] - line_[i0]) * frac;

    toneState_ += toneCoeff_ * (echo - toneState_);  // elke herhaling iets donkerder
    line_[writePos_] = buffer[i] + toneState_ * settings_.feedback;
    if (++writePos_ >= size) writePos_ = 0;

    buffer[i] += echo * settings_.mix;
  }
}

}  // namespace midipiano::dsp

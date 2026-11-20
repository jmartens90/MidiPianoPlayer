#include "midipiano/midi/StepSequencer.h"

#include <algorithm>
#include <chrono>

namespace midipiano::midi {

StepSequencer::StepSequencer() : pattern_(defaultPattern()) {}

StepSequencer::~StepSequencer() { stop(); }

StepSequencer::Pattern StepSequencer::defaultPattern() {
  // Een baslijn in c-klein om mee te beginnen (noot 36 = C2).
  Pattern pattern;
  const int notes[kSteps] = {36, 36, 48, 36, 39, 36, 46, 36, 36, 43, 36, 48, 41, 36, 39, 34};
  const bool active[kSteps] = {1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1};
  for (std::size_t i = 0; i < kSteps; ++i) {
    pattern[i].note = static_cast<std::uint8_t>(notes[i]);
    pattern[i].active = active[i];
    pattern[i].accent = (i % 4 == 0);
  }
  return pattern;
}

void StepSequencer::start(Callback onMessage) {
  if (running_) return;
  if (thread_.joinable()) thread_.join();
  running_ = true;
  thread_ = std::thread(&StepSequencer::run, this, std::move(onMessage));
}

void StepSequencer::stop() {
  {
    std::lock_guard<std::mutex> lock(waitMutex_);
    running_ = false;
  }
  wakeUp_.notify_all();
  if (thread_.joinable()) thread_.join();
  currentStep_ = -1;
}

void StepSequencer::setStep(std::size_t index, const Step& step) {
  if (index >= kSteps) return;
  std::lock_guard<std::mutex> lock(patternMutex_);
  pattern_[index] = step;
}

StepSequencer::Step StepSequencer::step(std::size_t index) const {
  std::lock_guard<std::mutex> lock(patternMutex_);
  return index < kSteps ? pattern_[index] : Step{};
}

StepSequencer::Pattern StepSequencer::pattern() const {
  std::lock_guard<std::mutex> lock(patternMutex_);
  return pattern_;
}

void StepSequencer::setPattern(const Pattern& pattern) {
  std::lock_guard<std::mutex> lock(patternMutex_);
  pattern_ = pattern;
}

void StepSequencer::clear() {
  std::lock_guard<std::mutex> lock(patternMutex_);
  for (Step& step : pattern_) step.active = false;
}

void StepSequencer::setBpm(float bpm) { bpm_ = std::clamp(bpm, 30.0f, 300.0f); }
void StepSequencer::setGate(float gate) { gate_ = std::clamp(gate, 0.1f, 1.0f); }
void StepSequencer::setSwing(float swing) { swing_ = std::clamp(swing, 0.0f, 0.6f); }

bool StepSequencer::waitUntil(std::chrono::steady_clock::time_point time) {
  std::unique_lock<std::mutex> lock(waitMutex_);
  return !wakeUp_.wait_until(lock, time, [this] { return !running_; });
}

void StepSequencer::run(Callback onMessage) {
  using Clock = std::chrono::steady_clock;
  auto stepStart = Clock::now();
  std::size_t index = 0;
  while (running_) {
    const Step current = step(index);
    const float stepSeconds = 60.0f / bpm_ / 4.0f;  // een zestiende
    // Swing: de oneven stappen (de "en" tussen de zestienden) schuiven wat op.
    const float swingDelay = (index % 2 == 1) ? swing_ * stepSeconds * 0.5f : 0.0f;
    const auto noteStart = stepStart + std::chrono::duration_cast<Clock::duration>(
                                           std::chrono::duration<float>(swingDelay));
    if (!waitUntil(noteStart)) break;

    currentStep_ = static_cast<int>(index);
    const int note = std::clamp(static_cast<int>(current.note) + transpose_.load(), 0, 127);
    if (current.active) onMessage(MidiMessage::noteOn(0, static_cast<std::uint8_t>(note), current.accent ? 127 : 90));

    const auto noteEnd = noteStart + std::chrono::duration_cast<Clock::duration>(
                                         std::chrono::duration<float>(stepSeconds * gate_ * 0.95f));
    const bool keepGoing = waitUntil(noteEnd);
    if (current.active) onMessage(MidiMessage::noteOff(0, static_cast<std::uint8_t>(note)));
    if (!keepGoing) break;

    stepStart += std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(stepSeconds));
    if (stepStart < Clock::now() - std::chrono::milliseconds(200)) stepStart = Clock::now();  // na een hapering niet inhalen
    index = (index + 1) % kSteps;
  }
  currentStep_ = -1;
}

}  // namespace midipiano::midi

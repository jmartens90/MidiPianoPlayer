#ifndef MIDIPIANO_MIDI_STEPSEQUENCER_H
#define MIDIPIANO_MIDI_STEPSEQUENCER_H

#include <array>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>

#include "midipiano/midi/MidiSource.h"

namespace midipiano::midi {

// Een 16-stappen-sequencer zoals op een groovebox: per stap aan/uit, een noot en een accent (harder).
// Je bewerkt het patroon terwijl het speelt. Net als de DemoSequencer een MidiSource met een eigen
// thread, maar dan bewerkbaar en met tempo, gate (nootlengte) en swing.
//
// Timing: elke stap wordt gepland op een absoluut tijdstip (start + n * staplengte) en de thread
// wacht met wait_until. Zo telt een kleine vertraging niet op en blijft het tempo strak.
class StepSequencer final : public MidiSource {
 public:
  static constexpr std::size_t kSteps = 16;

  struct Step {
    bool active = false;
    std::uint8_t note = 48;
    bool accent = false;
  };
  using Pattern = std::array<Step, kSteps>;
  using Clock = std::chrono::steady_clock;
  using TimedCallback = std::function<void(const MidiMessage&, Clock::time_point)>;

  StepSequencer();
  ~StepSequencer() override;
  StepSequencer(const StepSequencer&) = delete;
  StepSequencer& operator=(const StepSequencer&) = delete;

  void start(Callback onMessage) override;
  void stop() override;
  bool isRunning() const override { return running_; }
  std::string name() const override { return "Step sequencer"; }

  void setStep(std::size_t index, const Step& step);
  Step step(std::size_t index) const;
  Pattern pattern() const;
  void setPattern(const Pattern& pattern);
  void clear();

  void setBpm(float bpm);
  float bpm() const { return bpm_; }
  void setGate(float gate);  // 0,1..1: deel van de stap dat de noot klinkt
  float gate() const { return gate_; }
  void setSwing(float swing);  // 0..0,6: elke tweede stap later
  float swing() const { return swing_; }
  void setTranspose(int semitones) { transpose_ = semitones; }
  int transpose() const { return transpose_; }

  int currentStep() const { return currentStep_; }  // -1 als hij stilstaat

  // Vooruit plannen: elk bericht gaat "lookahead" te vroeg naar buiten, samen met het exacte tijdstip
  // waarop het moet klinken (Synth::schedule). Zo speelt de synth het op het juiste sample, los van
  // de grootte van de audioblokken. Instellen voor start().
  void setTimedOutput(TimedCallback output, std::chrono::milliseconds lookahead);

  static Pattern defaultPattern();

 private:
  void run(Callback onMessage);
  bool waitUntil(std::chrono::steady_clock::time_point time);

  mutable std::mutex patternMutex_;
  Pattern pattern_;
  std::atomic<float> bpm_{120.0f};
  std::atomic<float> gate_{0.5f};
  std::atomic<float> swing_{0.0f};
  std::atomic<int> transpose_{0};
  std::atomic<int> currentStep_{-1};
  std::atomic<bool> running_{false};
  TimedCallback timedOutput_;
  std::chrono::milliseconds lookahead_{0};
  std::thread thread_;
  std::mutex waitMutex_;
  std::condition_variable wakeUp_;
};

}  // namespace midipiano::midi

#endif  // MIDIPIANO_MIDI_STEPSEQUENCER_H

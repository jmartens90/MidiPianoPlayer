#ifndef MIDIPIANO_MIDI_DEMOSEQUENCER_H
#define MIDIPIANO_MIDI_DEMOSEQUENCER_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "midipiano/midi/MidiSource.h"

namespace midipiano::midi {

// Speelt een vast patroon af, zoals de PSoC 6-sequencer dat straks via BLE doet. Zo kan de app
// getest en beoordeeld worden zonder die hardware.
class DemoSequencer final : public MidiSource {
 public:
  struct Step {
    std::uint8_t note;   // 0 = rust
    std::uint8_t length; // in zestienden
  };

  DemoSequencer();
  explicit DemoSequencer(std::vector<Step> pattern, float bpm = 110.0f);

  // Vereiste 24: 24.at least 2 destructors
  // De sequencer heeft een eigen thread. Als het object verdwijnt terwijl die thread nog loopt,
  // zou std::thread het programma afbreken (std::terminate). De destructor stopt en joint de thread
  // daarom eerst. (Tweede destructor: AudioOutput sluit de geluidskaart af.)
  ~DemoSequencer() override;

  DemoSequencer(const DemoSequencer&) = delete;
  DemoSequencer& operator=(const DemoSequencer&) = delete;

  void start(Callback onMessage) override;
  void stop() override;
  bool isRunning() const override { return running_; }
  std::string name() const override { return "Demo sequencer"; }

  void setBpm(float bpm) { bpm_ = bpm; }
  float bpm() const { return bpm_; }
  const std::vector<Step>& pattern() const { return pattern_; }

  static std::vector<Step> defaultPattern();

 private:
  void run(Callback onMessage);
  bool waitSixteenths(int count);

  std::vector<Step> pattern_;
  std::atomic<float> bpm_;
  std::atomic<bool> running_{false};
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable wakeUp_;
};

}  // namespace midipiano::midi

#endif  // MIDIPIANO_MIDI_DEMOSEQUENCER_H

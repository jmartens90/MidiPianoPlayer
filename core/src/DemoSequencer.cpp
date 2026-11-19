#include "midipiano/midi/DemoSequencer.h"

#include <chrono>
#include <utility>

namespace midipiano::midi {

DemoSequencer::DemoSequencer() : DemoSequencer(defaultPattern()) {}

DemoSequencer::DemoSequencer(std::vector<Step> pattern, float bpm) : pattern_(std::move(pattern)), bpm_(bpm) {}

DemoSequencer::~DemoSequencer() { stop(); }

std::vector<DemoSequencer::Step> DemoSequencer::defaultPattern() {
  // Een arpeggio in a-klein: Am - F - C - G, telkens grondtoon, terts, kwint, octaaf.
  return {{57, 2}, {60, 2}, {64, 2}, {69, 2}, {53, 2}, {57, 2}, {60, 2}, {65, 2},
          {48, 2}, {52, 2}, {55, 2}, {60, 2}, {55, 2}, {59, 2}, {62, 2}, {67, 2}};
}

// Vereiste 50: 50.useful usage of threads
// De sequencer moet op een vast tempo noten sturen, los van de GUI: als de GUI even hapert (scrollen,
// presets laden) mag het ritme niet verschuiven. Daarom draait run() op een eigen std::thread. stop()
// wekt de thread via een condition_variable meteen, ook midden in een wachttijd, en wacht met join().
void DemoSequencer::start(Callback onMessage) {
  if (running_) return;
  if (thread_.joinable()) thread_.join();  // een vorige run die vanzelf stopte
  running_ = true;
  thread_ = std::thread(&DemoSequencer::run, this, std::move(onMessage));
}

void DemoSequencer::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
  }
  wakeUp_.notify_all();
  if (thread_.joinable()) thread_.join();
}

void DemoSequencer::run(Callback onMessage) {
  std::size_t index = 0;
  while (running_ && !pattern_.empty()) {
    // Vereiste 38: 38.at least 4 useful const references for variables (2/4)
    // step verwijst naar het element in pattern_: geen kopie, en const omdat de sequencer het
    // patroon alleen afspeelt en nooit wijzigt.
    const Step& step = pattern_[index];
    if (step.note != 0) onMessage(MidiMessage::noteOn(0, step.note, 96));
    const bool keepGoing = waitSixteenths(step.length);
    if (step.note != 0) onMessage(MidiMessage::noteOff(0, step.note));
    if (!keepGoing) break;
    index = (index + 1) % pattern_.size();
  }
}

bool DemoSequencer::waitSixteenths(int count) {
  const auto sixteenth = std::chrono::duration<double>(60.0 / bpm_ / 4.0);
  const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(sixteenth * count);
  std::unique_lock<std::mutex> lock(mutex_);
  return !wakeUp_.wait_for(lock, duration, [this] { return !running_; });
}

}  // namespace midipiano::midi

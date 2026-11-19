#include <algorithm>
#include <cmath>
#include <vector>

#include "TestFramework.h"
#include "midipiano/dsp/Reverb.h"
#include "midipiano/dsp/Synth.h"
#include "midipiano/midi/MidiMessage.h"

using namespace midipiano;

namespace {
float peakOf(const std::vector<float>& buffer, std::size_t from = 0) {
  float p = 0.0f;
  for (std::size_t i = from; i < buffer.size(); ++i) p = std::max(p, std::fabs(buffer[i]));
  return p;
}
}  // namespace

TEST(sustain_pedal_holds_released_notes) {
  dsp::Synth synth(22050.0f);
  std::vector<float> buffer(22050);
  synth.noteOn(60);
  synth.setSustain(true);
  synth.noteOff(60);
  synth.render(buffer);  // 1 s: zonder pedaal was de noot al uitgeklonken (release 0,4 s)
  CHECK(synth.activeVoiceCount() == 1);

  synth.setSustain(false);
  synth.render(buffer);
  CHECK(synth.activeVoiceCount() == 0);
}

TEST(sustain_pedal_keeps_keys_that_are_still_down) {
  dsp::Synth synth(22050.0f);
  std::vector<float> buffer(22050);
  synth.handle(midi::MidiMessage(0xB0, 64, 127));  // CC 64: pedaal in
  CHECK(synth.sustain());
  synth.noteOn(60);
  synth.noteOn(64);
  synth.noteOff(60);                               // 64 blijft ingedrukt
  synth.handle(midi::MidiMessage(0xB0, 64, 0));    // pedaal los
  CHECK(!synth.sustain());
  synth.render(buffer);
  CHECK(synth.activeVoiceCount() == 1);            // alleen 64 klinkt nog
}

TEST(reverb_adds_a_tail_after_the_sound_stops) {
  dsp::Reverb reverb(22050.0f);
  reverb.setAmount(0.5f);
  std::vector<float> buffer(22050, 0.0f);
  for (std::size_t i = 0; i < 2205; ++i) buffer[i] = std::sin(i * 0.1f) * 0.5f;  // 0,1 s toon
  reverb.process(buffer.data(), buffer.size());
  CHECK(peakOf(buffer, 4410) > 0.001f);  // 0,1 s na het einde van de toon hoor je nog galm
}

TEST(reverb_off_leaves_the_signal_untouched) {
  dsp::Reverb reverb(22050.0f);
  reverb.setAmount(0.0f);
  std::vector<float> buffer{0.1f, -0.2f, 0.3f};
  reverb.process(buffer.data(), buffer.size());
  CHECK(buffer[0] == 0.1f && buffer[1] == -0.2f && buffer[2] == 0.3f);
}

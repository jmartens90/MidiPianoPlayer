#include <algorithm>
#include <cmath>
#include <vector>

#include "TestFramework.h"
#include <chrono>
#include <mutex>
#include <thread>

#include "midipiano/dsp/Delay.h"
#include "midipiano/dsp/LowPassFilter.h"
#include "midipiano/dsp/Reverb.h"
#include "midipiano/dsp/Synth.h"
#include "midipiano/dsp/Voice.h"
#include "midipiano/dsp/Waveforms.h"
#include "midipiano/dsp/Wavetable.h"
#include "midipiano/midi/MidiMessage.h"
#include "midipiano/midi/StepSequencer.h"
#include "midipiano/preset/Patch.h"

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

TEST(reverb_predelay_delays_the_first_reflection) {
  dsp::Reverb reverb(10000.0f);
  dsp::Reverb::Settings settings;
  settings.mix = 1.0f;
  settings.predelay = 0.2f;  // 2000 samples
  settings.lowCutHz = 20.0f;
  reverb.setSettings(settings);
  std::vector<float> buffer(4000, 0.0f);
  buffer[0] = 1.0f;
  reverb.process(buffer.data(), buffer.size());
  // Voor de predelay (2000 samples) mag er nog geen galm zijn, kort erna wel (eerste reflectie).
  float early = 0.0f;
  for (std::size_t i = 1; i < 2000; ++i) early = std::max(early, std::fabs(buffer[i]));
  CHECK(early < 1e-6f);
  CHECK(peakOf(buffer, 2000) > 1e-4f);
}

TEST(reverb_is_stereo_and_width_controls_it) {
  auto difference = [](float width) {
    dsp::Reverb reverb(22050.0f);
    dsp::Reverb::Settings settings;
    settings.mix = 1.0f;
    settings.width = width;
    reverb.setSettings(settings);
    std::vector<float> in(22050, 0.0f), left(22050), right(22050);
    in[0] = 1.0f;
    reverb.process(in.data(), left.data(), right.data(), in.size());
    float diff = 0.0f;
    for (std::size_t i = 0; i < in.size(); ++i) diff = std::max(diff, std::fabs(left[i] - right[i]));
    return diff;
  };
  CHECK(difference(1.0f) > 0.001f);  // breed: links en rechts hoor je een andere galm
  CHECK(difference(0.0f) < 1e-6f);   // breedte 0: mono
}

TEST(reverb_stays_stable_at_maximum_decay) {
  dsp::Reverb reverb(22050.0f);
  dsp::Reverb::Settings settings;
  settings.mix = 1.0f;
  settings.decay = 1.0f;
  settings.size = 1.0f;
  settings.damping = 0.0f;
  reverb.setSettings(settings);
  std::vector<float> buffer(22050 * 10, 0.0f);  // 10 seconden
  for (std::size_t i = 0; i < 2205; ++i) buffer[i] = std::sin(i * 0.1f) * 0.5f;
  reverb.process(buffer.data(), buffer.size());
  float peak = 0.0f;
  bool finite = true;
  for (float x : buffer) {
    peak = std::max(peak, std::fabs(x));
    finite = finite && std::isfinite(x);
  }
  CHECK(finite);
  CHECK(peak < 2.0f);                        // groeit niet uit de hand
  CHECK(peakOf(buffer, 22050 * 3) > 1e-4f);  // maar galmt na 3 s nog wel
}

TEST(resonant_filter_rings_and_can_self_oscillate) {
  auto ringAfterImpulse = [](float resonance) {
    dsp::LowPassFilter filter;
    filter.setCutoff(1000.0f);
    filter.setResonance(resonance);
    filter.process(1.0f, 48000.0f);
    float late = 0.0f;
    for (int i = 0; i < 48000; ++i) {
      const float y = filter.process(0.0f, 48000.0f);
      if (i > 24000) late = std::max(late, std::fabs(y));  // na een halve seconde
    }
    return late;
  };
  CHECK(ringAfterImpulse(0.0f) < 1e-4f);  // zonder resonantie is de puls snel weg
  CHECK(ringAfterImpulse(1.0f) > 0.05f);  // met volle resonantie blijft de filter fluiten
}

TEST(wavetable_morphs_and_avoids_aliasing) {
  const dsp::Wavetable table("Test", [](float x, float t) { return t < 0.5f ? std::sin(6.2831853f * x) : 2.0f * x - 1.0f; });
  // Positie 0 is een sinus, positie 1 een zaagtand: halverwege de periode verschillen ze.
  CHECK(std::fabs(table.read(0.25f, 0.0f, 0) - 1.0f) < 0.01f);
  CHECK(std::fabs(table.read(0.25f, 1.0f, 0) - table.read(0.25f, 0.0f, 0)) > 0.3f);
  // Een hoge noot (5 kHz bij 48 kHz) mag maximaal 4 boventonen hebben: de zaagtand wordt dan bijna
  // een sinus, dus geen scherpe sprong meer bij fase 0.
  const std::size_t level = dsp::Wavetable::mipLevelFor(5000.0f, 48000.0f);
  CHECK((dsp::Wavetable::kSize / 2 >> level) <= 4);
  const float jump = std::fabs(table.read(0.999f, 1.0f, level) - table.read(0.001f, 1.0f, level));
  CHECK(jump < 0.2f);
}

TEST(every_wavetable_waveform_plays_through_a_voice) {
  for (int i = static_cast<int>(dsp::Waveform::Analog); i < static_cast<int>(dsp::Waveform::Count); ++i) {
    const auto waveform = static_cast<dsp::Waveform>(i);
    CHECK(dsp::isWavetable(waveform));
    dsp::Voice voice(waveform);
    voice.setWavetablePosition(0.5f);
    voice.start(60, 127, 0);
    float p = 0.0f;
    for (int n = 0; n < 4800; ++n) p = std::max(p, std::fabs(voice.render(48000.0f)));
    CHECK(p > 0.05f);
  }
  CHECK(!dsp::isWavetable(dsp::Waveform::Saw));
}

TEST(delay_repeats_the_sound_at_the_right_time) {
  dsp::Delay delay(10000.0f);
  dsp::Delay::Settings settings;
  settings.mix = 1.0f;
  settings.time = 0.1f;  // 1000 samples
  settings.feedback = 0.5f;
  settings.toneHz = 4000.0f;
  delay.setSettings(settings);
  std::vector<float> buffer(3500, 0.0f);
  buffer[0] = 1.0f;
  delay.process(buffer.data(), buffer.size());
  CHECK(std::fabs(buffer[500]) < 1e-6f);   // nog stil
  CHECK(peakOf(std::vector<float>(buffer.begin() + 990, buffer.begin() + 1010)) > 0.3f);   // eerste echo
  CHECK(peakOf(std::vector<float>(buffer.begin() + 1990, buffer.begin() + 2010)) > 0.05f); // tweede, zachter
}

TEST(delay_syncs_to_the_sequencer_tempo) {
  dsp::Synth synth(10000.0f);
  preset::Patch patch = synth.patch();
  patch.set("delay_mix", 1.0f).set("delay_sync", 1.0f).set("delay_beats", 1.0f).set("delay_feedback", 0.0f);
  patch.set("reverb", 0.0f).set("attack", 0.001f).set("decay", 0.01f).set("sustain", 0.0f).set("release", 0.01f);
  synth.applyPatch(patch);
  synth.setTempo(120.0f);  // 1 tel = 0,5 s = 5000 samples
  synth.noteOn(60);
  std::vector<float> buffer(8000);
  synth.render(buffer.data(), 200);  // 20 ms noot
  synth.noteOff(60);
  synth.render(buffer.data() + 200, buffer.size() - 200);
  CHECK(peakOf(std::vector<float>(buffer.begin() + 3000, buffer.begin() + 4800)) < 0.01f);
  CHECK(peakOf(std::vector<float>(buffer.begin() + 4900, buffer.begin() + 5600)) > 0.05f);
}

TEST(step_sequencer_plays_active_steps_in_time) {
  midi::StepSequencer sequencer;
  midi::StepSequencer::Pattern pattern;
  pattern[0] = {true, 60, true};
  pattern[2] = {true, 64, false};
  sequencer.setPattern(pattern);
  sequencer.setBpm(300.0f);  // zestiende = 50 ms
  sequencer.setTranspose(12);
  std::mutex mutex;
  std::vector<midi::MidiMessage> received;
  sequencer.start([&](const midi::MidiMessage& m) {
    std::lock_guard<std::mutex> lock(mutex);
    received.push_back(m);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(180));
  sequencer.stop();
  CHECK(!sequencer.isRunning());
  CHECK(sequencer.currentStep() == -1);
  std::lock_guard<std::mutex> lock(mutex);
  CHECK(received.size() >= 4);
  CHECK(received[0] == midi::MidiMessage::noteOn(0, 72, 127));  // accent + 1 octaaf hoger
  CHECK(received[1].isNoteOff());
  CHECK(received[2] == midi::MidiMessage::noteOn(0, 76, 90));
}

TEST(scheduled_note_starts_on_the_exact_sample) {
  dsp::Synth synth(10000.0f);
  preset::Patch patch = synth.patch();
  patch.set("reverb", 0.0f).set("attack", 0.001f);
  synth.applyPatch(patch);
  const auto blockStart = dsp::Synth::Clock::now();
  // 25 ms na het begin van het blok = sample 250 bij 10 kHz.
  synth.schedule(midi::MidiMessage::noteOn(0, 69, 120), blockStart + std::chrono::milliseconds(25));
  std::vector<float> buffer(1000);
  synth.render(buffer.data(), buffer.size(), blockStart);
  CHECK(peakOf(std::vector<float>(buffer.begin(), buffer.begin() + 249)) == 0.0f);
  CHECK(peakOf(std::vector<float>(buffer.begin() + 250, buffer.begin() + 400)) > 0.01f);
}

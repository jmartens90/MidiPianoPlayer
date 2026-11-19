// Vereiste 53: 53.test-driven development (= written test plan or unit tests)
// Deze unit tests (en docs/TESTPLAN.md) testen de synth zonder geluidskaart of scherm: golfvormen,
// envelope, stemtoewijzing, MIDI-berichten, presets en de sequencer. Ze draaien bij elke push in
// GitHub Actions (ctest), zodat een fout in bv. voice stealing meteen opvalt.

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "TestFramework.h"
#include "midipiano/dsp/Envelope.h"
#include "midipiano/dsp/Synth.h"
#include "midipiano/dsp/VoiceSynth.h"
#include "midipiano/dsp/Waveforms.h"
#include "midipiano/util/MathUtil.h"
#include "midipiano/util/RingBuffer.h"

using namespace midipiano;

TEST(midiToHz_gives_concert_pitch) {
  CHECK_NEAR(util::midiToHz(69), 440.0f, 0.01f);
  CHECK_NEAR(util::midiToHz(81), 880.0f, 0.01f);
  CHECK_NEAR(util::midiToHz(60), 261.63f, 0.01f);
}

TEST(waveforms_have_expected_values) {
  CHECK_NEAR(dsp::SineOscillator().sample(0.25f), 1.0f, 1e-5f);
  CHECK_NEAR(dsp::SawOscillator().sample(0.0f), -1.0f, 1e-5f);
  CHECK_NEAR(dsp::SquareOscillator().sample(0.75f), -1.0f, 1e-5f);
  CHECK_NEAR(dsp::TriangleOscillator().sample(0.5f), 1.0f, 1e-5f);
}

// Vereiste 27: 27.useful proven (dynamic) polymorphism
// Deze test *bewijst* dat de polymorfie werkt: de factory geeft voor elke golfvorm een
// unique_ptr<Oscillator> terug, en via dezelfde basispointer geeft sample() en name() telkens het
// resultaat van de juiste afgeleide klasse. Zie ook sequencer_works_through_base_class.
TEST(factory_returns_right_oscillator_through_base_pointer) {
  const std::vector<std::string> expected = {"Sine", "Saw", "Square", "Triangle"};
  for (int i = 0; i < static_cast<int>(dsp::Waveform::Count); ++i) {
    std::unique_ptr<dsp::Oscillator> oscillator = dsp::OscillatorFactory::create(static_cast<dsp::Waveform>(i));
    CHECK(oscillator != nullptr);
    CHECK(oscillator->name() == expected[static_cast<std::size_t>(i)]);
  }
  CHECK(dsp::OscillatorFactory::create(dsp::Waveform::Count) == nullptr);
  CHECK(dsp::waveformFromName("Square") == dsp::Waveform::Square);
  CHECK_THROWS(dsp::waveformFromName("Banjo"), std::invalid_argument);
}

TEST(oscillator_completes_one_cycle_per_period) {
  dsp::SawOscillator saw;
  // 100 Hz bij 1000 samples/s: na 10 samples zijn we rond, dus terug bij -1.
  for (int i = 0; i < 10; ++i) saw.next(100.0f, 1000.0f);
  CHECK_NEAR(saw.next(100.0f, 1000.0f), -1.0f, 1e-4f);
}

TEST(envelope_runs_through_adsr) {
  const float rate = 1000.0f;
  dsp::Envelope env(0.01f, 0.01f, 0.5f, 0.02f);  // 10 samples attack, 10 decay, 20 release
  CHECK(env.isIdle());
  env.noteOn();
  for (int i = 0; i < 10; ++i) env.next(rate);
  CHECK_NEAR(env.level(), 1.0f, 1e-4f);
  for (int i = 0; i < 11; ++i) env.next(rate);
  CHECK(env.stage() == dsp::Envelope::Stage::Sustain);
  CHECK_NEAR(env.level(), 0.5f, 1e-4f);
  env.noteOff();
  for (int i = 0; i < 25; ++i) env.next(rate);
  CHECK(env.isIdle());
}

TEST(envelope_default_constructor_forwards) {
  dsp::Envelope env;
  CHECK_NEAR(env.attack(), 0.01f, 1e-6f);
  CHECK_NEAR(env.sustain(), 0.6f, 1e-6f);
}

TEST(synth_allocates_and_steals_voices) {
  dsp::Synth synth(1000.0f);
  std::vector<float> buffer(16);
  synth.noteOn(60);
  synth.render(buffer);
  CHECK(synth.activeVoiceCount() == 1);
  for (std::uint8_t n = 61; n < 61 + 10; ++n) synth.noteOn(n);  // 11 noten, maar 8 stemmen
  synth.render(buffer);
  CHECK(synth.activeVoiceCount() == dsp::VoiceSynth::kMaxVoices);
}

TEST(synth_goes_silent_after_note_off) {
  dsp::Synth synth(1000.0f);
  std::vector<float> buffer(200);
  synth.noteOn(69, 127);
  synth.render(buffer);
  float peak = 0.0f;
  for (float s : buffer) peak = std::max(peak, std::fabs(s));
  CHECK(peak > 0.1f);
  synth.noteOff(69);
  std::vector<float> tail(5000);  // release is 0,4 s = 400 samples
  synth.render(tail);
  CHECK(synth.activeVoiceCount() == 0);
  CHECK_NEAR(tail.back(), 0.0f, 1e-6f);
}

TEST(synth_handles_midi_messages) {
  dsp::Synth synth(1000.0f);
  std::vector<float> buffer(8);
  synth.handle(midi::MidiMessage::noteOn(0, 64, 100));
  synth.render(buffer);
  CHECK(synth.activeVoiceCount() == 1);
  synth.handle(midi::MidiMessage(0xB0, 123, 0));  // All Notes Off
  std::vector<float> tail(5000);
  synth.render(tail);
  CHECK(synth.activeVoiceCount() == 0);
}

TEST(scope_keeps_last_samples) {
  dsp::Synth synth(1000.0f);
  std::vector<float> buffer(2000);
  synth.noteOn(60);
  synth.render(buffer);
  std::vector<float> scope;
  synth.scopeSnapshot(scope);
  CHECK(scope.size() == dsp::Synth::kScopeSize);
  CHECK_NEAR(scope.back(), buffer.back(), 1e-6f);
}

TEST(ring_buffer_wraps_in_order) {
  util::RingBuffer<int> ring(3);
  for (int i = 1; i <= 5; ++i) ring.push(i);
  std::vector<int> out;
  ring.copyTo(out);
  CHECK(out.size() == 3);
  CHECK(out[0] == 3 && out[1] == 4 && out[2] == 5);
}

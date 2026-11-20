#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

#include "TestFramework.h"
#include "midipiano/midi/DemoSequencer.h"
#include "midipiano/midi/MidiMessage.h"
#include "midipiano/preset/PresetStore.h"

using namespace midipiano;

TEST(midi_note_on_with_velocity_zero_is_note_off) {
  const midi::MidiMessage on = midi::MidiMessage::noteOn(2, 60, 100);
  CHECK(on.isNoteOn());
  CHECK(on.channel() == 2);
  CHECK(on.status() == 0x92);
  const midi::MidiMessage zero(0x90, 60, 0);
  CHECK(zero.isNoteOff());
  CHECK(!zero.isNoteOn());
}

TEST(midi_message_copy_and_print) {
  const midi::MidiMessage original = midi::MidiMessage::noteOn(0, 60, 100);
  const midi::MidiMessage copy(original);
  CHECK(copy == original);
  std::ostringstream text;
  text << copy;
  CHECK(text.str() == "NoteOn ch1 note 60 vel 100");
  CHECK(sizeof(midi::MidiMessage) == 3);
}

TEST(patch_clamps_and_chains) {
  preset::Patch patch;
  patch.set("cutoff", 99999.0f).set("sustain", -1.0f);
  CHECK_NEAR(patch.get("cutoff"), 12000.0f, 1e-3f);
  CHECK_NEAR(patch.get("sustain"), 0.0f, 1e-6f);
  CHECK_THROWS(patch.set("wobble", 1.0f), std::out_of_range);
}

TEST(patch_copy_is_independent) {
  preset::Patch a;
  a.set("attack", 1.0f);
  preset::Patch b(a);
  b.set("attack", 0.5f);
  CHECK_NEAR(a.get("attack"), 1.0f, 1e-6f);
  CHECK_NEAR(b.get("attack"), 0.5f, 1e-6f);
}

namespace {
std::filesystem::path freshTempDir(const std::string& name) {
  auto dir = std::filesystem::temp_directory_path() / ("midipiano_test_" + name);
  std::filesystem::remove_all(dir);
  return dir;
}
}  // namespace

TEST(preset_round_trip) {
  const preset::PresetStore store(freshTempDir("roundtrip"));
  preset::Patch patch;
  patch.setWaveform(dsp::Waveform::Triangle).set("release", 1.5f).set("cutoff", 900.0f);
  patch.setName("My Bass");
  store.save(patch);
  const preset::Patch loaded = store.load("My Bass");
  CHECK(loaded.waveform() == dsp::Waveform::Triangle);
  CHECK_NEAR(loaded.get("release"), 1.5f, 1e-4f);
  CHECK_NEAR(loaded.get("cutoff"), 900.0f, 1e-2f);
  CHECK(store.list().size() == 1);
  CHECK(store.list()[0] == "My Bass");
}

TEST(preset_errors_are_reported) {
  const auto dir = freshTempDir("errors");
  const preset::PresetStore store(dir);
  CHECK_THROWS(store.load("Does Not Exist"), preset::PresetError);
  std::ofstream(dir / "Broken.preset") << "waveform=Saw\ncutoff=abc\n";
  CHECK_THROWS(store.load("Broken"), preset::PresetError);
  std::ofstream(dir / "Unknown.preset") << "wobble=3\n";
  CHECK_THROWS(store.load("Unknown"), preset::PresetError);
}

TEST(factory_presets_are_installed_once) {
  const preset::PresetStore store(freshTempDir("factory"));
  store.installFactoryPresets();
  store.installFactoryPresets();
  CHECK(store.list().size() == 6);
  CHECK(store.load("Soft Pad").waveform() == dsp::Waveform::Triangle);
}

TEST(sequencer_works_through_base_class) {
  std::unique_ptr<midi::MidiSource> source =
      std::make_unique<midi::DemoSequencer>(std::vector<midi::DemoSequencer::Step>{{60, 1}, {64, 1}}, 600.0f);
  std::atomic<int> noteOns{0};
  std::atomic<int> noteOffs{0};
  source->start([&](const midi::MidiMessage& m) {
    if (m.isNoteOn()) ++noteOns;
    if (m.isNoteOff()) ++noteOffs;
  });
  CHECK(source->isRunning());
  std::this_thread::sleep_for(std::chrono::milliseconds(200));  // een zestiende = 25 ms
  source->stop();
  CHECK(!source->isRunning());
  CHECK(noteOns >= 3);
  CHECK(noteOns == noteOffs);  // elke noot wordt ook weer losgelaten
  CHECK(source->name() == "Demo sequencer");
}

TEST(sequencer_stops_quickly_even_when_slow) {
  midi::DemoSequencer sequencer({{60, 16}}, 30.0f);  // een noot van 2 s
  sequencer.start([](const midi::MidiMessage&) {});
  const auto begin = std::chrono::steady_clock::now();
  sequencer.stop();
  CHECK(std::chrono::steady_clock::now() - begin < std::chrono::milliseconds(200));
}

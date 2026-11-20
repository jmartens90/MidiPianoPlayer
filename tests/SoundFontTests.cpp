#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "TestFramework.h"
#include "midipiano/dsp/SoundFont.h"
#include "midipiano/dsp/Synth.h"

using namespace midipiano;

namespace {
float peak(const std::vector<float>& buffer) {
  float p = 0.0f;
  for (float s : buffer) p = std::max(p, std::fabs(s));
  return p;
}
}  // namespace

TEST(soundfont_loads_the_piano_presets) {
  const auto font = dsp::SoundFont::fromFile(MIDIPIANO_SOUNDFONT);
  CHECK(font->presetCount() == 8);
  CHECK(font->presetName(0) == "Grand Piano");
}

TEST(soundfont_errors_are_reported) {
  CHECK_THROWS(dsp::SoundFont::fromFile("does/not/exist.sf2"), dsp::SoundFontError);
  CHECK_THROWS(dsp::SoundFont::fromMemory(std::vector<char>(100, 'x')), dsp::SoundFontError);
}

TEST(piano_plays_and_stops_through_the_synth) {
  dsp::Synth synth(22050.0f);
  const auto font = dsp::SoundFont::fromFile(MIDIPIANO_SOUNDFONT);
  const std::size_t piano = synth.addInstrument(std::make_unique<dsp::SoundFontInstrument>(font, 0, 22050.0f));
  synth.selectInstrument(piano);
  CHECK(synth.instrumentNames()[piano] == "Grand Piano");
  CHECK(!synth.currentUsesPatch());

  std::vector<float> buffer(4410);
  synth.noteOn(60, 110);
  synth.render(buffer);
  CHECK(peak(buffer) > 0.05f);
  CHECK(synth.activeVoiceCount() >= 1);

  synth.noteOff(60);
  std::vector<float> tail(22050 * 3);  // 3 s: genoeg voor de uitklinktijd van de piano
  synth.render(tail);
  CHECK(synth.activeVoiceCount() == 0);
}

TEST(instruments_share_one_soundfont) {
  auto font = dsp::SoundFont::fromFile(MIDIPIANO_SOUNDFONT);
  auto piano = std::make_unique<dsp::SoundFontInstrument>(font, 0, 48000.0f);
  auto organ = std::make_unique<dsp::SoundFontInstrument>(font, 5, 48000.0f);
  CHECK(font.use_count() == 3);  // de test zelf + 2 instrumenten
  piano.reset();
  CHECK(font.use_count() == 2);
  CHECK(organ->name() == "Pipe Organ");
}

// Regressietest voor een hangende noot: met het pedaal dezelfde toets twee keer aanslaan gaf twee
// stemmen op een toets, en het loslaten van het pedaal liet er maar een los.
TEST(restruck_piano_note_never_hangs) {
  dsp::Synth synth(22050.0f);
  const auto font = dsp::SoundFont::fromFile(MIDIPIANO_SOUNDFONT);
  synth.selectInstrument(synth.addInstrument(std::make_unique<dsp::SoundFontInstrument>(font, 0, 22050.0f)));
  synth.setSustain(true);
  synth.noteOn(60);
  synth.noteOff(60);
  synth.noteOn(60);
  synth.noteOff(60);
  synth.setSustain(false);
  synth.noteOn(64);  // twee aanslagen, een noteOff
  synth.noteOn(64);
  synth.noteOff(64);
  std::vector<float> tail(22050 * 4);
  synth.render(tail);
  CHECK(synth.activeVoiceCount() == 0);
}

TEST(switching_instrument_releases_held_notes) {
  dsp::Synth synth(22050.0f);
  synth.noteOn(60);  // op de synth, en de noteOff "gaat verloren"
  const auto font = dsp::SoundFont::fromFile(MIDIPIANO_SOUNDFONT);
  synth.selectInstrument(synth.addInstrument(std::make_unique<dsp::SoundFontInstrument>(font, 0, 22050.0f)));
  std::vector<float> tail(22050 * 2);
  synth.render(tail);
  CHECK(synth.activeVoiceCount() == 0);
}

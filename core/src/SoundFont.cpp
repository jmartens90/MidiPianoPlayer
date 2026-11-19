#include "midipiano/dsp/SoundFont.h"

#include <tsf.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>

namespace midipiano::dsp {

namespace {
constexpr int kMaxSampleVoices = 64;  // vooraf gereserveerd: geen geheugen aanvragen in de audio-thread
}

std::shared_ptr<SoundFont> SoundFont::fromFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw SoundFontError("SoundFont niet gevonden: " + path.string());
  const std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return fromMemory(data);
}

std::shared_ptr<SoundFont> SoundFont::fromMemory(const std::vector<char>& data) {
  tsf* font = tsf_load_memory(data.data(), static_cast<int>(data.size()));
  if (font == nullptr) throw SoundFontError("geen geldig SoundFont-bestand");
  // De constructor is private (alleen via fromFile/fromMemory), dus geen make_shared.
  return std::shared_ptr<SoundFont>(new SoundFont(font));
}

SoundFont::~SoundFont() { tsf_close(font_); }

int SoundFont::presetCount() const { return tsf_get_presetcount(font_); }

std::string SoundFont::presetName(int index) const {
  const char* name = tsf_get_presetname(font_, index);
  return name ? std::string(name) : std::string();
}

SoundFontInstrument::SoundFontInstrument(std::shared_ptr<const SoundFont> font, int presetIndex, float sampleRate)
    : font_(std::move(font)), voices_(tsf_copy(font_->handle())), presetIndex_(presetIndex),
      name_(font_->presetName(presetIndex)) {
  if (voices_ == nullptr) throw SoundFontError("kan SoundFont-instrument niet maken");
  tsf_set_max_voices(voices_, kMaxSampleVoices);
  setSampleRate(sampleRate);
}

SoundFontInstrument::~SoundFontInstrument() { tsf_close(voices_); }

void SoundFontInstrument::noteOn(std::uint8_t note, std::uint8_t velocity) {
  tsf_note_on(voices_, presetIndex_, note, static_cast<float>(velocity) / 127.0f);
}

void SoundFontInstrument::noteOff(std::uint8_t note) { tsf_note_off(voices_, presetIndex_, note); }

void SoundFontInstrument::allNotesOff() { tsf_note_off_all(voices_); }

void SoundFontInstrument::render(float* out, std::size_t frames) {
  tsf_render_float(voices_, out, static_cast<int>(frames), 0);
}

void SoundFontInstrument::setSampleRate(float sampleRate) {
  tsf_set_output(voices_, TSF_MONO, static_cast<int>(sampleRate), 0.0f);
}

std::size_t SoundFontInstrument::activeVoiceCount() const {
  return static_cast<std::size_t>(tsf_active_voice_count(voices_));
}

}  // namespace midipiano::dsp

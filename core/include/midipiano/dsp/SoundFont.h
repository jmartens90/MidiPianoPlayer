#ifndef MIDIPIANO_DSP_SOUNDFONT_H
#define MIDIPIANO_DSP_SOUNDFONT_H

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "midipiano/dsp/Instrument.h"

struct tsf;  // uit tsf.h (TinySoundFont)

namespace midipiano::dsp {

class SoundFontError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Een ingeladen SoundFont-bestand (.sf2): opgenomen samples van echte instrumenten plus hoe ze
// over de toetsen verdeeld zijn. Wordt gedeeld door alle SoundFontInstruments die eruit spelen.
class SoundFont {
 public:
  static std::shared_ptr<SoundFont> fromFile(const std::filesystem::path& path);    // gooit SoundFontError
  static std::shared_ptr<SoundFont> fromMemory(const std::vector<char>& data);      // gooit SoundFontError

  ~SoundFont();
  SoundFont(const SoundFont&) = delete;
  SoundFont& operator=(const SoundFont&) = delete;

  int presetCount() const;
  std::string presetName(int index) const;
  tsf* handle() const { return font_; }

 private:
  explicit SoundFont(tsf* font) : font_(font) {}
  tsf* font_;
};

// Speelt een preset (bv. "Grand Piano") uit een SoundFont, via de bibliotheek TinySoundFont.
class SoundFontInstrument final : public Instrument {
 public:
  SoundFontInstrument(std::shared_ptr<const SoundFont> font, int presetIndex, float sampleRate);
  ~SoundFontInstrument() override;

  SoundFontInstrument(const SoundFontInstrument&) = delete;
  SoundFontInstrument& operator=(const SoundFontInstrument&) = delete;

  std::string name() const override { return name_; }
  void noteOn(std::uint8_t note, std::uint8_t velocity) override;
  void noteOff(std::uint8_t note) override;
  void allNotesOff() override;
  void render(float* out, std::size_t frames) override;
  void setSampleRate(float sampleRate) override;
  std::size_t activeVoiceCount() const override;

 private:
  // Vereiste 40: 40.2 useful and correct smart pointers
  // Alle instrumenten uit hetzelfde .sf2-bestand delen de samples (6 MB) via std::shared_ptr: het
  // bestand wordt een keer ingeladen en pas vrijgegeven als het laatste instrument verdwijnt. Hier is
  // gedeeld eigenaarschap echt nodig, in tegenstelling tot de unique_ptr's elders.
  std::shared_ptr<const SoundFont> font_;
  tsf* voices_;  // eigen afspeelstatus (stemmen), deelt de samples met font_
  int presetIndex_;
  std::string name_;
};

}  // namespace midipiano::dsp

#endif  // MIDIPIANO_DSP_SOUNDFONT_H

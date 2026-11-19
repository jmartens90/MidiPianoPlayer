#include "midipiano/preset/PresetStore.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace midipiano::preset {

namespace fs = std::filesystem;

PresetStore::PresetStore(const fs::path& directory) : directory_(directory) {
  std::error_code error;
  fs::create_directories(directory_, error);
  if (error) throw PresetError("kan presetmap niet maken: " + directory_.string() + " (" + error.message() + ")");
}

// Vereiste 39: 39.at least 4 useful const references for functions
// save(const Patch&), load(const std::string&), Synth::handle(const MidiMessage&) en
// Synth::applyPatch(const Patch&) krijgen hun argument als const-referentie: geen kopie van een
// Patch met zijn std::map of van een string, en de functie belooft het argument niet te wijzigen.
void PresetStore::save(const Patch& patch) const {
  // Vereiste 47: 47.useful usage of (modern) file-I/O
  // Presets worden met std::ofstream als tekst weggeschreven (via operator<< van Patch) en met
  // std::ifstream terug ingelezen; std::filesystem bouwt de paden en somt de bestanden op. Zo
  // blijven je klanken bewaard als je de app afsluit, en kan je een .preset-bestand ook met de hand
  // lezen of aanpassen.
  std::ofstream out(pathFor(patch.name()));
  if (!out) throw PresetError("kan preset niet opslaan: " + pathFor(patch.name()).string());
  out << patch;
}

Patch PresetStore::load(const std::string& name) const {
  std::ifstream in(pathFor(name));
  if (!in) throw PresetError("preset bestaat niet: " + name);

  Patch patch;
  patch.setName(name);
  std::string line;
  int lineNumber = 0;
  // Vereiste 48: 48.useful exception handling
  // Een preset-bestand kan met de hand aangepast of beschadigd zijn. Patch::set() gooit
  // std::out_of_range bij een onbekende parameter, std::stof gooit std::invalid_argument bij
  // "abc" als getal. Hier vangen we die op en gooien een PresetError met het regelnummer, die de app
  // als melding toont in plaats van te crashen. Een half ingelezen patch wordt nooit toegepast.
  try {
    while (std::getline(in, line)) {
      ++lineNumber;
      if (line.empty() || line.front() == '#') continue;
      // Vereiste 44: 44.useful string class usage (c++ functionality)
      // std::string::find, substr en std::stof splitsen een regel als "cutoff=1200" in sleutel en
      // waarde, zonder C-functies als strtok of vaste char-buffers.
      const std::size_t equals = line.find('=');
      if (equals == std::string::npos) throw std::invalid_argument("geen '=' gevonden");
      const std::string key = line.substr(0, equals);
      const std::string value = line.substr(equals + 1);
      if (key == "name") continue;  // de bestandsnaam is de echte naam
      if (key == "waveform") {
        patch.setWaveform(dsp::waveformFromName(value));
      } else {
        patch.set(key, std::stof(value));
      }
    }
  } catch (const std::exception& e) {
    throw PresetError("preset '" + name + "' regel " + std::to_string(lineNumber) + ": " + e.what());
  }
  return patch;
}

std::vector<std::string> PresetStore::list() const {
  std::vector<std::string> names;
  for (const auto& entry : fs::directory_iterator(directory_)) {
    if (entry.is_regular_file() && entry.path().extension() == ".preset") {
      names.push_back(entry.path().stem().string());
    }
  }
  std::sort(names.begin(), names.end());
  return names;
}

bool PresetStore::exists(const std::string& name) const { return fs::exists(pathFor(name)); }

void PresetStore::installFactoryPresets() const {
  using dsp::Waveform;
  Patch pad;
  pad.setWaveform(Waveform::Triangle).set("attack", 0.8f).set("decay", 1.0f).set("sustain", 0.8f).set("release", 2.0f).set("cutoff", 2500.0f);
  pad.setName("Soft Pad");
  Patch lead;
  lead.setWaveform(Waveform::Square).set("attack", 0.005f).set("decay", 0.2f).set("sustain", 0.5f).set("release", 0.2f).set("cutoff", 6000.0f);
  lead.setName("Square Lead");
  Patch pluck;
  pluck.setWaveform(Waveform::Saw).set("attack", 0.001f).set("decay", 0.25f).set("sustain", 0.0f).set("release", 0.3f).set("cutoff", 1800.0f);
  pluck.setName("Pluck");
  Patch organ;
  organ.setWaveform(Waveform::Sine).set("attack", 0.01f).set("decay", 0.1f).set("sustain", 1.0f).set("release", 0.1f).set("cutoff", 12000.0f);
  organ.setName("Sine Organ");

  // Vereiste 38: 38.at least 4 useful const references for variables (3/4)
  // patch verwijst naar elk van de vier presets in de lijst: zonder kopie en alleen-lezen, want
  // opslaan mag een patch niet wijzigen.
  for (const Patch& patch : {pad, lead, pluck, organ}) {
    if (!exists(patch.name())) save(patch);
  }
}

fs::path PresetStore::pathFor(const std::string& name) const {
  // Alleen veilige tekens in een bestandsnaam: "Soft/Pad" mag niet in een andere map schrijven.
  std::string safe = name.empty() ? std::string("Untitled") : name;
  std::replace_if(safe.begin(), safe.end(), [](char c) { return c == '/' || c == '\\' || c == ':' || c == '.'; }, '_');
  return directory_ / (safe + ".preset");
}

}  // namespace midipiano::preset

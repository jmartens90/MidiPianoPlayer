#ifndef MIDIPIANO_PRESET_PRESETSTORE_H
#define MIDIPIANO_PRESET_PRESETSTORE_H

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "midipiano/preset/Patch.h"

namespace midipiano::preset {

// Fout bij het lezen of schrijven van een preset (bestand ontbreekt, is kapot, ...).
class PresetError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Bewaart patches als leesbare tekstbestanden "<naam>.preset" in een map.
class PresetStore {
 public:
  explicit PresetStore(const std::filesystem::path& directory);

  void save(const Patch& patch) const;
  Patch load(const std::string& name) const;
  std::vector<std::string> list() const;
  bool exists(const std::string& name) const;
  void installFactoryPresets() const;

  const std::filesystem::path& directory() const { return directory_; }

 private:
  std::filesystem::path pathFor(const std::string& name) const;
  std::filesystem::path directory_;
};

}  // namespace midipiano::preset

#endif  // MIDIPIANO_PRESET_PRESETSTORE_H

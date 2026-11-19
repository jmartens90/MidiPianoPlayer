#ifndef MIDIPIANO_PRESET_PATCH_H
#define MIDIPIANO_PRESET_PATCH_H

#include <map>
#include <ostream>
#include <string>

#include "midipiano/dsp/Waveforms.h"
#include "midipiano/preset/Param.h"

namespace midipiano::preset {

// Een klankinstelling: golfvorm + ADSR + cutoff + volume, met een naam. Wordt opgeslagen als preset.
class Patch {
 public:
  // Vereiste 21: 21.at least 2 default constructors
  // Patch() maakt de standaardklank met alle parameters en hun bereik. De synth start hiermee,
  // en het inlezen van een bestand vertrekt hiervan (wat niet in het bestand staat, blijft standaard).
  Patch();

  // Vereiste 23: 23.at least 2 copy constructors
  // De synth geeft een kopie van zijn huidige patch aan de GUI en aan PresetStore::save(). Zo kan de
  // GUI-thread de kopie lezen terwijl de audio-thread de echte patch blijft gebruiken.
  Patch(const Patch& other);
  Patch& operator=(const Patch& other) = default;

  Patch& set(const std::string& paramName, float value);  // gooit std::out_of_range
  float get(const std::string& paramName) const;          // gooit std::out_of_range
  bool has(const std::string& paramName) const { return params_.count(paramName) > 0; }

  const std::string& name() const { return name_; }
  void setName(const std::string& name) { name_ = name; }
  dsp::Waveform waveform() const { return waveform_; }
  Patch& setWaveform(dsp::Waveform waveform);

  const std::map<std::string, Param>& params() const { return params_; }

  // Vereiste 35: 35.useful friend function or class
  // operator<< moet links een std::ostream hebben, dus het kan geen lid van Patch zijn. Als friend
  // mag het toch rechtstreeks name_, waveform_ en params_ lezen om de patch uit te schrijven. Dat
  // gebruiken PresetStore (bestand) en de tests (leesbare foutmeldingen).
  friend std::ostream& operator<<(std::ostream& os, const Patch& patch);

 private:
  std::string name_;
  dsp::Waveform waveform_;
  std::map<std::string, Param> params_;
};

}  // namespace midipiano::preset

#endif  // MIDIPIANO_PRESET_PATCH_H

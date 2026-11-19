#include "midipiano/preset/Patch.h"

#include <stdexcept>

namespace midipiano::preset {

Patch::Patch() : name_("Init"), waveform_(dsp::Waveform::Analog) {
  // naam, minimum, maximum, standaard
  for (const Param& param : {Param("attack", 0.001f, 2.0f, 0.01f), Param("decay", 0.01f, 2.0f, 0.3f),
                             Param("sustain", 0.0f, 1.0f, 0.6f), Param("release", 0.01f, 3.0f, 0.4f),
                             Param("cutoff", 100.0f, 12000.0f, 3000.0f), Param("volume", 0.0f, 1.0f, 0.7f),
                             Param("position", 0.0f, 1.0f, 0.66f),
                             Param("resonance", 0.0f, 1.0f, 0.2f), Param("drive", 1.0f, 10.0f, 1.0f),
                             Param("envmod", 0.0f, 1.0f, 0.0f),
                             // Galm: mix, predelay (s), uitklinktijd, demping, low cut voor en high cut na de galm.
                             Param("reverb", 0.0f, 1.0f, 0.25f), Param("reverb_predelay", 0.0f, 0.25f, 0.02f),
                             Param("reverb_decay", 0.0f, 1.0f, 0.6f), Param("reverb_damp", 0.0f, 1.0f, 0.3f),
                             Param("reverb_lowcut", 20.0f, 1000.0f, 150.0f),
                             Param("reverb_highcut", 1000.0f, 16000.0f, 7000.0f)}) {
    params_.emplace(param.name(), param);
  }
}

Patch::Patch(const Patch& other) : name_(other.name_), waveform_(other.waveform_), params_(other.params_) {}

// Vereiste 28: 28.useful usage of "this" (if the code does not work without it)
// set() geeft *this terug, zodat je instellingen kan ketenen:
//   Patch().setWaveform(Waveform::Square).set("cutoff", 800).set("release", 1.2f)
// Zonder "return *this" kan set() geen referentie naar het eigen object teruggeven en werkt die
// ketting niet. (Een tweede, onmisbaar gebruik staat in AudioOutput: pUserData = this.)
Patch& Patch::set(const std::string& paramName, float value) {
  auto it = params_.find(paramName);
  if (it == params_.end()) throw std::out_of_range("onbekende parameter: " + paramName);
  it->second.setValue(value);
  return *this;
}

Patch& Patch::setWaveform(dsp::Waveform waveform) {
  waveform_ = waveform;
  return *this;
}

float Patch::get(const std::string& paramName) const {
  auto it = params_.find(paramName);
  if (it == params_.end()) throw std::out_of_range("onbekende parameter: " + paramName);
  return it->second.value();
}

std::ostream& operator<<(std::ostream& os, const Patch& patch) {
  os << "name=" << patch.name_ << '\n' << "waveform=" << dsp::waveformName(patch.waveform_) << '\n';
  for (const auto& [key, param] : patch.params_) os << key << '=' << param.value() << '\n';
  return os;
}

}  // namespace midipiano::preset

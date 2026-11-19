#ifndef MIDIPIANO_PRESET_PARAM_H
#define MIDIPIANO_PRESET_PARAM_H

#include <algorithm>
#include <string>
#include <utility>

namespace midipiano::preset {

// Een instelbare waarde met een naam en een bereik, bv. "cutoff" van 100 tot 12000 Hz.
class Param {
 public:
  Param(std::string name, float minimum, float maximum, float value)
      : name_(std::move(name)), min_(minimum), max_(maximum), value_(std::clamp(value, minimum, maximum)) {}

  // Vereiste 32: 32.useful getters and setters for member variables
  // value() leest de waarde; setValue() is de enige manier om ze te veranderen en houdt ze binnen
  // [min, max]. Een knop, een preset-bestand of een fout BLE-bericht kan zo nooit een attack van
  // -3 s of een cutoff van 1 MHz instellen.
  float value() const { return value_; }
  void setValue(float value) { value_ = std::clamp(value, min_, max_); }

  const std::string& name() const { return name_; }
  float minimum() const { return min_; }
  float maximum() const { return max_; }

 private:
  // Vereiste 3: 3.useful and correct encapsulation (explain why)
  // De waarde en het bereik zijn private. Alleen setValue() mag value_ veranderen, en die clamp't
  // altijd. Daardoor is de regel "een parameter ligt altijd binnen zijn bereik" op een plek
  // gegarandeerd, in plaats van bij elke knop of elk bestand opnieuw.
  std::string name_;
  float min_;
  float max_;
  float value_;
};

}  // namespace midipiano::preset

#endif  // MIDIPIANO_PRESET_PARAM_H

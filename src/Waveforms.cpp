#include "Waveforms.h"

#include <cmath>

namespace midipiano {

float SineOscillator::sample(float phase) const {
  return std::sin(2.0f * 3.14159265f * phase);
}

float SquareOscillator::sample(float phase) const {
  return phase < 0.5f ? 1.0f : -1.0f;
}

}  // namespace midipiano

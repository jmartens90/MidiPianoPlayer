#ifndef MIDIPIANO_UTIL_MATHUTIL_H
#define MIDIPIANO_UTIL_MATHUTIL_H

#include <cmath>

// Vereiste 36: 36.everything in one or more self-made useful namespace(s)
// Alle code zit in namespace midipiano, opgesplitst per laag: midipiano::dsp (klankopwekking),
// midipiano::midi (noten en bronnen), midipiano::preset (klankinstellingen bewaren),
// midipiano::audio (geluidskaart), midipiano::util (hulpcode) en midipiano::app (Qt-app). Zo botsen
// namen als "Filter" of "Patch" nooit met Qt of miniaudio, en zie je aan de naam tot welke laag
// een klasse hoort.
namespace midipiano::util {

constexpr float kPi = 3.14159265358979f;

// Vereiste 33: 33.correct usage of inline function
// midiToHz() wordt voor elke noot aangeroepen en is maar een regel lang. Omdat ze in de header
// staat, moet ze inline zijn: anders zou elke .cpp die de header include een eigen definitie
// krijgen en geeft de linker een "multiple definition"-fout.
// MIDI-noot 69 is A4 = 440 Hz, elke noot hoger is een halve toon (factor 2^(1/12)).
inline float midiToHz(int note) {
  return 440.0f * std::pow(2.0f, (static_cast<float>(note) - 69.0f) / 12.0f);
}

}  // namespace midipiano::util

#endif  // MIDIPIANO_UTIL_MATHUTIL_H

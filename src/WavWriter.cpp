#include "WavWriter.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace midipiano {

namespace {
// Schrijft een getal als little-endian bytes, zoals het WAV-formaat vraagt.
template <typename T>
void writeLE(std::ofstream& out, T value) {
  out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}
}  // namespace

void WavWriter::write(const std::string& path, const std::vector<float>& samples, int sampleRate) {
  std::ofstream out(path, std::ios::binary);
  if (!out) throw std::runtime_error("kan " + path + " niet openen om te schrijven");

  const auto dataBytes = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));
  out.write("RIFF", 4);
  writeLE<std::uint32_t>(out, 36 + dataBytes);
  out.write("WAVEfmt ", 8);
  writeLE<std::uint32_t>(out, 16);                // grootte van het fmt-blok
  writeLE<std::uint16_t>(out, 1);                 // PCM
  writeLE<std::uint16_t>(out, 1);                 // mono
  writeLE<std::uint32_t>(out, sampleRate);
  writeLE<std::uint32_t>(out, sampleRate * 2);    // bytes per seconde
  writeLE<std::uint16_t>(out, 2);                 // bytes per sample
  writeLE<std::uint16_t>(out, 16);                // bits per sample
  out.write("data", 4);
  writeLE<std::uint32_t>(out, dataBytes);
  for (float s : samples) {
    writeLE<std::int16_t>(out, static_cast<std::int16_t>(std::clamp(s, -1.0f, 1.0f) * 32767.0f));
  }
}

}  // namespace midipiano

#ifndef MIDIPIANO_WAVWRITER_H
#define MIDIPIANO_WAVWRITER_H

#include <string>
#include <vector>

namespace midipiano {

// Schrijft mono 16-bit PCM samples als .wav-bestand, af te spelen in elke mediaspeler.
class WavWriter {
 public:
  static void write(const std::string& path, const std::vector<float>& samples, int sampleRate);
};

}  // namespace midipiano

#endif  // MIDIPIANO_WAVWRITER_H

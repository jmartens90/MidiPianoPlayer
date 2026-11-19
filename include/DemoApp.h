#ifndef MIDIPIANO_DEMOAPP_H
#define MIDIPIANO_DEMOAPP_H

#include <string>

namespace midipiano {

// Speelt een korte melodie met twee klanken en schrijft het resultaat naar een WAV-bestand.
class DemoApp {
 public:
  void run(const std::string& wavPath);
};

}  // namespace midipiano

#endif  // MIDIPIANO_DEMOAPP_H

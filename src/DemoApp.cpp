#include "DemoApp.h"

#include <iostream>
#include <memory>
#include <vector>

#include "Voice.h"
#include "WavWriter.h"
#include "Waveforms.h"

namespace midipiano {

void DemoApp::run(const std::string& wavPath) {
  constexpr int kSampleRate = 44100;
  constexpr int kSamplesPerNote = kSampleRate / 4;  // 4 noten per seconde

  // Twee stemmen met dezelfde klasse maar een andere klank (compositie).
  Voice sine(std::make_unique<SineOscillator>());
  Voice square(std::make_unique<SquareOscillator>());
  square.setVolume(0.2f);  // een blokgolf klinkt veel luider dan een sinus

  // Vereiste 5: 5.useful and correct polymorphism (explain why)
  // De lus hieronder speelt de melodie eerst met de sinus- en dan met de blokgolfstem. Voice roept
  // telkens oscillator_->next() aan; via de virtuele sample() kiest C++ tijdens het draaien de juiste
  // golfvorm, zonder dat Voice of deze lus weet welke oscillator erin zit.
  const std::vector<int> melody = {60, 62, 64, 65, 67, 65, 64, 62, 60};  // C D E F G F E D C
  std::vector<float> samples;
  for (Voice* voice : {&sine, &square}) {
    for (int note : melody) {
      voice->noteOn(note);
      for (int i = 0; i < kSamplesPerNote; ++i) samples.push_back(voice->render(kSampleRate));
      voice->noteOff();
    }
  }

  WavWriter::write(wavPath, samples, kSampleRate);
  std::cout << "Geschreven: " << wavPath << " (" << samples.size() / kSampleRate << " s)\n";
}

}  // namespace midipiano

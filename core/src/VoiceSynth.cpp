#include "midipiano/dsp/VoiceSynth.h"

#include <algorithm>

namespace midipiano::dsp {

VoiceSynth::VoiceSynth(float sampleRate) : sampleRate_(sampleRate) {
  // Alle wavetables nu al berekenen (een paar honderd ms), zodat het wisselen van golfvorm tijdens
  // het spelen geen hapering in de audio geeft.
  for (int i = 0; i < static_cast<int>(Waveform::Count); ++i) OscillatorFactory::create(static_cast<Waveform>(i));
  applyPatch(preset::Patch());
}

// Vereiste 29: 29.useful member function
// noteOn() is het hart van de polyfonie: het zoekt een vrije stem, en als alle 8 bezet zijn, neemt
// het de oudste (voice stealing). Zo kan je akkoorden spelen en blijft een nieuwe noot altijd hoorbaar.
void VoiceSynth::noteOn(std::uint8_t note, std::uint8_t velocity) {
  Voice* voice = findFreeVoice();
  if (voice == nullptr) voice = &oldestVoice();
  voice->start(note, velocity, ++clock_);
}

void VoiceSynth::noteOff(std::uint8_t note) {
  for (Voice& voice : voices_) {
    if (voice.isHeld() && voice.note() == note) voice.release();
  }
}

void VoiceSynth::allNotesOff() {
  for (Voice& voice : voices_) voice.release();
}

void VoiceSynth::render(float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    float mix = 0.0f;
    for (Voice& voice : voices_) mix += voice.render(sampleRate_);
    out[i] = mix * 0.6f;
  }
}

std::size_t VoiceSynth::activeVoiceCount() const {
  return static_cast<std::size_t>(
      std::count_if(voices_.begin(), voices_.end(), [](const Voice& v) { return v.isActive(); }));
}

void VoiceSynth::applyPatch(const preset::Patch& patch) {
  for (Voice& voice : voices_) {
    voice.setWaveform(patch.waveform());
    voice.setWavetablePosition(patch.get("position"));
    voice.envelope().setAttack(patch.get("attack"));
    voice.envelope().setDecay(patch.get("decay"));
    voice.envelope().setSustain(patch.get("sustain"));
    voice.envelope().setRelease(patch.get("release"));
    voice.filter().setCutoff(patch.get("cutoff"));
    voice.filter().setResonance(patch.get("resonance"));
    voice.filter().setDrive(patch.get("drive"));
    voice.setFilterEnvAmount(patch.get("envmod"));
  }
}

// Vereiste 46: 46.useful usage of nullptr
// findFreeVoice() geeft nullptr terug als alle 8 stemmen bezig zijn. noteOn() test daarop en kiest
// dan de oudste stem. Zo betekent nullptr duidelijk "geen vrije stem" in plaats van een magisch getal.
Voice* VoiceSynth::findFreeVoice() {
  for (Voice& voice : voices_) {
    if (!voice.isActive()) return &voice;
  }
  return nullptr;
}

Voice& VoiceSynth::oldestVoice() {
  return *std::min_element(voices_.begin(), voices_.end(), [](const Voice& a, const Voice& b) {
    return a.startedAt() < b.startedAt();
  });
}

}  // namespace midipiano::dsp

# midipiano_core: de synth zonder Qt. Wordt gebruikt door de app en door de tests.
add_library(midipiano_core STATIC
  src/AudioOutput.cpp
  src/DemoSequencer.cpp
  src/Envelope.cpp
  src/LowPassFilter.cpp
  src/MidiMessage.cpp
  src/Patch.cpp
  src/PresetStore.cpp
  src/Reverb.cpp
  src/Synth.cpp
  src/Voice.cpp
  src/SoundFont.cpp
  src/VoiceSynth.cpp
  src/Waveforms.cpp
  src/Wavetable.cpp
)
target_include_directories(midipiano_core PUBLIC include)
target_link_libraries(midipiano_core PUBLIC miniaudio tinysoundfont)
target_compile_features(midipiano_core PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(midipiano_core PRIVATE /W4)
else()
  target_compile_options(midipiano_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# De geluidsmotor moet elke paar milliseconden een blok audio afleveren, anders kraakt het. In een
# Debug-build (de standaard in Qt Creator, ook voor Android) zou alles zonder optimalisatie draaien
# en is een gsm daar te traag voor. Daarom altijd -O2 voor de motor en de twee C-bibliotheken
# (debuggen kan nog steeds: -g blijft staan). Niet voor MSVC: /O2 botst daar met de Debug-controles.
if(NOT MSVC)
  target_compile_options(midipiano_core PRIVATE -O2)
  target_compile_options(tinysoundfont PRIVATE -O2)
  target_compile_options(miniaudio PRIVATE -O2)
endif()

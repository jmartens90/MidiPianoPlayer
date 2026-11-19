add_executable(midipiano_tests main.cpp DspTests.cpp MidiPresetTests.cpp SoundFontTests.cpp
  EffectsTests.cpp
  BleMidiTests.cpp)
target_link_libraries(midipiano_tests PRIVATE midipiano_core)
target_compile_definitions(midipiano_tests PRIVATE
  MIDIPIANO_SOUNDFONT="${PROJECT_SOURCE_DIR}/assets/soundfonts/instruments.sf2")
add_test(NAME midipiano_tests COMMAND midipiano_tests)

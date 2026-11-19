add_executable(midipiano_tests main.cpp DspTests.cpp MidiPresetTests.cpp)
target_link_libraries(midipiano_tests PRIVATE midipiano_core)
add_test(NAME midipiano_tests COMMAND midipiano_tests)

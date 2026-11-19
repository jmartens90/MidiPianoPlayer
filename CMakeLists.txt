cmake_minimum_required(VERSION 3.16)
project(MidiPianoPlayer VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MIDIPIANO_BUILD_APP "Build the Qt app" ON)
option(MIDIPIANO_BUILD_TESTS "Build the unit tests" ON)
if(ANDROID)
  set(MIDIPIANO_BUILD_TESTS OFF)  # tests draaien op de PC, niet op de telefoon
endif()

add_subdirectory(third_party/miniaudio)
add_subdirectory(third_party/tinysoundfont)
add_subdirectory(core)

if(MIDIPIANO_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(MIDIPIANO_BUILD_APP)
  add_subdirectory(app)
endif()

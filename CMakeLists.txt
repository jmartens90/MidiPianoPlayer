cmake_minimum_required(VERSION 3.16)
project(MidiPianoPlayer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(midipiano
  src/main.cpp
  src/DemoApp.cpp
  src/Voice.cpp
  src/Waveforms.cpp
  src/WavWriter.cpp
)
target_include_directories(midipiano PRIVATE include)
if(MSVC)
  target_compile_options(midipiano PRIVATE /W4)
else()
  target_compile_options(midipiano PRIVATE -Wall -Wextra -Wpedantic)
endif()

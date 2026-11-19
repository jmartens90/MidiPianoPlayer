# miniaudio (https://miniaud.io, v0.11.26): one C file that talks to the sound card on Windows
# (WASAPI), macOS (Core Audio), Linux (PulseAudio/ALSA) and Android (AAudio/OpenSL ES).
add_library(miniaudio STATIC miniaudio.c)
target_include_directories(miniaudio SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED)
target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX AND NOT APPLE)
  target_link_libraries(miniaudio PUBLIC m)
endif()

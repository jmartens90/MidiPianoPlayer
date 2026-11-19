# TinySoundFont (https://github.com/schellingb/TinySoundFont, MIT): plays SoundFont (.sf2) samples.
add_library(tinysoundfont STATIC tsf_impl.c)
target_include_directories(tinysoundfont SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(UNIX)
  target_link_libraries(tinysoundfont PUBLIC m)
endif()

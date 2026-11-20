find_package(Qt6 6.4 REQUIRED COMPONENTS Gui Quick QuickControls2 OpenGL)
# Bluetooth MIDI heeft de module Qt Bluetooth nodig (in de Qt-installer: "Qt Connectivity"). Zonder
# die module bouwt de app ook, alleen zonder Bluetooth.
find_package(Qt6 6.4 QUIET COMPONENTS Bluetooth)
qt_standard_project_setup()

# Nieuwere Qt-versies (6.5+) waarschuwen anders over de standaard QML-resourcemap.
if(QT_KNOWN_POLICY_QTP0001)
  qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
  qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(MidiPianoPlayer
  main.cpp
  Application.h
  Application.cpp
)

qt_add_qml_module(MidiPianoPlayer
  URI MidiPiano
  VERSION 1.0
  QML_FILES
    qml/Main.qml
    qml/PianoKeyboard.qml
    qml/Knob.qml
    qml/KnobStrip.qml
    qml/SequencerPanel.qml
  SOURCES
    SynthController.h
    SynthController.cpp
    BleMidiSource.h
    BleMidiSource.cpp
    WaveformView.h
    WaveformView.cpp
)

# De pianosamples (6 MB) worden als Qt-resource in de app gebakken, zodat ze ook in de APK zitten.
qt_add_resources(MidiPianoPlayer "soundfonts"
  PREFIX "/sounds"
  BASE "${PROJECT_SOURCE_DIR}/assets/soundfonts"
  FILES "${PROJECT_SOURCE_DIR}/assets/soundfonts/instruments.sf2"
)

target_link_libraries(MidiPianoPlayer PRIVATE
  midipiano_core
  Qt6::Gui
  Qt6::Quick
  Qt6::QuickControls2
  Qt6::OpenGL
)
if(TARGET Qt6::Bluetooth)
  target_link_libraries(MidiPianoPlayer PRIVATE Qt6::Bluetooth)
  target_compile_definitions(MidiPianoPlayer PRIVATE MIDIPIANO_HAS_BLUETOOTH)
  message(STATUS "Bluetooth MIDI: on")
else()
  message(STATUS "Bluetooth MIDI: off (install the Qt Connectivity / Qt Bluetooth module to enable it)")
endif()

# Vereiste 60: 60.a nice extra that you think that should deserve grading (stuff you put time in and is not rewarded by an item above)
# Dezelfde code bouwt als desktop-app en als Android-app (APK) voor de telefoon: kies in Qt Creator
# een Android-kit en het project draait op je gsm, met een multitouch-pianotoetsenbord (tien vingers,
# glijden over de toetsen, aanslagsterkte uit de plaats waar je de toets raakt) en lage-latency audio
# via AAudio. Zie README "Android".
set_target_properties(MidiPianoPlayer PROPERTIES
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
  # Eigen AndroidManifest.xml: de app start meteen liggend (sensorLandscape). Draaien tijdens het
  # opstarten liet het Qt-venster op zijn staande maat staan.
  QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
)

if(MSVC)
  target_compile_options(MidiPianoPlayer PRIVATE /W4)
else()
  target_compile_options(MidiPianoPlayer PRIVATE -Wall -Wextra)
endif()

# Installeren is alleen zinvol op de desktop; op Android maakt Qt een APK.
if(NOT ANDROID)
  include(GNUInstallDirs)
  install(TARGETS MidiPianoPlayer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

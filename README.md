# MidiPianoPlayer

A softsynth in C++17. Later it becomes a Qt app for Android and desktop that a PSoC 6 sequencer plays over
BLE MIDI. This first version plays a short melody with a sine voice and a square voice and writes it to
`demo.wav`.

<!-- Vereiste 20: 20.working build manual as readme on GitHub (project must be possible to build from scratch on a clean PC) -->
## Build manual (clean PC)

### Windows

1. Install [Qt Online Installer](https://www.qt.io/download-qt-installer): pick Qt Creator, CMake, Ninja and MinGW.
   (Qt itself is not needed yet, but it will be for the app.)
2. In Qt Creator: **File → Open File or Project** → choose `CMakeLists.txt` → pick the MinGW kit → **Run**.
3. `demo.wav` appears in the build folder. Open it in any media player.

### Linux / macOS / terminal

```
sudo apt install g++ cmake        # macOS: xcode-select --install && brew install cmake
git clone https://github.com/jmartens90/MidiPianoPlayer.git
cd MidiPianoPlayer
cmake -B build
cmake --build build
cd build && ./midipiano            # writes demo.wav
```

## Requirements (Vereisten)

Each requirement is marked in the code with `Vereiste <n>:`. Search for it with Ctrl+F.

| # | Requirement | Where |
|---|---|---|
| 3 | encapsulation | `include/Voice.h` |
| 4 | inheritance | `include/Waveforms.h` |
| 5 | polymorphism | `src/DemoApp.cpp` |
| 6 | object composition | `include/Voice.h` |
| 8 | abstract base class | `include/Oscillator.h` |
| 9 | virtual function | `include/Oscillator.h` |
| 11 | clean main | `src/main.cpp` |
| 20 | build manual | this README |

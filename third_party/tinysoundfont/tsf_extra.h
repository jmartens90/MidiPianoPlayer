/* Own addition to TinySoundFont for MidiPianoPlayer (implemented in tsf_impl.c). */
#ifndef TSF_EXTRA_H
#define TSF_EXTRA_H

#include "tsf.h"

#ifdef __cplusplus
extern "C" {
#endif

int tsf_held_voice_count(tsf* f, int preset_index, int key);

#ifdef __cplusplus
}
#endif

#endif

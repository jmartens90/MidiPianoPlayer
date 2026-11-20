#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "tsf_extra.h"

/* Own addition (MidiPianoPlayer): number of voices of this key that are still held, i.e. not yet
   in their release phase. tsf_note_off() only releases the oldest strike of a key per call. */
int tsf_held_voice_count(tsf* f, int preset_index, int key)
{
	int count = 0;
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++)
		if (v->playingPreset == preset_index && v->playingKey == key && v->ampenv.segment < TSF_SEGMENT_RELEASE)
			count++;
	return count;
}

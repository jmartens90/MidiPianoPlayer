# Test plan

<!-- Vereiste 53: 53.test-driven development (= written test plan or unit tests) -->
<!-- Dit testplan en de unit tests in tests/ samen: wat automatisch getest wordt en wat met de hand. -->

The synth core has no Qt and no sound card in it, so almost everything is tested automatically on a PC.
What needs a screen or ears is tested by hand with the checklist below before every evaluation.

## Automatic: unit tests (`tests/`)

Run them with `ctest --test-dir build` or by running `build/tests/midipiano_tests`. GitHub Actions runs
them on every push.

| Area | Test | What it proves |
|---|---|---|
| Pitch | `midiToHz_gives_concert_pitch` | note 69 = 440 Hz, one octave = double frequency |
| Oscillators | `waveforms_have_expected_values`, `oscillator_completes_one_cycle_per_period` | each waveform formula, phase wraps once per period |
| Polymorphism | `factory_returns_right_oscillator_through_base_pointer` | the right subclass answers through `Oscillator*`; `nullptr` for an invalid waveform |
| Envelope | `envelope_runs_through_adsr`, `envelope_default_constructor_forwards` | attack reaches 1, decay stops at sustain, release ends in Idle |
| Voices | `synth_allocates_and_steals_voices` | 11 notes on 8 voices: no crash, all 8 busy (oldest stolen) |
| Voices | `synth_goes_silent_after_note_off` | sound while held, silence after the release time |
| MIDI | `synth_handles_midi_messages`, `midi_note_on_with_velocity_zero_is_note_off`, `midi_message_copy_and_print` | Note On/Off, "velocity 0 = off", All Notes Off, a message is 3 bytes |
| Scope | `scope_keeps_last_samples`, `ring_buffer_wraps_in_order` | the scope shows the newest 1024 samples in order |
| Presets | `patch_clamps_and_chains`, `patch_copy_is_independent` | values stay in range, unknown names throw, copies are independent |
| Files | `preset_round_trip`, `preset_errors_are_reported`, `factory_presets_are_installed_once` | save + load gives the same patch; missing, corrupt or unknown files give a `PresetError` |
| Threads | `sequencer_works_through_base_class`, `sequencer_stops_quickly_even_when_slow` | the sequencer thread sends matching on/off pairs and stops within 200 ms |

## By hand: app checklist

Do this on the desktop build and on an Android phone.

| # | Step | Expected |
|---|---|---|
| 1 | Start the app | Window opens, no error in the status line, "Init" preset shown |
| 2 | Click or touch a white key, then a black key | Tone sounds, key lights up, scope shows the waveform |
| 3 | Hold three keys (three fingers, or A + D + G on the PC keyboard) | Chord sounds, voice counter shows 3 / 8 |
| 4 | Slide a finger across the keys | Each key you pass plays (glissando), no stuck notes after lifting |
| 5 | Touch a key high, then low | The low touch is louder |
| 6 | Switch Sine / Saw / Square / Triangle while holding a key | Sound and scope change right away |
| 7 | Turn Attack to 1 s, Release to 2 s | Notes fade in and ring out |
| 8 | Turn Cutoff down to 200 Hz | Saw sounds dull, the scope shows a rounded wave |
| 9 | Press "Play demo" | Arpeggio plays, keys light up; "Stop demo" stops it at once |
| 10 | Octave - / + | Key labels change (C2..C6), notes go down / up |
| 11 | Save as "Test", pick "Pluck", then pick "Test" again | The knobs jump back to the saved values |
| 12 | Close and reopen the app | "Test" is still in the preset list |
| 13 | Put "cutoff=abc" in Test.preset with a text editor, then load it | Orange error message with the line number, the app keeps running |
| 14 | Run without a sound card (or with it disabled) | Status shows "No sound: ...", the app keeps running |

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
| Samples | `soundfont_loads_the_piano_presets`, `piano_plays_and_stops_through_the_synth`, `instruments_share_one_soundfont` | the SoundFont loads, the piano sounds and stops, instruments share one font |
| Effects | `sustain_pedal_*`, `reverb_*`, `resonant_filter_rings_and_can_self_oscillate` | pedal holds notes, reverb tail and predelay, stereo width, stable at maximum decay, filter self-oscillates at full resonance |
| Wavetables | `wavetable_morphs_and_avoids_aliasing`, `every_wavetable_waveform_plays_through_a_voice` | Position morphs, high notes use a band-limited table, every table sounds |
| Delay | `delay_repeats_the_sound_at_the_right_time`, `delay_syncs_to_the_sequencer_tempo` | echoes arrive at the set time and get softer; sync uses the tempo |
| Step sequencer | `step_sequencer_plays_active_steps_in_time` | only active steps play, accent = velocity 127, transpose works, stop is immediate |
| Hanging notes | `restruck_piano_note_never_hangs`, `switching_instrument_releases_held_notes` | a key struck twice (pedal, sequencer) never keeps sounding |
| Bluetooth | `ble_parser_*` (5 tests) | BLE MIDI packets: several messages, running status (also across packets), realtime and SysEx skipped, bad packets ignored, 13-bit timestamps and their wraparound |
| Timing | `scheduled_note_starts_on_the_exact_sample`, `ble_clock_sync_removes_bluetooth_jitter` | a scheduled note starts on its exact sample inside an audio block; notes sent 10 ms apart but received with random radio delay are played exactly 10 ms apart |

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
| 15 | Settings → Search devices, with a BLE MIDI device (PSoC 6) switched on | The device appears at the top, marked "• MIDI" |
| 16 | Tap the device, then play notes on it | Status shows "Connected to ...", notes sound and light up, the message counter rises |
| 17 | Switch the device off while connected | Status shows "... disconnected", no notes keep hanging |
| 18 | Search on a PC without Bluetooth | Orange "Bluetooth error: ..." message, the app keeps running |
| 19 | Sequencer tab → Play, tap steps on and off | Pattern plays, the white frame walks over the steps, changes are heard right away |
| 20 | ▴ More on the Sequencer tab, tap cells in the grid | The note on that step changes; the bottom row toggles accents |
| 21 | Effects tab: Delay Mix up, Sync on, 1/8 . | Echoes on the beat of the sequencer tempo; changing Tempo changes the echo |
| 22 | On Android: press Home while a note or the sequencer plays | Sound stops at once; back in the app, playing works again |
| 23 | Settings → Buffer: Auto, 5, 10, 20 ms while playing | The audio line shows the new buffer; 5 ms responds fastest, nothing crackles at 10 ms or Auto; the choice is kept after a restart |
| 24 | Play fast notes from the PSoC 6 with "Steady Bluetooth timing" on, then off | On: the rhythm is even. Off: notes come slightly sooner but with uneven timing |
| 25 | Sequencer tab → Key on, hold C2, then D2, then let go | C2 plays the pattern from step 1, D2 restarts it a tone higher, letting go stops it |
| 26 | Load a preset on a phone | "Loaded ..." appears above the keyboard for 3 seconds and does not cover the header |
| 27 | On Android: open the app, let the phone go to sleep, wake it and unlock | Playing works right away, no restart needed |
| 28 | Effects tab: Reverb Mix up, play a chord with headphones, turn Width from 1 to 0 and Size from 0 to 1 | Wide: the tail fills both ears; Width 0: it sits in the middle. Size 0 sounds like a small room, 1 like a hall |

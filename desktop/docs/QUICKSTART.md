# Pulse Foundry — getting started

This is a modified, unofficial software port. It is not an Erica Synths product.

## Install on Windows

1. Run `PulseFoundry-0.1.0-Windows-x64-Setup.exe` on 64-bit Windows 10 or 11. Administrator access is needed to install the VST3 into the system VST3 directory.
2. The app installs into `C:\Program Files\Pulse Foundry`. The VST3 bundle installs into `C:\Program Files\Common Files\VST3\Pulse Foundry.vst3`.
3. The installer is unsigned. Its first release has not been runtime-tested on Windows; use the acceptance checks in `VALIDATION.md` before relying on it in a session.
4. The complete corresponding source is installed in the app's `Source` folder. Uninstall using Windows Settings or the Start-menu shortcut. User-created projects are retained.

## Standalone

Open **Audio / MIDI** and choose the audio device, enabled MIDI inputs, sample rate and buffer size. For an ASIO-capable interface, install its manufacturer's ASIO driver and select it here. WASAPI is also available. Start with a 256-sample buffer.

Click **PLAY** to run the initial groove, or click a track name to audition and select its sound. With the editor focused, **A S D F G H J** trigger the seven tracks and **Space** toggles playback. Number boxes and text-entry controls take keyboard focus; click the panel background to return to performance keys.

The tempo is local to the standalone app. Swing delays odd sixteenth notes by up to 49% of a step. Master gain affects the master stereo output; separate voice outputs remain at their own voice levels.

## Ableton Live 12

Enable **Use VST3 Plug-In System Folders** in Live's plug-in preferences and rescan. Put **Pulse Foundry** on a MIDI track. The plugin follows Live's tempo, position and Play/Stop transport. Its own PLAY button and local BPM are disabled in the plugin.

Choose **Patterns + MIDI** to run its internal sequencer and accept MIDI. Choose **MIDI only** to sequence it entirely from Live clips. A MIDI hit takes priority over an internal hit for the same physical voice within one 64-sample native processing block, avoiding a simultaneous double trigger. Multiple MIDI hits for one track inside that window coalesce to the last hit.

MIDI note numbers (octave names vary between DAWs):

| Note number | Track |
|---|---|
| 36 | Drum 1 |
| 38 | Drum 2 |
| 43 | Drum 3 |
| 40 | Snare |
| 49 | Cymbal |
| 42 | Closed hat |
| 46 | Open hat |

Velocity is passed to the original synthesis engine. The mapped MIDI notes trigger the sound at its programmed base pitch; use sound tuning or per-step NOTE to pitch it. Note-off does not cut off a percussion envelope. CC 120/123 clears voices and effect tails. Default MIDI input is omni; the MIDI-channel parameter is available to the host.

For individual outputs, create additional Audio tracks in Live, choose the instrument track as **Audio From**, then select Drum 1, Drum 2, Drum 3, Snare, Cymbal or Hi-hats in the sub-output selector. Enable monitoring as appropriate. These are stereo, post-voice-level/pan/drive/decimation, pre-master-effect stems. Open/closed hats share one output and choke each other. Use the master mix or the stems according to the mix you want; monitoring both adds the same drums twice.

## Sound and pattern editing

Choose a sound section to reveal its knobs. **All Parameters** exposes the complete supported sound-parameter list. Values use firmware units. Modulation destination values are desktop parameter IDs, not hardware menu indices; the Modulation page provides named destination selectors. A value of 0 disables a modulation target.

Click grid cells to toggle them and select the step. The inspector below the grid edits its velocity, probability, note, flam, timing shift and two parameter locks. Select a different track by clicking its name. Open/closed hat sound controls refer to the same synthesis voice, with separate envelope decay values.

Use the four page buttons for steps 1–64. **Length** is independent per track. **Rotate** offsets that track's playback. **M** mutes internal sequencing on a track; manual/MIDI triggering remains available. **Euclidean** fills the selected track using Length, Hits and Rotate, replacing its existing steps. **Clear Track** clears it.

For song playback, type pattern numbers separated by commas into the chain field, press Enter and enable **SONG**. A pattern plays for the longest track rounded up to whole bars. The chain loops. Song position follows the timeline from PPQ zero. In song mode, pattern selection remains the edit selection; the song determines the sounding pattern.

**Save Project** stores the whole sound engine configuration, playback settings, 64 patterns and song. Live also stores this state inside its Set. **Save Kit** stores the sound parameters; loading a kit preserves the current sequence and transport settings. Files use `.pfproject` and `.pfkit`; hardware kit/pattern import is not implemented.

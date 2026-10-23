# Validation record — 2026-10-23

Release: Pulse Foundry 0.1.0. This is a first desktop port, with the scope differences described in `PORT.md`.

## Completed in this environment

- Native Linux Release builds: standalone app, VST3, headless renderer and test executables, using GCC 14 and JUCE 7.0.12.
- Windows x64 cross-builds: standalone PE executable and VST3 PE module using MinGW-w64 GCC 14.2 and JUCE 7.0.12, with ASIO enabled. The PE import tables were inspected: only Windows system DLL dependencies remain; GCC, libstdc++ and winpthread runtimes are statically linked.
- NSIS 3.11 compiled the Windows installer containing both binaries, licence notices and the full corresponding-source archive. The source ZIP was checked for CRC integrity and required firmware/JUCE/ASIO/build-script contents. Installer execution on Windows remains unverified.
- CTest: all three test executables passed (`native_engine`, `sound_parameter_boundaries`, `plugin_state_and_routing`).
- Every original voice produces finite, nonzero audio; voice stems remain isolated after panic; all four enabled effects produce finite, bounded master audio at 44.1/48/96 kHz.
- Four-on-floor timing, flams, 0% probability, Euclidean hit counts, MIDI priority, two independent engine instances and multiple sample rates are exercised.
- Complete project state roundtrips preserve sound, patterns, step locks/timing, per-track lengths and song data. Corrupted state is rejected. Kit loading restores sound while retaining sequence selection and standalone tempo.
- Plugin tests exercise Live-style PPQ/BPM transport, stop and seek, MIDI-only mode, all seven stereo buses and individual-output isolation. They do not execute Ableton itself.
- Allocation counters detect no C++ heap allocation during the native engine render loop or during the complete plugin callback test. The callback contains no file I/O, UI calls or mutex acquisition. This is a bounded test, not a formal proof for every host interaction.
- AddressSanitizer/UndefinedBehaviorSanitizer parameter-boundary stress found an upstream wavetable end-read. After adding circular interpolation reads, the same test passed. Shift, signed-overflow and float-cast checks are excluded for inherited fixed-point DSP wrap/conversion idioms. LeakSanitizer was disabled because this executor's tracing prevents it from running; no leak-validation claim is made.
- An eight-second, stereo, 48 kHz PCM demo WAV was rendered. The editor was instantiated and visually inspected from a 1280×900 Linux snapshot.

## Checks requiring your Windows machine

1. Install the unsigned installer on Windows 10/11 x64; confirm the app, VST3 bundle, Start-menu shortcuts and corresponding-source archive are installed. Check uninstall and reinstall; user project files should remain intact.
2. Launch the standalone app. In Audio / MIDI select your manufacturer's ASIO driver and a 256-sample buffer. Test 44.1, 48 and 96 kHz; test supported 64–512-sample buffers. Check for dropouts at the configuration you intend to use. WASAPI should also be checked.
3. Confirm the MIDI controller input appears, all seven mapped notes trigger, velocity responds and the A–J performance keys work when the panel has focus. Verify sample-rate/device switching while stopped.
4. In Ableton Live 12 enable the VST3 system folder and rescan. Load the instrument and open/close/resize the editor. Verify that Live's transport controls playback, the BPM follows Live and the standalone transport controls are disabled.
5. Run a four-on-floor pattern at several tempos, including a tempo change, loop wrap and relocation to a new bar. Try song mode and independently sized tracks. The documented native-block timing quantisation is expected.
6. Switch to MIDI only and play a Live MIDI clip; the internal pattern must stop contributing. In Patterns + MIDI, send a mapped hit on an existing pattern hit and check for a single trigger for that physical voice in the same native block.
7. Route the six stereo voice sub-outputs to separate Live tracks. Verify no unintended voices appear in those stems. Open and closed hats intentionally share one output. Compare stems to the master; master FX and master gain intentionally affect the master mix only.
8. Automate tuning, envelope, filter, level and FX controls. Save a Set with two instrument instances and different kits/patterns, close Live, reopen and confirm independent state restoration. Stop playback before loading a whole project for a clean state transition.
9. Save/reload `.pfproject` and `.pfkit` files. A kit load must preserve patterns and tempo. Confirm that a malformed or truncated project is rejected.
10. Compare against hardware reference recordings if exact sound matching matters. Hardware RNG samples, desktop resampling, codec/output circuitry and the documented interpolation correction prevent a claim of bit-exact parity.

## Unverified and limited

Ableton Live 12, Windows device drivers and installer execution are unavailable in this Linux environment. Windows cross-compilation establishes that the binaries can be built, not that they have passed those runtime checks. The installer is unsigned. Hardware file compatibility is deliberately out of scope; kit morphing, live recording and external MIDI-clock I/O are also absent in this version. See `PORT.md` for the complete differences.

# Source inspection and native port

Source: `PaoloSupernova/lxr-desktop-app`, commit `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`, firmware 1.7. It targets STM32F765VG/Cortex-M7, not a computer application. There were no repository AGENTS.md files. The original CMake requires an ARM bare-metal toolchain, HAL/CMSIS, DMA/I2S, hardware RNG, GPIO, timers, USB/UART MIDI, FatFs and SD storage.

## Reused code

`tools/port_firmware.py` reads the original files without modifying them and generates a C++ `Firmware` class. Functions become inline member functions and formerly global mutable variables become instance fields. All oscillators, voice structures, envelopes, filters, modulation nodes and effect buffers are owned by an instance. Immutable wave/sample tables are shared. No global active-instance pointer or audio mutex is used, so instruments can render on different host threads.

The original three drum voices, snare, cymbal and shared hi-hat are reused, including synthesis modes, wavetables, transient tables, FM, filters, amplitude and pitch envelopes, LFOs, velocity modulation, voice distortion and decimation. The original CC-to-parameter switch and ParameterArray mapping preserve parameter conversion behaviour. Modulation targets are assigned directly from stable desktop sound-parameter IDs; the hardware front-panel menu translation is replaced.

The original distortion, ring modulator, compressor and delay/ping-pong delay are reused. Reverb is disabled in the upstream firmware (`USE_REVERB=0`) and is not enabled by this port. Hardware output-jack selection is replaced by a complete stereo master and six stereo stems.

## Replacements and portability changes

- Physical audio/MIDI drivers, clocks, I/O, LCD/front-panel code and FatFs are replaced by JUCE and a desktop model. No STM32 platform code is linked.
- The sound engine runs at the original `REAL_FS=44140` Hz and uses 64-frame control/audio blocks. A streaming linear interpolator converts to the host/device rate. This preserves original DSP coefficients and envelope cadence but introduces resampling differences. High-frequency attenuation from linear interpolation is a known limitation.
- MIDI and pattern hits are applied at native block boundaries, with up to approximately 1.45 ms quantisation plus one interpolation sample. The host's audio buffer/device latency is additional. This is not a sample-exact port of the firmware's hardware scheduling.
- ARM saturating SIMD lane operations, bit packing and count-leading-zero are replaced with portable scalar equivalents. Delay memory retains IEEE binary16 quantisation through the portable `Half` type.
- The hardware RNG becomes an independent xorshift generator per engine. Noise and random modulation will not produce the same individual samples as the hardware.
- C99 variable-sized scratch arrays become fixed-size stack arrays for MSVC and predictable real-time use. Section attributes and register/inline qualifiers are removed as required by native C++.
- Interpolation in upstream 1024-sample wavetables can read element 1024, outside the table. The desktop generator wraps this read to element zero. ASan found the original access during boundary testing. This correction can change a small number of samples at waveform wraparound.
- Signed transient bytes and arithmetic shifts are expressed explicitly where needed for C++/MSVC portability. Strict aliasing optimisations are disabled for the inherited packed 16-bit processing.

## Sequencer

The original sequencer directly couples millisecond hardware ticks, MIDI clock/UART, recording, SD bank loading and front-panel state. The desktop sequencer replaces that scheduling layer with host PPQ and the standalone sample clock. It preserves the seven-track/six-voice relationship, independent 1–64-step lengths, rotations, 64 patterns, swing, per-step probability/note/velocity, 1/2/3/4/6/8 flams, signed timing offsets and two sound parameter locks.

Step state is packed into lock-free 64-bit atomics. Sequencer edits do not block the render thread. Pattern changes select a bank directly. A song chain uses up to 64 entries; each entry spans the longest track rounded to whole bars. Probability uses a deterministic hash of absolute step, pattern and track; results repeat when returning to the same timeline position. Negative microtiming before PPQ zero cannot sound before playback begins. Parameter locks last until the next master sixteenth-note boundary, then restore the unautomated base value.

The Euclidean generator replaces the hardware UI's destructive generation workflow with direct filling of the selected track. Hardware song/program logic, pattern-bank preloading, kit morphing, live step/automation recording, external MIDI-clock following/output and hardware project file compatibility are not implemented. These are explicit scope gaps relative to complete hardware feature parity.

## State and threading

JUCE APVTS provides stable host parameter IDs. The callback caches raw atomic parameter pointers and never traverses a ValueTree. Audio state is separate from serialised UI state. UI audition uses a fixed SPSC queue. MIDI bytes are inspected without constructing allocating SysEx messages. All buffers used for rendering are allocated in construction/prepare or are fixed-size stack/member buffers.

Project loads parse and validate a versioned ValueTree, finite parameter ranges, exact sequence payload size, pattern/song ranges and packed-step fields before changing state. State callbacks and file I/O are non-render operations. Atomic sequence fields allow race-free reads during a restore, but a running restore is not an atomic transaction across the entire project; stop transport when loading a project for a clean musical transition.

No hardware sound reference recordings or Windows/Ableton runtime are available in this environment. Source reuse and automated rendering checks are evidence of the implementation, not proof of identical hardware sound or production readiness.

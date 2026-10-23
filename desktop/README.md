# Pulse Foundry 0.1.0

A modified, unofficial desktop port of the open-source firmware for Erica Synths' LXR V2 drum machine. Windows x64 standalone and VST3 instrument, with a shared C++ audio engine and JUCE 7.0.12 interface. Original firmware files remain unchanged. This is a software instrument, not a firmware updater or hardware editor.

- [Install and play](docs/QUICKSTART.md)
- [Build on Windows](docs/BUILD-WINDOWS.md)
- [Port architecture and sound differences](docs/PORT.md)
- [Validation and Windows acceptance checks](docs/VALIDATION.md)
- [Licences and attribution](docs/THIRD_PARTY.md)

The six original synthesis voices, original sample/wave tables, filters, envelopes, LFOs, velocity modulation, decimation and enabled master effects are reused. The desktop sequencer supplies seven tracks, 64 patterns, 64 steps per track, independent lengths and rotations, swing, probability, pitched steps, flams, microtiming, two parameter locks, Euclidean generation and looping song chains.

Windows binaries must be evaluated on the intended Windows audio hardware and in Ableton Live 12. Automated Linux checks do not establish Windows runtime or hardware sound parity.

Licensed under GPLv3. See the repository's `LICENSE` and `NOTICE.md`, and `docs/THIRD_PARTY.md`. Names and logos belonging to Erica Synths are used only to identify the source and do not imply endorsement.

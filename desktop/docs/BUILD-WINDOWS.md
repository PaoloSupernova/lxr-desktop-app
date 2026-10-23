# Windows build and installer

## Requirements

- 64-bit Windows 10/11 and Visual Studio 2022 with **Desktop development with C++** and the Windows 10/11 SDK.
- CMake 3.22+, Git and Python 3 on PATH.
- NSIS 3 to create the installer (or use `-SkipInstaller`).

Extract the source archive, then run from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\desktop\tools\Build-Windows.ps1
```

This configures Visual Studio x64, builds the standalone app, VST3 and tests, runs CTest, archives the complete source and creates `desktop\release\PulseFoundry-0.1.0-Windows-x64-Setup.exe`. The MSVC runtime is statically linked. No separate VC++ redistributable is required by this configuration.

The source bundle includes JUCE 7.0.12 under `desktop/third_party/JUCE` for offline builds. A normal repository checkout uses CMake FetchContent to retrieve the exact commit `4f43011b96eb0636104cb3e433894cda98243626`. The small ASIO SDK headers are included separately under the GPLv3 option. There is no proprietary ASIO licence acceptance step.

Manual build:

```powershell
cmake -S desktop -B desktop/build/windows-msvc -G "Visual Studio 17 2022" -A x64
cmake --build desktop/build/windows-msvc --config Release --parallel 3
ctest --test-dir desktop/build/windows-msvc -C Release --output-on-failure
```

Standalone: `desktop/build/windows-msvc/PulseFoundry_artefacts/Release/Standalone/Pulse Foundry.exe`.
VST3: `desktop/build/windows-msvc/PulseFoundry_artefacts/Release/VST3/Pulse Foundry.vst3`.

For a local dependency checkout, add `-DPF_JUCE_DIR=C:/path/to/JUCE-7.0.12`. `-DPF_ASIO=OFF` disables ASIO without disabling WASAPI. This is the desktop CMake project; the repository root CMake project still builds STM32 firmware.

## Linux verification

Install CMake, Ninja, a C++17 compiler, Python 3, ALSA development headers, X11 development headers and FreeType development headers. Then:

```sh
cmake -S desktop -B desktop/build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build desktop/build/linux -j3
ctest --test-dir desktop/build/linux --output-on-failure
```

Headless engine-only builds need no JUCE or GUI dependencies: add `-DPF_BUILD_PLUGIN=OFF`. `pf_render demo.wav` writes an eight-second PCM WAV. GUI and plugin checks may need an X display; use `xvfb-run` on a headless machine.

## Cross build used for the supplied first installer

The supplied Windows binaries are cross-compiled from Linux with Debian's MinGW-w64 GCC 14.2 POSIX toolchain and JUCE 7.0.12. `tools/mingw-toolchain.cmake` describes this optional route; override `PF_MINGW_ROOT` to point to the installed prefix. The VST3 module-info manifest generation step is disabled for cross builds because it would execute a Windows binary. The VST3 bundle contains the instrument module required for scanning/loading.

JUCE 7.0.12 was selected because it supports MinGW and GPLv3; JUCE 8/9 explicitly reject MinGW. Windows MSVC builds generate the normal module-info manifest. Windows runtime, installer and Ableton verification remain distinct from cross-compilation.

# NOCTURNE — Dark Synth (starter VST3 + standalone)

An original dark-toned hybrid instrument project for JUCE. It includes a custom saw/sub/pulse oscillator voice, low-pass filter and saturation, a phaser, LFO rate/depth controls, envelope parameter controls in the UI, and a sample loader for WAV/AIFF/FLAC/OGG files.

## Important
This package contains **source code**, not a precompiled Windows `.exe` or `.vst3` binary. A Windows build requires Windows, Visual Studio 2022 C++ tools, CMake, and JUCE. JUCE is not bundled here.

## Build on Windows
1. Install **Visual Studio 2022** with *Desktop development with C++* and **CMake**.
2. Download JUCE from https://github.com/juce-framework/JUCE .
3. Extract JUCE and note its directory.
4. Open a Developer PowerShell in this folder and run:

   ```powershell
   cmake -S . -B build -DJUCE_DIR="C:/path/to/JUCE"
   cmake --build build --config Release
   ```

5. Look under `build/NocturneDarkSynth_artefacts/Release/` for the Standalone app and VST3 bundle. The exact folder can vary by CMake/JUCE version.
6. Copy the `.vst3` bundle into:
   `%CommonProgramFiles%\VST3`
   Then rescan plugins in your DAW.

## Sound design controls
- **Cutoff / Resonance:** dark low-pass tone shaping.
- **Drive:** soft saturation.
- **Phaser:** modulation effect mix.
- **LFO Rate / Mod Depth:** phaser modulation speed and intensity.
- **Attack / Release:** exposed envelope controls (currently UI/parameter scaffold; see limitations).
- **Sampler Mix:** exposed control scaffold; sample playback is loaded and triggered from MIDI.

## Current prototype limitations
This is a starting point, not a production-ready commercial instrument. The attack/release and sampler-mix parameters are exposed but not yet wired into full envelope/mix behavior; sample loading is supported but sampler routing and host automation should be thoroughly tested before release. The sampler is triggered by MIDI notes, while the oscillator is the default sound source. Validate in your target DAW and test plugin state, MIDI, and sample-rate changes before distributing.

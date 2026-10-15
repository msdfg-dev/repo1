# NOCTURNE — Dark Synth (VST3 + standalone)

A dark-toned polyphonic synthesizer built with JUCE: two detuned band-limited oscillators (saw / square / triangle) plus a sub oscillator, an ADSR amp envelope, a resonant low-pass filter with saturation, and a phaser. It ships with three sound banks of five patches each.

## Build

A GitHub Actions workflow (`.github/workflows/build-windows.yml`) builds the Windows Standalone `.exe` and the VST3 on every push to `main`. Download the `NocturneDarkSynth-Windows` artifact from the run's page.

To build locally on Windows: install Visual Studio 2022 (Desktop development with C++), CMake and JUCE, then

```powershell
cmake -S . -B build -DJUCE_DIR="C:/path/to/JUCE"
cmake --build build --config Release
```

Output is under `build/NocturneDarkSynth_artefacts/Release/`. Copy the `.vst3` bundle to `%CommonProgramFiles%\VST3` and rescan plugins in your DAW.

## Playing it

- In the Standalone app, click the on-screen keyboard, or click it once to focus it and play with the computer keyboard (A W S E D F T G Y H U J K ...). Z / X shift the octave.
- In a DAW, play it from any MIDI track. Pitch bend is ±2 semitones.

## Sound banks

Pick a bank and patch with the selectors at the top right, or step through with `<` and `>`. Choosing a patch sets every sound control. All 15 patches are also exposed to the host as programs.

| Bank | Patches |
|------|---------|
| NOCTURNE | Midnight Pad, Black Glass, Dusk Bass, Hollow Keys, Eclipse Lead |
| ABYSS | Deep Drone, Sunken Choir, Pressure, Trench Pulse, Black Water |
| STATIC | Neon Rust, Wire Saw, Cold Arp, Glitch Bell, Broken Radio |

Edit `Source/Presets.h` to change the patches or add banks.

## Controls

- **Waveform / Detune / Sub:** oscillator shape, spread between the two oscillators (cents), and sub-oscillator level.
- **Attack / Release:** amp envelope times.
- **Cutoff / Resonance:** dark low-pass tone shaping.
- **Drive:** soft saturation.
- **Phaser / LFO Rate / Mod Depth:** phaser mix, speed and intensity.

Test in your target DAW before distributing, including plugin state, MIDI and sample-rate changes.

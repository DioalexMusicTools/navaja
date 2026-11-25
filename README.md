# DC-23 v0.3 — Vintage Color Lab

Original, JUCE-based 6-module multi-FX VST3 for macOS and FL Studio, produced for Dioalex Music Tools.

**Not affiliated with XLN Audio, RC-20, or Arturia.** No source code, proprietary presets, logos, audio samples or internal algorithms are copied. The preset library is an original interpretation of common lo-fi tape, vinyl, ambient and sampler-production use cases.

## What's new

- **Clear units throughout the interface**: Amount, Flux, Color, Digital Crush, Smooth, Feedback, Magnitude and Stereo Width appear as 0–100%. Wobble Rate shows Hz. Reverb Decay and Magnetic Drop Interval show milliseconds (ms). Output is dB.
- **Centered bipolar Dry/Wet controls**: the upper-center position is **0% = 50/50 dry/wet**. Turning left moves to **-100% = fully dry**, right to **+100% = fully wet**. Both global mix and Echo mix use this convention. Echo has its own ON/OFF switch and is OFF for the clean default preset.
- **BPM-synced Echo**: note divisions **1/2, 1/4, 1/8 dotted, 1/8, 1/8 triplet, 1/16, 1/32**. Echo reads the host's BPM, with a 120 BPM fallback. The UI displays the resulting duration in milliseconds and current BPM. Time mapping, e.g. 1/4 at 120 BPM = 500ms.
- **Low Cut / High Cut in Hertz**: physical cutoff frequencies, 20–2200Hz and 250–20000Hz; separate on/off switches permit genuine bypass without using a magic 0Hz setting.
- **Custom professional interface**: centered bipolar knobs, consistent numeric readouts and legends, six horizontally aligned effect modules, animated waveforms, audio-reactive display and output-meter indicator. These visualizations indicate motion and character; they are **not** precision oscilloscopes or frequency analyzers.
- **20 original factory presets**: Clean Slate, Cassette Warmth, Night Bus, Dusty Piano, Vinyl Morning, Beat Tape, Worn Drum Loop, Bit Crumble, Dreamy Drift, Soft Organ, Spacious Pad, Glassy Repeats, Neon Stairs, Lost Transmission, Old Radio, Mono Memory, Analog Bus, Saturated 808, Tape Flutter and Slumber Keys. Presets are original DC-23 values, not conversions of RC-20 files.

## Audio engine

Six serial modules: Noise (Tape/Vinyl/Hum), Wobble (wow/flutter), Distort (Tape/Valve/Fuzz), Digital (bit and rate reduction), Space (4-feedback-comb diffuse reverb with time-based feedback), Magnetic (dropout interval in ms). Each has ON/OFF, Amount, Character and Flux. Echo follows the effect modules, with BPM sync and feedback. Output width and low/high cuts follow Echo. All JUCE parameters are automatable.

## Building

Requirements: macOS, Xcode 16 or Xcode CLI tools, CMake 3.22+, network access for JUCE 9.0.6 and its licence terms.

    cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
    cmake --build build --config Release --target DC23Plugin_VST3 dc23_dsp_tests --parallel 3
    ctest --test-dir build -C Release --output-on-failure

Compiled bundle expected in:

    build/DC23Plugin_artefacts/Release/VST3/DC-23.vst3

Or run portable DSP tests without JUCE:

    cmake -S . -B build-core -DDC23_BUILD_PLUGIN=OFF
    cmake --build build-core
    ctest --test-dir build-core --output-on-failure

## macOS installation

In GitHub, open **Actions > DC-23 macOS VST3** and select the latest green build for branch **dc23-v03**. Under Artifacts, download **DC-23-macOS-universal-VST3**; expand both the Actions artifact ZIP and the inner plugin ZIP if applicable. Install the full **DC-23.vst3** bundle into one of:

    ~/Library/Audio/Plug-Ins/VST3/
    /Library/Audio/Plug-Ins/VST3/

Close FL Studio before replacing the existing bundle; back up your older DC-23.vst3 and any important FL Studio projects first.

The GitHub artifact is not Apple Developer signed/notarized. If macOS quarantines your self-built download, confirm its origin and then use (for user-level installation):

    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/DC-23.vst3"
    codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/DC-23.vst3"

Change the path and add sudo if you installed under /Library/Audio/Plug-Ins/VST3. Reopen FL Studio and rescan via Options > Manage plugins.

## Compatibility and test status

- GitHub Actions builds native VST3 for Intel and Apple Silicon and runs timing, factory preset, audio bypass, Echo and extreme-input tests. These are engineering tests, not proof of auditory quality, sample-accurate host automation or FL Studio stability.
- The VST3 bundle ID is unchanged for host recognition. **Existing parameter ranges have changed to display user-friendly units**, so older DC-23 automation and project recall may behave differently. Back up projects before installing. The previous v0.2 branch remains available.
- No sample-based vinyl loops, oversampled saturation, automatic tempo-ramp crossfading, adjustable module order, surround sound or signed installer are provided in this build.
- JUCE 9 licensing is dual AGPLv3/commercial. Review the upstream licensing before distributing a proprietary commercial plugin.

## Next listening tests

Compare original and processed audio at equal loudness. Check piano, drum breaks, sampled loops, synth pads, vocals and bass in FL Studio. Assess unwanted clicks, noise at silence, gain compensation, reverb buildup, large transport BPM changes and the sound of each model. Screenshot the plugin editor if any controls are clipped at your screen's macOS display scaling.

# DioVERB — Reverse Reverb VST3 for macOS

**DioVERB** is an original reverse-reverb creation plugin for FL Studio and other macOS VST3 hosts, produced for **Dioalex Music Tools**. It has a simple recording workflow designed for short vocal phrases, chord hits, guitars and synths.

**Capture → Shape → Render → Preview → Drag WAV.**

> Backverb by Yoshi Audio was a workflow reference. DioVERB is independently implemented, with original DSP, visuals, names and source code. It is not an XLN, Arturia or Yoshi Audio product. No proprietary presets or assets are embedded.

## Features

- **Capture** up to 12 seconds of audio from an insert in the DAW, while passing it through unaffected.
- **Reverse reverb** created with an original offline multidelays/diffuser engine: reverse the captured onset; create the wet tail; reverse that tail into an anticipatory swell of exactly the selected beat length.
- **Length:** 1/4 bar, 1/2 bar, 1 bar (4/4). Reads host BPM; defaults to 120 BPM if no tempo is provided.
- **Mode:** Vocal / Instrument (tail excitation and phrase focus vary).
- **Controls:** Size, Tone, Width, Tremolo rate (Off, 1/8, 1/16, 1/32), Depth, Output.
- **Preview:** plays the generated audio from the plugin into the host mixer while the audio callback runs.
- **Drag WAV:** click and drag the labeled pad into the FL Studio Playlist. Uses JUCE native external file dragging. A **SAVE WAV** action also exports a copy through a file dialog.
- **WAV:** interleaved stereo, 24-bit, at the host sample rate.
- All shape parameters saved with FL Studio projects via the JUCE parameter state. Captured audio and exported WAV **are not** embedded into saved project state, so keep exported files.

## Installing macOS VST3

In GitHub, open **Actions → DioVERB macOS VST3** and open the latest **successful** workflow on branch **dioverb-macos**. Under **Artifacts**, download **DioVERB-macOS-universal-VST3**. Unzip the Actions artifact ZIP and then, if present, the inner ZIP to obtain **DioVERB.vst3**.

Install the entire bundle to one of:

    ~/Library/Audio/Plug-Ins/VST3/
    /Library/Audio/Plug-Ins/VST3/

In FL Studio: **Options → Manage plugins → Find installed plugins**. Put DioVERB on a Mixer effect insert receiving a vocal, synth, piano or guitar. It is an **effect, not an instrument**.

**Gatekeeper:** The GitHub Actions output is not Developer-ID signed/notarized. Confirm the binary came from the linked GitHub build before removing quarantine. On a user-level installation:

    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/DioVERB.vst3"
    codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/DioVERB.vst3"

For a system-wide install, substitute /Library/Audio/Plug-Ins/VST3/DioVERB.vst3 and use sudo. These commands affect only DioVERB.

## How to use

1. Set the FL Studio tempo. In the DioVERB insert, choose **VOCAL** or **INSTRUMENT** and a bar length.
2. Press **CAPTURE** and start playback in FL Studio so the phrase passes through the insert. Press **STOP CAPTURE** after the sound. A maximum of 12 seconds is buffered.
3. Adjust **SIZE, TONE, WIDTH** and optional **TREMOLO / DEPTH**. Click **RENDER** to create and save the reverse effect.
4. Press **PREVIEW**. The audio plays through the same insert; turn off DAW playback for a clean audition.
5. Drag the green **DRAG WAV TO DAW** pad onto the FL Studio Playlist. Move it so its end lines up precisely with the vocal/chord onset.
6. Alter parameters and click **RENDER** again to create a new WAV. Previous renders are retained in the export folder.

The files are placed in the macOS Application Support folder under **DioVERB/Exports**. The plug-in does not delete older exports; remove ones you no longer need.

## Build from source

Install Xcode command-line tools and CMake >= 3.22.

    cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
    cmake --build build --config Release --target DioVERBPlugin_VST3 dioverb_dsp_tests --parallel 3
    ctest --test-dir build -C Release --output-on-failure

Expected plugin: build/DioVERBPlugin_artefacts/Release/VST3/DioVERB.vst3

You can run the portable reverse-reverb engine tests without JUCE:

    cmake -S . -B build-core -DDIOVERB_BUILD_PLUGIN=OFF
    cmake --build build-core
    ctest --test-dir build-core --output-on-failure

## Engineering / limitations

- Initial v0.1 engineering version, not yet validated by ear or through a full FL Studio regression session on user hardware.
- Export is **offline** (not latency-compensated live preverb), because a lead-in effect must be placed before its source clip.
- The preview uses the DAW audio callback; it may require the audio engine to be running even if the song transport is stopped.
- Captures focus on the **first audible event** of the phrase (up to around one second); this helps create a clear swell, but is not automatic word segmentation.
- Only 4/4 bar terminology is currently supported; there is no DAW timeline drag positioning API, so align the WAV's end manually.
- macOS FL Studio should support the VST3 API, but native external drag acceptance may vary by host/version; the **SAVE WAV** control is provided as a reliable alternative.
- This plugin is not signed or notarized by Apple. Commercial macOS distribution requires a proper Developer ID signing and notarization workflow.
- DSP buffers are allocated offline except for preallocated capture buffers. No disk I/O or rendering on the realtime processBlock callback.
- JUCE 9 is dual-licensed under AGPLv3 or commercial terms. Review JUCE legal terms before proprietary distribution.

## Version

0.1.0 — first capture / reverse renderer / preview / drag VST3 for macOS.

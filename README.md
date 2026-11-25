# DC-23 0.2 — Vintage Color Lab

Original macOS VST3 multi-effects processor for FL Studio. Developed for Dioalex Music Tools.

## What's improved in 0.2

A major audio/UI iteration built on the original six-module engine.

* **Noise**: Tape hiss, vinyl crackles or electronic hum; COLOR and FLUX controls.
* **Wobble**: tape wow/flutter with independent RATE and organic FLUX.
* **Distort**: three original models (TAPE / VALVE / FUZZ), tone and per-effect FLUX.
* **Digital**: sampler-style downsampling, bit depth degradation, extra SMOOTH control.
* **Space**: four damped combs plus two diffusion allpasses; adjustable DECAY.
* **Magnetic**: intermittent, smoothly ramped volume dropouts.
* **Echo**: tape-colored repeats with MIX, TIME (90–900 ms) and FEEDBACK.
* **Master**: MAGNITUDE, DRY/WET, output dB, stereo width, low-cut and high-cut.

Each module offers its own ON/OFF, AMOUNT, CHARACTER and FLUX. FLUX adds smoothed, bounded organic variation independently to each module. Six factory presets and user .dc23preset save/load remain available.

The interface presents a single horizontal six-module signal chain with a high-contrast neon green header and dark custom rotary knobs. This is an **original UI** inspired by modern audio-plugin workflows, not a reproduction of RC-20 or Arturia Pure Sub.

## Status and limitations

- DSP, module controls and project recall are integrated; a successful CI run verifies macOS universal VST3 compilation and the C++ audio tests. **Actual plugin stability and audio character must still be checked in FL Studio on real projects.**
- Effect order remains fixed. No Echo sync-to-tempo or exact RC-20 proprietary algorithm/preset matching.
- Added parameters preserve old parameter IDs and VST3 identity; older saved projects should continue to load, but make a backup of critical projects before upgrading.
- The GitHub build is **unsigned/not notarized by Apple**. Mac Gatekeeper may flag the download. Review source and build provenance before installing.
- The engine is designed to avoid allocations/locks while processing audio; channel delay buffers are allocated in prepareToPlay.

## Download and installation

Navigate to **Actions > DC-23 macOS VST3** and open the latest successful workflow run for branch dc23-v2. Under *Artifacts*, download **DC-23-macOS-universal-VST3**, unzip the downloaded artifact, and if needed unzip the inner DC-23-macOS-universal-VST3.zip. You should have a full **DC-23.vst3** bundle.

Close FL Studio. Replace the previous DC-23.vst3 bundle in either:

    ~/Library/Audio/Plug-Ins/VST3/
    /Library/Audio/Plug-Ins/VST3/

Never install two copies simultaneously. If macOS quarantines this self-built artifact, and **only after verifying its origin**, execute this in Terminal for a *user-level* installation:

    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/DC-23.vst3"
    codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/DC-23.vst3"

For a system-level installation replace the path with /Library/Audio/Plug-Ins/VST3/DC-23.vst3 and use sudo as appropriate. These commands do not disable system-wide Gatekeeper. Reopen FL Studio and run Options > Manage Plugins > Find installed plugins. Insert DC-23 into a mixer effect slot.

## Compile on your Mac

Requires macOS, Xcode Command Line Tools or Xcode, CMake >= 3.22 and internet for JUCE 9.0.6.

    cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
    cmake --build build --config Release --target DC23Plugin_VST3 dc23_dsp_tests --parallel 3
    ctest --test-dir build -C Release --output-on-failure

Expected plug-in bundle:

    build/DC23Plugin_artefacts/Release/VST3/DC-23.vst3

Core audio tests can also run without JUCE:

    cmake -S . -B build-core -DDC23_BUILD_PLUGIN=OFF
    cmake --build build-core
    ctest --test-dir build-core --output-on-failure

## Roadmap

1. Blind A/B listening tests with commercial lo-fi processors and reference audio clips.
2. Audio analyzer: frequency response, dynamic gain, input/output peak and spectrum.
3. Linear/oversampled saturation and more musical distortion models.
4. Tempo-sync Echo, adjustable per-module ordering and parameter-level fine-tuning.
5. Installable signed/notarized macOS package.

## Licensing

DC-23 is original source created for Dioalex Music Tools, not affiliated with XLN Audio or Arturia. No third-party source or preset samples are included. JUCE is offered under AGPLv3 or a commercial licence; review the upstream terms before commercial closed-source distribution. No license grant is stated for this repository's original code until its owner selects one. Read https://juce.com/legal/juce-9-licence/ and https://github.com/juce-framework/JUCE/blob/master/LICENSE.md.

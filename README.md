# DC-23 — Vintage Color Engine

**DC-23** is an original six-module lo-fi multi-effect designed for macOS VST3 hosts including FL Studio. Created for Dioalex Music Tools. Version 0.1.0 (initial engineering build).

> This is NOT a copy of RC-20 Retro Color or Arturia Pure Sub. Its code, DSP and interface are original. The reference design informs only broad visual direction (dark panels, acid-lime header, modern controls). There are no third-party presets, sampled assets, trademarks or source code in this project.

## Features

| Module | Amount knob | Character knob |
| --- | --- | --- |
| Noise | Tape-style hiss and dust | Filtered noise color |
| Wobble | Continuous wow/flutter tape warble | Modulation speed (Hz) |
| Distort | Gain-compensated tanh saturator | Drive color |
| Digital | Sample-and-hold / bit-depth reduction | Rate reduction |
| Space | 3-comb stereo ambient room | Room decay |
| Magnetic | Smoothed stochastic gain dropouts | Wear speed |

Master controls: magnitude (scales all effects), Dry/Wet, output gain, low cut, high cut. Each module has its own ON/OFF toggle. All parameters are automatable and stored with the FL Studio project (JUCE AudioProcessorValueTreeState). Includes six factory presets and .dc23preset XML save/load.

## Building on a Mac

Requirements: macOS with Xcode Command Line Tools or Xcode, CMake >= 3.22, network access to clone JUCE, C++17.

    cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
    cmake --build build --config Release --target DC23Plugin_VST3 --parallel 3
    ctest --test-dir build -C Release --output-on-failure

Output (expected):

    build/DC23Plugin_artefacts/Release/VST3/DC-23.vst3

Install by copying the entire .vst3 bundle to:

    ~/Library/Audio/Plug-Ins/VST3/

Rescan plugins in FL Studio (Options > Manage plugins > Find installed plugins), then insert DC-23 on a mixer effect slot. Do not put it in a synth instrument slot.

## GitHub Actions / compiled plugin

The workflow at .github/workflows/build-macos.yml compiles an arm64/x86_64 universal macOS VST3 and runs core DSP tests. Upon a successful workflow run, get the ZIP under **Actions > DC-23 macOS VST3 > run > Artifacts**. GitHub Actions produces build artifacts, *not* a notarized or installer-signed release.

If Gatekeeper blocks an independently built plugin, inspect its origin and sign the locally installed bundle ad-hoc for local testing:

    codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/DC-23.vst3"

For public distribution, developer signing, notarization and tests on the target macOS/FL Studio versions are required. No reliability or compatibility guarantee is made before those tests.

## Testing core DSP without JUCE

    cmake -S . -B build-core -DDC23_BUILD_PLUGIN=OFF
    cmake --build build-core
    ctest --test-dir build-core --output-on-failure

The portable DSP is in Source/DSP.h (no runtime dynamic allocations); smoke tests cover transparent bypass, dry/wet preservation, audible noise and sustained finite output.

## Design/technical notes

- Stereo and mono support, 44.1–192 kHz intended.
- Smoothed effect intensities and volume dropouts.
- Stateful delay lines allocated only at prepareToPlay, no allocation or locks in processBlock.
- Factory sounds are illustrative starting points, not matches to proprietary RC-20 algorithms.
- The reverb is deliberately light and compact; this is an early engineering release, not an exact feature/sonic equivalent of RC-20 v2 (e.g. no granular echo, movable FX order or advanced codec processing).
- Editor layout fixed at 1060×660; VST3 host automation and project state are supported.

## Licensing

JUCE 9 is dual-licensed under AGPLv3 and a commercial JUCE license. Review https://github.com/juce-framework/JUCE/blob/master/LICENSE.md and https://juce.com/legal/juce-9-licence/ before redistribution. **A commercial JUCE licence may be required**, especially for closed-source products. No license grant is provided for this original DC-23 source until the project owner selects an appropriate license. Third-party code must retain its own applicable license notices.

# DioCHORD X — Mood Progression Studio (v0.1)

**Original chord-progression generator and instrument by Dioalex Music Tools.**

DioCHORD X is an original C++17 / JUCE 9 VST3 instrument, designed for FL Studio and compatible VST3 hosts on Apple Silicon, Intel macOS and Windows x64. It creates four-bar harmonic ideas, previews them using an internal synthesized keyboard, and exports editable Standard MIDI Files.

**Not affiliated with Sound Horizon Academy, ChordZ or Valhalla DSP.** ChordZ was a workflow reference and a user-provided VintageVerb screenshot was a general color-and-panel reference; all UI, presets and algorithms in this project are independently created. No proprietary samples, preset banks or interface assets are included.

## Version 0.1 features

- 16 original progression presets in four moods: Reflective, Uplifting, Urban and Dark.
- Transpose across 12 tonic keys and major/minor scale intervals.
- Four chord bars with extended chord voicings (maj7, m7, maj9, m9, dominant, sus and diminished).
- Adjustable octave (-1, 0, +1), inversion (root / first / second).
- Performance patterns: Flow, Stabs, Arpeggio and rhythmic 3-3-2.
- Half speed, normal and double speed; each export remains musically timed in PPQ beats.
- Humanize, Strum (0–90 ms), Velocity and Take variations are reproducible in preview and export.
- Piano-like Studio Keys, Electric Keys and Warm Pad synthesis for instant audition, plus MIDI Only (silent audio output) mode.
- Supports DAW playhead tempo and position, or internal preview PLAY while transport is stopped (when host audio callback remains running).
- Native MIDI output is emitted in the plugin's process callback, though **FL Studio's VST3 MIDI routing depends on host support/configuration**.
- Drag MIDI to another track where supported, with **SAVE MIDI** fallback.
- All parameters saved as JUCE plugin state with your project, including the selected progression.

## Important limitations

This is a **first functional release**, not complete feature equivalence with ChordZ. Current v0.1 does **not** include importing MIDI, per-chord STEAL or SPLIT, manual per-bar resize, polished sampled piano libraries, independent chord substitution, or a full visual piano roll editor. The preview is synthesized from original simple oscillators, not a sampled grand piano.

Internal preview may rely on the host running audio callbacks when its transport is stopped. Host playback of emitted VST3 MIDI, native dragging, and GUI scaling must be checked in real FL Studio sessions on both platforms. The generated MIDI export and internal synth share the same harmonic sequence algorithm, but GUI and host timing still require listening tests.

## Download (GitHub Actions)

Open **Actions → DioCHORD X VST3 - macOS and Windows**, select the latest successful run for branch **diochord-x-v01**, and choose your platform in **Artifacts**:
- **DioCHORD-X-macOS-universal-VST3** — native arm64 and x86_64 Mac plugin.
- **DioCHORD-X-Windows-x64-VST3** — Windows 10/11 64-bit plugin.

Expand the Actions artifact ZIP, then the inner ZIP if present, until you have the complete **DioCHORD X.vst3** folder/bundle. Preserve its internal Contents directory intact.

### macOS installation

Copy \`DioCHORD X.vst3\` into one of:

    ~/Library/Audio/Plug-Ins/VST3/
    /Library/Audio/Plug-Ins/VST3/

Then rescan via FL Studio **Options → Manage plugins → Find installed plugins**. DioCHORD X is a **generator / instrument**, not a Mixer effect. Add it from **Channel Rack → +**.

This GitHub build is **not signed or notarized with Apple Developer ID**. After verifying its provenance, you may remove per-file quarantine if Gatekeeper blocks it (user installation only):

    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/DioCHORD X.vst3"
    codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/DioCHORD X.vst3"

Do not disable macOS security globally. For the system VST3 directory substitute the path and use sudo where needed.

### Windows installation

Close FL Studio and copy the **entire** \`DioCHORD X.vst3\` folder into:

    C:\Program Files\Common Files\VST3\

Open **FL Studio 64-bit → Options → Manage plugins → Find installed plugins**. Load DioCHORD X in **Channel Rack → +**. Windows binaries are unsigned; install only downloads from your own verified GitHub Actions workflow.

## Workflow

1. Add DioCHORD X to the FL Studio Channel Rack.
2. Select a mood and progression, then pick key and scale.
3. Press PLAY to audition internally, or start the DAW transport for host synchronization.
4. Choose voicing, speed and Feel (Flow/Stabs/Arpeggio/3-3-2). Set Humanize, Strum, Velocity and Take.
5. Choose audition sound; MIDI Only mutes DioCHORD X audio (without disabling generated MIDI).
6. **Drag MIDI to Track** into a compatible DAW track, or press **SAVE MIDI** and import the generated .mid file into FL Studio's Piano Roll / Playlist.
7. Load the .mid in another instrument to hear your chords using any VSTi.

The .mid file is Standard MIDI File type 0 with 960 PPQ ticks per quarter note, no embedded proprietary audio. Musical timing follows the tempo of the destination DAW. The MIDI does not embed the source project's absolute transport start position.

## Build source

Requires CMake 3.22+, Xcode (macOS) or Visual Studio 2022 with the C++ Desktop workload (Windows), and internet for JUCE 9.0.6.

    cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
    cmake --build build --config Release --target DioCHORDXPlugin_VST3 diochord_harmony_tests --parallel 3
    ctest --test-dir build -C Release --output-on-failure

For Windows configure with:

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --target DioCHORDXPlugin_VST3 diochord_harmony_tests --parallel 3
    ctest --test-dir build -C Release --output-on-failure

Harmony regression tests can run without fetching JUCE:

    cmake -S . -B build-test -DDIOCHORD_BUILD_PLUGIN=OFF
    cmake --build build-test
    ctest --test-dir build-test --output-on-failure

JUCE is licensed under AGPLv3 or commercial terms. Review https://juce.com/legal/ before commercial proprietary distribution.

## Planned v0.2

Per-chord editor, Split/Steal with original theory-driven options, MIDI import with sanitization, richer voicing and humanization presets, host-specific drag and note-routing validation, improved sampled audition instruments with appropriately licensed content, and a signed/notarized macOS distribution.

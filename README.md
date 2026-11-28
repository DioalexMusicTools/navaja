# DC-23 v1.0 — Vintage Color Lab

Original JUCE/C++ six-module vintage multi-FX by Dioalex Music Tools. VST3 for FL Studio on macOS universal (Apple Silicon + Intel) and Windows x64.

Not affiliated with XLN Audio, RC-20, Valhalla DSP, Arturia or other products; no proprietary source, samples, brand or presets are copied.

## v1.0 parameter reference

| Parameter | Range | Default |
| --- | --- | --- |
| Amount (6 modules) | 0–100% | 0% |
| Flux (6 modules) | 0–100% | 0% |
| Rate (Wobble) | 6.0–30.0 Hz | 12.0 Hz |
| Stereo Width | 0–200% | 100% |
| High Cut | 20–20000 Hz | 8778 Hz |
| Low Cut | 20–20000 Hz | 62 Hz |
| Tone (Distort) | −12 to +12 dB | +4.2 dB |
| Master Dry/Wet | 0–100% | 50% |
| Magnitude | 0–100% | 100% |
| Noise Color | 0–45% | 35% |
| Digital Crush | 0–50% | 25% |
| Magnetic Speed | 0–100 ms | 50 ms |
| Space Decay | 0–100 ms | 50 ms |
| Output | −18 to +18 dB | +5 dB |

Low Cut min/max were reversed in the requested description, so DC-23 correctly uses minimum 20 Hz, maximum 20000 Hz, and default 62 Hz.

At 0ms Decay there is no reverb feedback; at 0ms magnetic Speed new dropouts are bypassed. Filters are ON by default. Dry/Wet is 0% dry, 100% wet. Echo wet control also uses 0–100%. Echo OFF by default. 20 original factory presets were remapped for the revised control ranges.

**Upgrade warning:** Existing parameter IDs are retained, but many ranges and units have changed since v0.3. Back up all FL Studio projects, automations and custom presets before installing v1.0. Old sessions can sound different after recall.

## Download v1 installers

GitHub Actions, branch dc23-v1-release, workflow DC-23 v1.0 - macOS DMG and Windows EXE. Select a successful job and download:

* DC-23-v1.0-macOS-DMG: macOS .dmg with graphical .pkg installer.
* DC-23-v1.0-Windows-EXE: x64 Windows .exe setup; contains an Inno uninstaller and Uninstall DC-23.cmd.

Artifacts arrive as ZIPs that must be extracted first.

### macOS

Open the DMG and double-click Install DC-23.pkg. Installer copies DC-23.vst3 to /Library/Audio/Plug-Ins/VST3/.

For unsigned development builds only, its post-install script automatically executes the two terminal commands you have used before, scoped to DC-23 alone:

    xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/DC-23.vst3"
    codesign --force --deep --sign - "/Library/Audio/Plug-Ins/VST3/DC-23.vst3"

For Developer-ID-signed builds, this script does NOT re-sign, preserving the original notarizable signature. It does not disable Gatekeeper globally.

Uninstall from the same DMG with "Uninstall DC-23.command"; this removes only /Library/Audio/Plug-Ins/VST3/DC-23.vst3 and the package receipt, after confirmation and administrative approval.

Important: unsigned Mac .dmg and .pkg still may be blocked by Gatekeeper before installation. Automatic local permissions fixes are not an alternative to Apple Developer ID signing and Apple notarization.

### Windows

Close FL Studio and run DC-23-v1.0-Windows-x64-Setup.exe. It installs the complete bundle under C:\Program Files\Common Files\VST3\DC-23.vst3. Administrative approval is required. Uninstall from Windows Settings > Installed Apps, the Start menu shortcut or the installed Uninstall DC-23.cmd in Program Files > Dioalex Music Tools > DC-23.

Open FL Studio → Options → Manage plugins → Find installed plugins. Insert DC-23 as an effect on a mixer channel.

## Public distribution readiness

GitHub Actions can build evaluation installers without proprietary signing certificates. They are NOT suitable for a frictionless public distribution until all of these are done:
- Supply Apple Developer ID Application and Installer signing identities, sign the VST3 and package, notarize both PKG and DMG, staple tickets, and validate Gatekeeper clean-machine installation.
- Sign the Windows DLL and EXE with an Authenticode publisher certificate for better Windows trust.
- Test UI sizing, plugin scanning, automation and old preset recall, 20 factory presets, CPU usage and sound (gain/headroom at +18 dB; delay extremes; click-free changes) in real FL Studio on Apple Silicon, Intel and Windows x64.
- Check licensing for JUCE 9 (dual AGPLv3/commercial) and any third-party dependencies and release rights.

GitHub Action secret names for optional macOS public signing:
  DC23_MAC_DEVELOPER_CERT_P12_BASE64
  DC23_MAC_DEVELOPER_CERT_PASSWORD
  DC23_MAC_APP_SIGNING_IDENTITY
  DC23_MAC_INSTALLER_SIGNING_IDENTITY
  DC23_APPLE_ID
  DC23_APPLE_TEAM_ID
  DC23_APP_SPECIFIC_PASSWORD

For optional Windows Authenticode signing:
  DC23_WINDOWS_CODESIGN_PFX_BASE64
  DC23_WINDOWS_CODESIGN_PFX_PASSWORD

Add these to GitHub repository Actions Secrets. Never paste private keys/passwords in chat or commit them.

## Manual builds

Mac:

    cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
    cmake --build build --config Release --target DC23Plugin_VST3 dc23_dsp_tests --parallel 3
    ctest --test-dir build -C Release --output-on-failure

Windows:

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --target DC23Plugin_VST3 dc23_dsp_tests --parallel 3
    ctest --test-dir build -C Release --output-on-failure

Portable tests:

    cmake -S . -B build-core -DDC23_BUILD_PLUGIN=OFF
    cmake --build build-core
    ctest --test-dir build-core --output-on-failure

## v1.0 UI refinement (version number unchanged)

- Units are shown in the plugin and exposed as VST3 parameter labels: %, Hz, ms and dB, with at most two digits after the decimal point.
- Rotary knobs stop at both limits. Circular drags do not jump to the other end at maximum or minimum.
- The top-right gear opens Settings: Scale adjusts the entire window from 50% through 200%, default 100%, stored with the plugin's project state. About displays "DC-23 | Version 1.0 | Developed by Dioalex Music Tools".
- The macOS DMG includes a standalone confirmed-uninstall script. Windows includes Inno Setup's native uninstaller, Start menu entry and Uninstall DC-23.cmd; it removes only DC-23.
- UI updates retain version 1.0.0 and the current VST3 plugin identity.

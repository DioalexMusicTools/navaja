# DioVERB — Windows VST3 (64-bit)

This branch builds the same original audio plugin as the existing macOS edition, but for Windows 10/11 64-bit using Microsoft Visual C++ and JUCE 9.0.6.

## Download
1. Visit the GitHub repository Actions tab and choose **DioVERB Windows x64 VST3**.
2. Open the latest **successful** run from branch `dioverb-windows-x64`.
3. Under **Artifacts**, download **DioVERB-Windows-x64-VST3**.
4. Unzip the downloaded GitHub artifact, then unzip its internal `DioVERB-Windows-x64-VST3.zip` if present.
5. The resulting folder **DioVERB.vst3** (including the `Contents` folder) is the Windows VST3 bundle. Preserve it intact.

## Install in FL Studio for Windows
Close FL Studio, then copy the **whole** `DioVERB.vst3` folder into:
`C:\Program Files\Common Files\VST3\`

You may need Administrator permission to copy there. Reopen FL Studio and choose **Options → Manage Plugins → Find installed plugins**. Add the effect to a Mixer insert. The **64-bit edition** of FL Studio is required.

## Technical notes
- Workflow compiles and tests with Windows Server 2022 / Visual Studio 2022, Release, x64, VST3.
- The VST3 binary inside the folder is expected at `DioVERB.vst3/Contents/x86_64-win/DioVERB.vst3`.
- The output uses the same original plugin ID and parameters as this plugin's macOS branch; do not install two copies with the same name in multiple VST3 folders.
- No installer or code-signing certificate is supplied; Windows SmartScreen or Defender may still inspect the downloaded bundle. Do not disable antivirus protection to install.
- Successful automated build/tests do not prove runtime compatibility with every FL Studio installation. Confirm UI scale, WAV capture/drag behavior (DioVERB), and sound with test sessions.
- Any further platform-specific corrections should stay on this dedicated branch until verified.

#!/bin/bash
# DC-23 v1.0 macOS package builder, scoped postinstall and optional notarization.
set -euo pipefail
APP="build/DC23Plugin_artefacts/Release/VST3/DC-23.vst3"
BIN="$APP/Contents/MacOS/DC-23"
[[ -f "$BIN" ]]
lipo "$BIN" -verify_arch arm64 x86_64
mkdir -p dist package-root/Library/Audio/Plug-Ins/VST3 dmg-content
SIGNED=false
if [[ -n "$MAC_P12_BASE64" ]]; then
  [[ -n "$MAC_P12_PASSWORD" && -n "$MAC_APP_IDENTITY" && -n "$MAC_INSTALLER_IDENTITY" ]] || {
    echo "Incomplete Apple Developer ID signing secrets." >&2; exit 1;
  }
  KEYCHAIN="$RUNNER_TEMP/dc23-release.keychain-db"
  PASSWORD="dc23-actions-$GITHUB_RUN_ID"
  printf '%s' "$MAC_P12_BASE64" | base64 -D > "$RUNNER_TEMP/developer-id.p12"
  security create-keychain -p "$PASSWORD" "$KEYCHAIN"
  security unlock-keychain -p "$PASSWORD" "$KEYCHAIN"
  security set-keychain-settings -lut 18000 "$KEYCHAIN"
  security list-keychains -d user -s "$KEYCHAIN"
  security import "$RUNNER_TEMP/developer-id.p12" -k "$KEYCHAIN" -P "$MAC_P12_PASSWORD" -T /usr/bin/codesign -T /usr/bin/productsign
  security set-key-partition-list -S apple-tool:,apple: -s -k "$PASSWORD" "$KEYCHAIN"
  codesign --force --deep --options runtime --timestamp --sign "$MAC_APP_IDENTITY" "$APP"
  codesign --verify --strict --deep --verbose=2 "$APP"
  SIGNED=true
else
  echo "::warning::No Apple Developer ID certificate. This is an unsigned evaluation DMG."
fi
ditto "$APP" "package-root/Library/Audio/Plug-Ins/VST3/DC-23.vst3"
chmod +x installer/macos/postinstall
pkgbuild --root package-root --scripts installer/macos --identifier com.dioalexmusictools.dc23.vst3.installer --version 1.0.0 --install-location / dist/DC-23-v1.0-unsigned.pkg
if [[ "$SIGNED" == true ]]; then
  productsign --sign "$MAC_INSTALLER_IDENTITY" dist/DC-23-v1.0-unsigned.pkg dist/DC-23-v1.0.pkg
  pkgutil --check-signature dist/DC-23-v1.0.pkg
  rm dist/DC-23-v1.0-unsigned.pkg
else
  mv dist/DC-23-v1.0-unsigned.pkg dist/DC-23-v1.0.pkg
fi
NOTARIZED=false
if [[ "$SIGNED" == true && -n "$NOTARY_APPLE_ID" && -n "$NOTARY_TEAM_ID" && -n "$NOTARY_APP_PASSWORD" ]]; then
  xcrun notarytool submit dist/DC-23-v1.0.pkg --apple-id "$NOTARY_APPLE_ID" --team-id "$NOTARY_TEAM_ID" --password "$NOTARY_APP_PASSWORD" --wait
  xcrun stapler staple dist/DC-23-v1.0.pkg
  xcrun stapler validate dist/DC-23-v1.0.pkg
  NOTARIZED=true
else
  echo "::warning::PKG not notarized: not suitable for public Mac distribution."
fi
cp dist/DC-23-v1.0.pkg "dmg-content/Install DC-23.pkg"
cp installer/macos/INSTALL.txt "dmg-content/READ ME.txt"
hdiutil create -volname "DC-23 v1.0" -srcfolder dmg-content -format UDZO -ov dist/DC-23-v1.0-macOS-universal.dmg
hdiutil verify dist/DC-23-v1.0-macOS-universal.dmg
if [[ "$NOTARIZED" == true ]]; then
  codesign --force --sign "$MAC_APP_IDENTITY" --timestamp dist/DC-23-v1.0-macOS-universal.dmg
  xcrun notarytool submit dist/DC-23-v1.0-macOS-universal.dmg --apple-id "$NOTARY_APPLE_ID" --team-id "$NOTARY_TEAM_ID" --password "$NOTARY_APP_PASSWORD" --wait
  xcrun stapler staple dist/DC-23-v1.0-macOS-universal.dmg
  xcrun stapler validate dist/DC-23-v1.0-macOS-universal.dmg
  echo "Mac release signed and notarized."
fi

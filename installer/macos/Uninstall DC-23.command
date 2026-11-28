#!/bin/bash
# Only removes the public DC-23 VST3 bundle and its installer receipt.
# Does not touch any other VST3 plugins, system audio files or FL Studio.
set -euo pipefail
TARGET="/Library/Audio/Plug-Ins/VST3/DC-23.vst3"
RECEIPT="com.dioalexmusictools.dc23.vst3.installer"
echo "DC-23 v1.0 — Uninstall"
echo "The only plugin removed will be:"
echo "  $TARGET"
echo
read -r -p "Remove DC-23? [y/N]: " reply
case "$reply" in [yY]|[yY][eE][sS]) ;; *) echo "Cancelled."; exit 0;; esac
if [[ -d "$TARGET" ]]; then
    echo "Administrator permission is required to remove DC-23."
    sudo /bin/rm -rf "$TARGET"
fi
if /usr/sbin/pkgutil --pkg-info "$RECEIPT" >/dev/null 2>&1; then
    sudo /usr/sbin/pkgutil --forget "$RECEIPT" >/dev/null || true
fi
echo "DC-23 has been removed. Restart FL Studio and rescan plugins."
read -r -p "Press Return to close this window." _

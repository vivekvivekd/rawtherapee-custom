#!/usr/bin/env bash
# Bundle, re-sign and install the custom RawTherapee build.
#
# Run from the repo root after `make -C build install` has succeeded:
#   tools/osx/install-custom.sh
#
# Why the re-sign step exists:
#  - The official app's bundle id is com.rawtherapee.rawtherapee5 (the source
#    tree says com.rawtherapee.RawTherapee). Settings live in the sandbox
#    container keyed on that id, so the custom build must use the same one.
#  - Ad-hoc signed dylibs fail hardened-runtime library validation
#    ("different Team IDs"), so we sign WITHOUT `-o runtime`.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="$ROOT/build"
ID=com.rawtherapee.rawtherapee5
APP="$BUILD/RawTherapee.app"
DEST=/Applications/RawTherapee.app

export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:/usr/bin:/bin:/usr/sbin:/sbin"

echo "== bundling"
make -C "$BUILD" macosx_bundle > "$BUILD/bundle.log" 2>&1

echo "== re-signing as $ID"
plutil -replace CFBundleIdentifier -string "$ID" "$APP/Contents/Info.plist"
sed "s/com.rawtherapee.RawTherapee/$ID/" "$BUILD/Release/rt.entitlements" > "$BUILD/Release/rt5.entitlements"

sign() {
    codesign --force --strict -s - -i "$ID" --entitlements "$BUILD/Release/rt5.entitlements" "$1" 2>&1 \
        | grep -v "replacing existing signature" || true
}

for f in "$APP"/Contents/Frameworks/*; do sign "$f"; done
find "$APP/Contents/Resources" -type f \( -name "*.so" -o -name "*.dylib" \) | while read -r f; do sign "$f"; done
sign "$APP/Contents/MacOS/rawtherapee-cli"
sign "$APP/Contents/MacOS/rawtherapee"
sign "$APP"
codesign --verify --deep --strict "$APP"

echo "== installing to $DEST"
if pgrep -x rawtherapee >/dev/null; then
    osascript -e 'tell application "RawTherapee" to quit' || true
    sleep 3
fi
if [[ -d "$DEST" ]]; then
    osascript -e "tell application \"Finder\" to delete POSIX file \"$DEST\"" >/dev/null
fi
ditto "$APP" "$DEST"
codesign -dv "$DEST" 2>&1 | grep "^Identifier"
echo "done: $DEST"

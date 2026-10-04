#!/bin/zsh
# HitLaunchDmx installer — double-click to install the standalone app on THIS
# machine. Copies HitLaunchDmx.app from the folder this script sits in, then
# strips the quarantine flag so Gatekeeper doesn't block the ad-hoc-signed
# binary.
#
# The bundle is an Apple Silicon (arm64) build, macOS 12.0+.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"

if [ ! -d "$HERE/HitLaunchDmx.app" ]; then
  echo "ERROR: HitLaunchDmx.app not found next to this script ($HERE)." >&2
  exit 1
fi

APP_DEST="/Applications"
[ -w "$APP_DEST" ] || APP_DEST="$HOME/Applications"
mkdir -p "$APP_DEST"

echo "Installing HitLaunchDmx from: $HERE"

rm -rf "$APP_DEST/HitLaunchDmx.app"
cp -R "$HERE/HitLaunchDmx.app" "$APP_DEST/"
xattr -dr com.apple.quarantine "$APP_DEST/HitLaunchDmx.app" 2>/dev/null || true
echo "  Standalone -> $APP_DEST/HitLaunchDmx.app"

echo "Done. Launch it from $APP_DEST (or Spotlight: \"HitLaunchDmx\")."

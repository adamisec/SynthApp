#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BINARY="$BUILD_DIR/SynthApp_artefacts/Release/Standalone/SynthApp G2"

# Zbuduj jeśli binary nie istnieje
if [ ! -f "$BINARY" ]; then
    notify-send "SynthApp G2" "Budowanie aplikacji..." 2>/dev/null || true
    cmake --build "$BUILD_DIR" --target SynthApp -j$(nproc)
fi

exec "$BINARY" "$@"

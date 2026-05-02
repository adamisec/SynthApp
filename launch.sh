#!/bin/bash
set -e

BINARY="/home/optiplex3040/synthapp/build/SynthApp_artefacts/Release/Standalone/SynthApp G2"
BUILD_DIR="/home/optiplex3040/synthapp/build"

# Zbuduj jeśli binary nie istnieje
if [ ! -f "$BINARY" ]; then
    notify-send "SynthApp G2" "Budowanie aplikacji..." 2>/dev/null || true
    cmake --build "$BUILD_DIR" --target SynthApp -j$(nproc)
fi

exec "$BINARY" "$@"

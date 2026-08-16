#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

# 1. Build the dashboard binary with nxdk
PATH=$PATH:$DIR/nxdk/bin make

# 2. If the HDD image does not exist, build it once
if [ ! -f "xbox_hdd.qcow2" ]; then
    echo "==> xbox_hdd.qcow2 not found. Generating initial image..."
    ./scripts/build_hdd.sh
fi

# 3. Launch xemu
XEMU_CMD="flatpak run app.xemu.xemu"
if [ -n "$CONTAINER_ID" ] || [ -f "/run/.containerenv" ] || [ -f "/run/.toolboxenv" ] || [ -d "/run/host" ]; then
    if command -v distrobox-host-exec >/dev/null 2>&1; then
        XEMU_CMD="distrobox-host-exec flatpak run app.xemu.xemu"
    fi
fi

$XEMU_CMD

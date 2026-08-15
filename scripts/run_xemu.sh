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

XEMU='distrobox-host-exec flatpak run app.xemu.xemu'

# 3. Launch xemu
$XEMU

#!/bin/bash
set -e

# Note: xemu settings already point to HDD image generate by this repo
# to avoid an expensive copy of very large image

# Build dashboard and rebuild HDD image with injected configs
./scripts/build_hdd.sh

sudo chown jeffl:jeffl xbox_hdd.qcow2
XEMU='distrobox-host-exec flatpak run app.xemu.xemu'

# Deploy generated HDD image to xemu's configured path
$XEMU

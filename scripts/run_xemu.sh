#!/bin/bash
set -e

# Note: xemu settings already point to HDD image generate by this repo
# to avoid an expensive copy of very large image

sudo chown jeffl:jeffl xbox_hdd.qcow2
XEMU='distrobox-host-exec flatpak run app.xemu.xemu'

# Build the project using nxdk
make

# Deploy generated HDD image to xemu's configured path
$XEMU

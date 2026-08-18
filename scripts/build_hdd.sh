#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

echo "==> 1. Compiling OGX-XMB Dashboard..."
PATH=$PATH:$DIR/nxdk/bin make

echo "==> 2. Preparing C: partition payload..."
mkdir -p c_part/Cerbios
cp bin/default.xbe c_part/evoxdash.xbe
cp bin/default.xbe c_part/xboxdash.xbe
cp bin/default.xbe c_part/default.xbe
cp -r assets c_part/

echo "==> 3. Preparing E: partition payload..."
mkdir -p e_part/Cerbios e_part/Dash
cp bin/default.xbe e_part/evoxdash.xbe
cp bin/default.xbe e_part/default.xbe
cp bin/default.xbe e_part/Dash/default.xbe

# Inject cerbios.ini if present in root (ensure CRLF line endings)
if [ -f "cerbios.ini" ]; then
    echo "==> Injecting cerbios.ini into C:\\cerbios.ini, C:\\Cerbios\\, and E:\\Cerbios\\..."
    sed 's/$/\r/' cerbios.ini > c_part/cerbios.ini
    sed 's/$/\r/' cerbios.ini > c_part/Cerbios/cerbios.ini
    sed 's/$/\r/' cerbios.ini > e_part/Cerbios/cerbios.ini
fi

# Inject ind-bios.cfg if present in root
if [ -f "ind-bios.cfg" ]; then
    echo "==> Injecting ind-bios.cfg into C:\\ind-bios.cfg..."
    sed 's/$/\r/' ind-bios.cfg > c_part/ind-bios.cfg
fi

# Sync local Games directory payload to F:\Games (using extended F: partition for full library capacity)
rm -rf f_part
if [ -d "Games" ]; then
    echo "==> 4. Preparing F: partition payload (Games library)..."
    mkdir -p f_part/Games
    for g in Games/*; do
        if [ -d "$g" ]; then
            bname=$(basename "$g")
            clean_name=$(echo "$bname" | tr -d ',')
            
            # Check for FATX non-compliance issues and issue clear warnings
            if [ "$bname" != "$clean_name" ]; then
                echo "    ⚠️ WARNING: '$bname' contains illegal characters (commas) for FATX. Sanitized to: '$clean_name'"
            fi
            
            if [ ${#clean_name} -gt 42 ]; then
                truncated_name="${clean_name:0:42}"
                echo "    ⚠️ WARNING: '$clean_name' exceeds FATX 42-character limit (${#clean_name} chars). Truncated to: '$truncated_name'"
                clean_name="$truncated_name"
            fi
            
            echo "    -> Staging game: '$clean_name'..."
            mkdir -p "f_part/Games/$clean_name"
            cp -ru "$g"/* "f_part/Games/$clean_name/" 2>/dev/null || cp -r "$g"/* "f_part/Games/$clean_name/"
        fi
    done
fi

echo "==> 5. Repacking c.zip, e.zip, and f.zip..."
rm -f "$DIR/c.zip" "$DIR/e.zip" "$DIR/f.zip"
(cd c_part && zip -q -r "$DIR/c.zip" .)
(cd e_part && zip -q -r "$DIR/e.zip" .)

F_ARG=""
if [ -d "f_part" ]; then
    (cd f_part && zip -q -r "$DIR/f.zip" .)
    F_ARG="-f /data/f.zip"
fi

# Calculate required image size dynamically:
# Retail Xbox partitions (C, E, X, Y, Z) always consume the first 8 GB (offset 0x1DD156000).
# Partition F receives all space beyond 8 GB.
# Image size = 8 GB base + F payload + buffer
F_PAYLOAD_KB=$(du -sk f_part 2>/dev/null | awk '{s+=$1} END {print s}')
F_PAYLOAD_GB=$(( (F_PAYLOAD_KB + 1024 * 1024 - 1) / (1024 * 1024) ))
BASE_RETAIL_GB=8
BUFFER_GB=4
IMAGE_SIZE_GB=$(( BASE_RETAIL_GB + F_PAYLOAD_GB + BUFFER_GB ))

if [ "$IMAGE_SIZE_GB" -lt "$BASE_RETAIL_GB" ]; then
    IMAGE_SIZE_GB="$BASE_RETAIL_GB"
fi

echo "==> 6. Building xbox_hdd.qcow2 disk image (Size: ${IMAGE_SIZE_GB}GB [8GB base + ${F_PAYLOAD_GB}GB games + ${BUFFER_GB}GB buffer])..."
rm -f xbox_hdd.bin xbox_hdd.qcow2 xbox_hdd.qcow2.tmp

# Prefix with distrobox-host-exec if running inside a distrobox container
DOCKER_CMD="docker"
if [ -n "$CONTAINER_ID" ] || [ -f "/run/.containerenv" ] || [ -f "/run/.toolboxenv" ] || [ -d "/run/host" ]; then
    if command -v distrobox-host-exec >/dev/null 2>&1; then
        DOCKER_CMD="distrobox-host-exec docker"
    fi
fi

$DOCKER_CMD run --rm --user "$(id -u):$(id -g)" -v "$(pwd)":/data jeffbrl/ogxbox-image-builder python3 /app/main.py /data/xbox_hdd.qcow2 -c /data/c.zip -e /data/e.zip $F_ARG -s "$IMAGE_SIZE_GB" -t qcow2

echo "==> Build complete: xbox_hdd.qcow2 is ready (${IMAGE_SIZE_GB}GB)!"

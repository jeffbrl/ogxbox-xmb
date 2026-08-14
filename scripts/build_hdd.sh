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

echo "==> 4. Repacking c.zip and e.zip..."
cd c_part
rm -f ../c.zip
zip -q -r ../c.zip *
cd ../e_part
rm -f ../e.zip
zip -q -r ../e.zip *
cd ..

echo "==> 5. Building xbox_hdd.qcow2 disk image..."
rm -f xbox_hdd.bin xbox_hdd.qcow2 xbox_hdd.qcow2.tmp
distrobox-host-exec docker run --rm -v "$(pwd)":/data jeffbrl/ogxbox-image-builder python3 /app/main.py /data/xbox_hdd.qcow2 -c /data/c.zip -e /data/e.zip -t qcow2

echo "==> Build complete: xbox_hdd.qcow2 is ready!"

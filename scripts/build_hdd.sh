#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

echo "==> 1. Compiling OGX-XMB Dashboard..."
PATH=$PATH:$DIR/nxdk/bin make

echo "==> 2. Preparing C: partition payload..."
mkdir -p c_part
cp bin/default.xbe c_part/evoxdash.xbe
cp bin/default.xbe c_part/xboxdash.xbe
cp bin/default.xbe c_part/default.xbe
cp -r assets c_part/

# Inject cerbios.ini if present in root
if [ -f "cerbios.ini" ]; then
    echo "==> Injecting root cerbios.ini into C:\\cerbios.ini..."
    cp cerbios.ini c_part/cerbios.ini
fi

# Inject ind-bios.cfg if present in root
if [ -f "ind-bios.cfg" ]; then
    echo "==> Injecting root ind-bios.cfg into C:\\ind-bios.cfg..."
    cp ind-bios.cfg c_part/ind-bios.cfg
fi

echo "==> 3. Repacking c.zip..."
cd c_part
rm -f ../c.zip
zip -q -r ../c.zip *
cd ..

echo "==> 4. Building xbox_hdd.qcow2 disk image..."
rm -f xbox_hdd.bin xbox_hdd.qcow2 xbox_hdd.qcow2.tmp
distrobox-host-exec docker run --rm -v "$(pwd)":/data jeffbrl/ogxbox-image-builder python3 /app/main.py /data/xbox_hdd.qcow2 -c /data/c.zip -e /data/e.zip -t qcow2

echo "==> Build complete: xbox_hdd.qcow2 is ready!"

import struct
import zlib
import os
import math

def write_png(filename, width, height, rgba_data):
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)

    raw_data = bytearray()
    for y in range(height):
        raw_data.append(0)  # filter type 0 (None)
        raw_data.extend(rgba_data[y * width * 4:(y + 1) * width * 4])

    compressed = zlib.compress(raw_data, 9)
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) + chunk(b'IDAT', compressed) + chunk(b'IEND', b'')
    
    with open(filename, 'wb') as f:
        f.write(png)

def make_gamepad(w=64, h=64):
    buf = bytearray(w * h * 4)
    cx, cy = w / 2, h / 2
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            # Body: rounded pill
            dx = x - cx
            dy = (y - cy) * 1.3
            dist = math.sqrt(dx * dx + dy * dy)
            
            # Left grip, right grip
            lg = math.sqrt((x - (cx - 16))**2 + (y - (cy + 6))**2)
            rg = math.sqrt((x - (cx + 16))**2 + (y - (cy + 6))**2)
            
            is_body = dist < 20 or lg < 12 or rg < 12
            if is_body:
                alpha = 240
                # Border highlight
                if dist > 18 or (lg > 10 and lg < 12) or (rg > 10 and rg < 12):
                    buf[idx:idx+4] = bytes([255, 255, 255, 255])
                else:
                    buf[idx:idx+4] = bytes([210, 225, 240, alpha])
            
            # D-pad cutout on left
            if (abs(x - (cx - 12)) < 5 and abs(y - cy) < 2) or (abs(x - (cx - 12)) < 2 and abs(y - cy) < 5):
                buf[idx:idx+4] = bytes([40, 45, 55, 255])
            
            # Buttons on right
            for bx, by in [(cx + 12, cy - 3), (cx + 15, cy), (cx + 9, cy), (cx + 12, cy + 3)]:
                if (x - bx)**2 + (y - by)**2 < 2.5:
                    buf[idx:idx+4] = bytes([40, 45, 55, 255])
    return buf

def make_gear(w=64, h=64):
    buf = bytearray(w * h * 4)
    cx, cy = w / 2, h / 2
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            dx = x - cx
            dy = y - cy
            dist = math.sqrt(dx * dx + dy * dy)
            angle = math.atan2(dy, dx)
            
            # 8 teeth
            teeth = 18 + 4 * math.sin(8 * angle)
            if dist < teeth and dist > 7:
                if dist > teeth - 1.5 or dist < 8.5:
                    buf[idx:idx+4] = bytes([255, 255, 255, 255])
                else:
                    buf[idx:idx+4] = bytes([210, 225, 240, 240])
    return buf

def make_grid(w=64, h=64):
    buf = bytearray(w * h * 4)
    cx, cy = w / 2, h / 2
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            # 4 squares
            for sx, sy in [(cx - 11, cy - 11), (cx + 11, cy - 11), (cx - 11, cy + 11), (cx + 11, cy + 11)]:
                if abs(x - sx) < 8 and abs(y - sy) < 8:
                    if abs(x - sx) >= 7 or abs(y - sy) >= 7:
                        buf[idx:idx+4] = bytes([255, 255, 255, 255])
                    else:
                        buf[idx:idx+4] = bytes([210, 225, 240, 240])
    return buf

def make_disc(w=64, h=64):
    buf = bytearray(w * h * 4)
    cx, cy = w / 2, h / 2
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            dx = x - cx
            dy = y - cy
            dist = math.sqrt(dx * dx + dy * dy)
            if dist < 22 and dist > 6:
                if dist > 20.5 or dist < 7.5:
                    buf[idx:idx+4] = bytes([255, 255, 255, 255])
                elif dist > 11 and dist < 13:
                    buf[idx:idx+4] = bytes([160, 180, 200, 180])
                else:
                    # Subtle metallic sheen
                    angle = math.atan2(dy, dx)
                    sheen = int(200 + 40 * math.sin(3 * angle))
                    buf[idx:idx+4] = bytes([sheen, min(255, sheen + 10), min(255, sheen + 20), 230])
    return buf

def make_info(w=64, h=64):
    buf = bytearray(w * h * 4)
    cx, cy = w / 2, h / 2
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            dx = x - cx
            dy = y - cy
            dist = math.sqrt(dx * dx + dy * dy)
            # Outer ring
            if dist < 22 and dist > 18:
                buf[idx:idx+4] = bytes([255, 255, 255, 255])
            elif dist <= 18:
                # Dot of the 'i'
                if abs(dx) <= 2.5 and (y >= cy - 12 and y <= cy - 7):
                    buf[idx:idx+4] = bytes([255, 255, 255, 255])
                # Stem of the 'i'
                elif abs(dx) <= 2.5 and (y >= cy - 3 and y <= cy + 11):
                    buf[idx:idx+4] = bytes([255, 255, 255, 255])
                # Base bar of the 'i'
                elif abs(dx) <= 5.5 and (y >= cy + 9 and y <= cy + 11):
                    buf[idx:idx+4] = bytes([255, 255, 255, 255])
                # Top serif of the 'i'
                elif (dx >= -5.5 and dx <= 0) and (y >= cy - 3 and y <= cy - 1):
                    buf[idx:idx+4] = bytes([255, 255, 255, 255])
    return buf

if __name__ == '__main__':
    os.makedirs('assets/icons', exist_ok=True)
    write_png('assets/icons/cat_games.png', 64, 64, make_gamepad())
    write_png('assets/icons/cat_settings.png', 64, 64, make_gear())
    write_png('assets/icons/cat_info.png', 64, 64, make_info())
    write_png('assets/icons/cat_apps.png', 64, 64, make_grid())
    write_png('assets/icons/default_game.png', 64, 64, make_disc())
    print("Generated icons successfully.")


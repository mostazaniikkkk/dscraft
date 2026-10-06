"""Build nitrofiles/dscraft/chest.bin (chest screen) from inventory.bin.

The chest screen keeps the inventory and hotbar slots, and replaces the upper
part of the window (armor slots, player preview, 2x2 crafting grid) with three
rows of nine slots, copied from the inventory's own grid so they line up with
it, as in Minecraft's chest screen.

Usage: python tools/make_chest_bin.py [preview.png]
Slot positions used by the game are printed and must match source/game/inventory.c.
"""
import collections
import os
import struct
import sys
import zlib

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
SRC = os.path.join(ROOT, 'nitrofiles', 'dscraft', 'inventory.bin')
DST = os.path.join(ROOT, 'nitrofiles', 'dscraft', 'chest.bin')


def load(path):
    data = open(path, 'rb').read()
    img = [[0] * 256 for _ in range(192)]
    for sy in range(3):
        for sx in range(4):
            base = 512 + (sx + 4 * sy) * 4096
            for t in range(64):
                for py in range(8):
                    for px in range(8):
                        img[sy*64 + (t//8)*8 + py][sx*64 + (t % 8)*8 + px] = data[base + t*64 + py*8 + px]
    return data[:512], img, data[512 + 12 * 4096:]


def save(path, pal, img, tail):
    gfx = bytearray(12 * 4096)
    for sy in range(3):
        for sx in range(4):
            base = (sx + 4 * sy) * 4096
            for t in range(64):
                for py in range(8):
                    for px in range(8):
                        gfx[base + t*64 + py*8 + px] = img[sy*64 + (t//8)*8 + py][sx*64 + (t % 8)*8 + px]
    open(path, 'wb').write(pal + bytes(gfx) + tail)


pal, img, tail = load(SRC)

# panel background: the most common colour of the crafting panel
counts = collections.Counter(img[y][x] for y in range(42, 98) for x in range(176, 212))
bg = counts.most_common(1)[0][0]

# the inventory's three rows of slots with their frame: cells at x 55+18c, y 110+18r
grid = [row[53:218] for row in img[108:165]]

# clear the upper part of the window, below the top buckles
for y in range(38, 101):   # inside the frame: below its top edge, above the separator
    for x in range(55, 214):  # between the left pipe and the right edge
        img[y][x] = bg

GX, GY = 53, 39
for j, row in enumerate(grid):
    img[GY + j][GX:GX + len(row)] = row

save(DST, pal, img, tail)
print('chest.bin written; chest cells at x', [GX + 2 + 18 * c for c in range(9)], 'y', [GY + 2 + 18 * r for r in range(3)])

if len(sys.argv) > 1:
    p = struct.unpack('<256H', pal)
    raw = b''
    for y in range(192 * 2):
        raw += b'\x00' + b''.join(bytes((((p[img[y//2][x//2]] >> s) & 31) << 3 for s in (0, 5, 10))) for x in range(256 * 2))

    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    open(sys.argv[1], 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 512, 384, 8, 2, 0, 0, 0))
                                  + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))

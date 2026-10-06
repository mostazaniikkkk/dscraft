"""Build nitrofiles/dscraft/crafting.bin (crafting table screen) from inventory.bin.

The sub-screen images are a 256-colour palette followed by 12 sprites of 64x64
pixels (8x8 tiles, 1D mapping). The crafting table screen keeps the inventory
and hotbar slots, and replaces the player preview and the 2x2 grid with a 3x3
grid, the arrow and the result slot, all cut from the original art.

Usage: python tools/make_crafting_bin.py [preview.png]
Slot positions used by the game are printed and must match source/game/inventory.c.
"""
import collections
import os
import struct
import sys
import zlib

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
SRC = os.path.join(ROOT, 'nitrofiles', 'dscraft', 'inventory.bin')
DST = os.path.join(ROOT, 'nitrofiles', 'dscraft', 'crafting.bin')


def load(path):
    data = open(path, 'rb').read()
    pal = data[:512]
    gfx = data[512:512 + 12 * 4096]
    img = [[0] * 256 for _ in range(192)]
    for sy in range(3):
        for sx in range(4):
            base = (sx + 4 * sy) * 4096
            for t in range(64):
                for py in range(8):
                    for px in range(8):
                        img[sy*64 + (t//8)*8 + py][sx*64 + (t % 8)*8 + px] = gfx[base + t*64 + py*8 + px]
    return pal, img, data[512 + 12 * 4096:]


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


def crop(img, x, y, w, h):
    return [row[x:x + w] for row in img[y:y + h]]


def paste(img, part, x, y):
    for j, row in enumerate(part):
        img[y + j][x:x + len(row)] = row


pal, img, tail = load(SRC)

# panel background colour: the most common colour of the crafting panel
counts = collections.Counter(img[y][x] for y in range(42, 98) for x in range(176, 212))
bg = counts.most_common(1)[0][0]

# original pieces: 2x2 grid frame (38x38), result frame (20x20), arrow
grid2 = crop(img, 133, 50, 38, 38)
result = crop(img, 189, 60, 20, 20)
arrow = crop(img, 173, 62, 16, 16)

# 3x3 frame: frame+cell+separator, cell+separator, cell+frame along both axes
cols = list(range(0, 20)) + list(range(2, 20)) + list(range(20, 38))
grid3 = [[grid2[r][c] for c in cols] for r in cols]

# clear the player preview and the old crafting area (below the top buckles)
for y in range(42, 101):
    for x in range(73, 214):
        img[y][x] = bg

GRIDX, GRIDY = 84, 42
paste(img, grid3, GRIDX, GRIDY)
paste(img, arrow, 148, 62)
paste(img, result, 168, 60)

save(DST, pal, img, tail)
print('crafting.bin written')
print('3x3 cells at x', [GRIDX + 2 + 18 * i for i in range(3)], 'y', [GRIDY + 2 + 18 * i for i in range(3)])
print('result cell at', (170, 62))

if len(sys.argv) > 1:
    p = struct.unpack('<256H', pal)
    raw = b''
    for y in range(192 * 2):
        raw += b'\x00' + b''.join(bytes((((p[img[y//2][x//2]] >> s) & 31) << 3 for s in (0, 5, 10))) for x in range(256 * 2))

    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    open(sys.argv[1], 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 512, 384, 8, 2, 0, 0, 0))
                                  + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))

"""Build nitrofiles/dscraft/furnace.bin (furnace screen) from inventory.bin.

The furnace screen keeps the inventory and hotbar slots, and replaces the upper
part of the window with Minecraft Beta's furnace layout taken from the pack's
gui/furnace.png (input and fuel slots, the empty flame and arrow, the large
output slot). Beta's screen coordinates map to the DS screen with +47, +26,
which puts the inventory slots exactly where inventory.bin has them.

Needs Pillow. Usage: python tools/make_furnace_bin.py [preview.png]
Positions must match source/game/inventory.c (FURNACEINX ... ARROWY).
"""
import collections
import os
import sys

from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
SRC = os.path.join(ROOT, 'nitrofiles', 'dscraft', 'inventory.bin')
DST = os.path.join(ROOT, 'nitrofiles', 'dscraft', 'furnace.bin')
GUI = os.path.join(ROOT, 'dscraft', 'packs', 'minecraft', 'gui', 'furnace.png')
OX, OY = 47, 26


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
colors = [(c & 31, (c >> 5) & 31, (c >> 10) & 31) for c in (int.from_bytes(pal[2*i:2*i+2], 'little') for i in range(256))]

counts = collections.Counter(img[y][x] for y in range(42, 98) for x in range(176, 212))
bg = counts.most_common(1)[0][0]

for y in range(38, 101):   # inside the frame: below its top edge, above the separator
    for x in range(55, 214):  # between the left pipe and the right edge
        img[y][x] = bg

gui = Image.open(GUI).convert('RGBA')
gbg = gui.getpixel((40, 40))[:3]


def nearest(rgb):
    r, g, b = (v >> 3 for v in rgb)
    return min(range(1, 256), key=lambda i: (colors[i][0]-r)**2 + (colors[i][1]-g)**2 + (colors[i][2]-b)**2)


# input and fuel slots: the inventory's own slot frame (cell at 55,110), as on the chest screen
cell = [row[54:72] for row in img[109:127]]
for (x0, y0) in [(55, 16), (55, 52)]:
    for y in range(18):
        img[OY + y0 + y][OX + x0:OX + x0 + 18] = cell[y]

cache = {}
# empty flame, empty arrow, large output slot (Beta coordinates)
for (x0, y0, w, h) in [(56, 36, 14, 14), (79, 34, 24, 17), (111, 30, 26, 26)]:
    for y in range(h):
        for x in range(w):
            p = gui.getpixel((x0 + x, y0 + y))
            if p[3] == 0 or p[:3] == gbg:
                continue
            if p[:3] not in cache:
                cache[p[:3]] = nearest(p[:3])
            img[OY + y0 + y][OX + x0 + x] = cache[p[:3]]

save(DST, pal, img, tail)
print('furnace.bin written; input', (OX + 56, OY + 17), 'fuel', (OX + 56, OY + 53), 'output', (OX + 116, OY + 35),
      'flame', (OX + 56, OY + 36), 'arrow', (OX + 79, OY + 34))

if len(sys.argv) > 1:
    out = Image.new('RGB', (256, 192))
    for y in range(192):
        for x in range(256):
            c = colors[img[y][x]]
            out.putpixel((x, y), (c[0] << 3, c[1] << 3, c[2] << 3))
    out.resize((512, 384), Image.NEAREST).save(sys.argv[1])

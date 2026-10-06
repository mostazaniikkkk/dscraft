"""Build nitrofiles/dscraft/pause.bin (the game menu shown while paused).

It keeps the frame of the inventory window (inventory.bin), empties its two
panels, writes the title in the upper one and draws three buttons in the
lower one with the menu's button texture (menu/button.pcx) and the HUD font
(font/HUD.pcx), like Minecraft's "Game menu".

Needs Pillow. Usage: python tools/make_pause_bin.py [preview.png]
The button rectangles printed must match source/game/interface.c (PAUSE_*).
"""
import collections
import os
import sys

from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
NITRO = os.path.join(ROOT, 'nitrofiles', 'dscraft')
SRC = os.path.join(NITRO, 'inventory.bin')
DST = os.path.join(NITRO, 'pause.bin')

TITLE = 'Game menu'
BUTTONS = ['Back to game', 'Screenshot', 'Save and quit']
BX, BW, BH = 59, 152, 20                 # buttons: left, width, height
BY = [112, 136, 160]                     # tops


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
cache = {}


def nearest(rgb):
    if rgb not in cache:
        r, g, b = (v >> 3 for v in rgb)
        cache[rgb] = min(range(1, 256), key=lambda i: (colors[i][0]-r)**2 + (colors[i][1]-g)**2 + (colors[i][2]-b)**2)
    return cache[rgb]


bg = collections.Counter(img[y][x] for y in range(42, 98) for x in range(176, 212)).most_common(1)[0][0]
# empty both panels inside the frame
for y in range(38, 101):
    for x in range(55, 214):
        img[y][x] = bg
for y in range(109, 185):
    for x in range(55, 215):
        img[y][x] = bg

# the HUD font: 16x16 cells from ' ', 32 per row, glyphs on magenta
font = Image.open(os.path.join(NITRO, 'font', 'HUD.pcx')).convert('RGB')
KEY = font.getpixel((0, 0))


def glyph(c):
    n = ord(c) - 32
    gx, gy = (n % 32) * 16, (n // 32) * 16
    cols = [x for x in range(16) if any(font.getpixel((gx + x, gy + y)) != KEY for y in range(16))]
    if not cols:
        return gx, gy, 0, 5                                  # space
    return gx + cols[0], gy, cols[-1] - cols[0] + 1, cols[-1] - cols[0] + 1


def text_width(s):
    return sum(glyph(c)[3] for c in s) + len(s) - 1


def draw_text(s, cx, top):
    x = cx - text_width(s) // 2
    for c in s:
        gx, gy, w, adv = glyph(c)
        for yy in range(16):
            for xx in range(w):
                p = font.getpixel((gx + xx, gy + yy))
                if p != KEY and 0 <= top + yy < 192:
                    img[top + yy][x + xx] = nearest(p)
        x += adv + 1


button = Image.open(os.path.join(NITRO, 'menu', 'button.pcx')).convert('RGB')
BKEY = button.getpixel((0, 0))
button = button.resize((BW, BH), Image.NEAREST)
for i, label in enumerate(BUTTONS):
    for yy in range(BH):
        for xx in range(BW):
            p = button.getpixel((xx, yy))
            if p != BKEY:
                img[BY[i] + yy][BX + xx] = nearest(p)
    draw_text(label, BX + BW // 2, BY[i] + 2)
draw_text(TITLE, 134, 61)

save(DST, pal, img, tail)
print('pause.bin written; buttons x %d..%d, y %s, height %d' % (BX, BX + BW, BY, BH))

if len(sys.argv) > 1:
    out = Image.new('RGB', (256, 192))
    for y in range(192):
        for x in range(256):
            c = colors[img[y][x]]
            out.putpixel((x, y), (c[0] << 3, c[1] << 3, c[2] << 3))
    out.resize((512, 384), Image.NEAREST).save(sys.argv[1])

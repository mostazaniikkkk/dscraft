"""Convert the sub-screen GUI backgrounds (nitrofiles/dscraft/*.bin) to PNG
for editing, and back.

A .bin is a 256-colour palette (RGB15, colour 0 transparent), the 256x192
screen as 12 blocks of 64x64 pixels in 8x8 tiles, and a tail with more sprite
graphics drawn with the same palette (the pressed look of the top buttons).

  python tools/gui_png.py export [name ...]   ->  art/gui/<name>.png
  python tools/gui_png.py import name [...]   ->  nitrofiles/dscraft/<name>.bin

Names are crafting, furnace, chest, pause (or any .bin of the same kind).
On import the palette and the tail are kept: colours already in the palette
keep their entry, new colours take entries nothing else uses, and only when
those run out are they matched to the nearest palette colour (the import says
so). Transparent pixels (alpha below 128) become colour 0. Colours are 5 bits
per channel on the DS, so the lowest 3 bits of each channel are dropped.

Note: tools/make_chest_bin.py, make_furnace_bin.py and make_pause_bin.py
build their .bin from inventory.bin and would overwrite hand edits.

Needs Pillow.
"""
import os
import sys

from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
BINS = os.path.join(ROOT, 'nitrofiles', 'dscraft')
PNGS = os.path.join(ROOT, 'art', 'gui')
DEFAULT = ['crafting', 'furnace', 'chest', 'pause']
SCREEN = 12 * 4096


def read_bin(path):
    data = open(path, 'rb').read()
    pal = [int.from_bytes(data[2*i:2*i+2], 'little') for i in range(256)]
    img = [[0] * 256 for _ in range(192)]
    for sy in range(3):
        for sx in range(4):
            base = 512 + (sx + 4 * sy) * 4096
            for t in range(64):
                for py in range(8):
                    for px in range(8):
                        img[sy*64 + (t//8)*8 + py][sx*64 + (t % 8)*8 + px] = data[base + t*64 + py*8 + px]
    return pal, img, data[512 + SCREEN:]


def write_bin(path, pal, img, tail):
    gfx = bytearray(SCREEN)
    for sy in range(3):
        for sx in range(4):
            base = (sx + 4 * sy) * 4096
            for t in range(64):
                for py in range(8):
                    for px in range(8):
                        gfx[base + t*64 + py*8 + px] = img[sy*64 + (t//8)*8 + py][sx*64 + (t % 8)*8 + px]
    head = b''.join((c & 0x7FFF).to_bytes(2, 'little') for c in pal)
    open(path, 'wb').write(head + bytes(gfx) + tail)


def rgb15(c):
    return (c & 31, (c >> 5) & 31, (c >> 10) & 31)


def export(name):
    pal, img, _ = read_bin(os.path.join(BINS, name + '.bin'))
    out = Image.new('RGBA', (256, 192))
    for y in range(192):
        for x in range(256):
            i = img[y][x]
            r, g, b = rgb15(pal[i])
            out.putpixel((x, y), (r << 3, g << 3, b << 3, 0 if i == 0 else 255))
    os.makedirs(PNGS, exist_ok=True)
    path = os.path.join(PNGS, name + '.png')
    out.save(path)
    print('%s -> %s' % (name + '.bin', os.path.relpath(path, ROOT)))


def import_png(name):
    binpath = os.path.join(BINS, name + '.bin')
    pal, old, tail = read_bin(binpath)
    png = Image.open(os.path.join(PNGS, name + '.png')).convert('RGBA')
    if png.size != (256, 192):
        sys.exit('%s.png must be 256x192, it is %dx%d' % (name, png.size[0], png.size[1]))
    keep = set(tail) | {0}                       # entries the tail still needs
    index = {}
    for i in range(1, 256):
        index.setdefault(rgb15(pal[i]), i)
    colours = {}
    for y in range(192):
        for x in range(256):
            r, g, b, a = png.getpixel((x, y))
            if a >= 128:
                colours.setdefault((r >> 3, g >> 3, b >> 3), 0)
    used = set(keep)
    for c in colours:
        if c in index:
            used.add(index[c])
    free = [i for i in range(1, 256) if i not in used]
    added = approx = 0
    for c in colours:
        if c in index:
            continue
        if free:
            i = free.pop(0)
            pal[i] = c[0] | (c[1] << 5) | (c[2] << 10)
            index[c] = i
            added += 1
        else:
            index[c] = min((i for i in range(1, 256)),
                           key=lambda i: sum((p - q) ** 2 for p, q in zip(rgb15(pal[i]), c)))
            approx += 1
    img = [[0] * 256 for _ in range(192)]
    for y in range(192):
        for x in range(256):
            r, g, b, a = png.getpixel((x, y))
            c = (r >> 3, g >> 3, b >> 3)
            o = old[y][x]
            if a < 128:
                img[y][x] = 0
            elif o and rgb15(pal[o]) == c:
                img[y][x] = o                      # unchanged pixel: same entry (the palette repeats colours)
            else:
                img[y][x] = index[c]
    write_bin(binpath, pal, img, tail)
    print('%s.png -> %s.bin: %d colours, %d new in the palette%s' % (
        name, name, len(colours), added, (', %d matched to the nearest (palette full)' % approx) if approx else ''))


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in ('export', 'import'):
        sys.exit(__doc__)
    names = sys.argv[2:] or (DEFAULT if sys.argv[1] == 'export' else [])
    if not names:
        sys.exit('import: which .bin? e.g. python tools/gui_png.py import furnace')
    for n in names:
        (export if sys.argv[1] == 'export' else import_png)(n)

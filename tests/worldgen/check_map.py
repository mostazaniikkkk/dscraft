"""Validate a DScraft .map file against the game's own rules.

Usage: python check_map.py file.map [columns_to_sample]

Column (cx,cy) lives at 2048 + (cx + cy*sizeX)*2048: 1024 block bytes indexed
x + y*4 + k*16 (k = 0..63), then 1024 precalculated bytes with the same indexing:
bits 0-5 visible faces (k-1, k+1, y-1, y+1, x-1, x+1) as computed by
transparent3(), bit 6 sky light as computed by precalcCollumn(), and bit 7 on
bytes 0..2 of each 4x4x4 cluster = occlusion walls (z, y, x).
"""
import collections
import random
import struct
import sys

WATER = 239
VERSIONMAGIC = 150325785
path = sys.argv[1]
samples = int(sys.argv[2]) if len(sys.argv) > 2 else 48
flat = len(sys.argv) > 3 and sys.argv[3] == 'flat'
data = open(path, 'rb').read()
sx, sy, magic, spx, spy, spz = struct.unpack('<HHIHHi', data[:16])
W, H, Z = sx * 4, sy * 4, 64
errors = []


def check(cond, msg):
    if not cond:
        errors.append(msg)


check(len(data) == 2048 + sx * sy * 2048, 'file size %d' % len(data))
check(magic == VERSIONMAGIC, 'magic')
check(0 <= spx < W and 0 <= spy < H, 'spawn outside the map')


def col(cx, cy):
    o = 2048 + (cx + cy * sx) * 2048
    return data[o:o + 1024], data[o + 1024:o + 2048]


cache = {}
def blk(x, y, k):
    if x < 0 or y < 0 or k < 0 or x >= W or y >= H or k >= Z:
        return 5
    c = (x // 4, y // 4)
    if c not in cache:
        cache[c] = col(*c)[0]
    return cache[c][(x % 4) + (y % 4) * 4 + k * 16]


def ladder(t): return 40 <= t < 44
def door(t): return 44 <= t < 60
def plant(t): return t in (95, 96) or 120 <= t < 140   # sapling, carrots, pumpkin stem
def see_through(t): return t == 0 or t >= WATER or t in (13, 12, 10) or plant(t) or ladder(t) or door(t)
def shows(tt, t): return (t == 0 or t >= WATER or t in (12, 13) or plant(t) or ladder(t) or door(t)) and not (t == tt or (t >= WATER and tt >= WATER))


def faces(x, y, k):
    b = blk(x, y, k)
    if not b: return 0
    if door(b) or ladder(b): return 1
    n = [(x, y, k - 1), (x, y, k + 1), (x, y - 1, k), (x, y + 1, k), (x - 1, y, k), (x + 1, y, k)]
    return sum(1 << i for i, p in enumerate(n) if shows(b, blk(*p)))


hc = {}
def highest(x, y):
    if (x, y) not in hc:
        h = 0
        for k in range(Z):
            if faces(x, y, k) & 63 and not see_through(blk(x, y, k)): h = k
        hc[(x, y)] = h
    return hc[(x, y)]


def walls(cx, cy, c):
    w = 7
    for i in range(4):
        for j in range(4):
            d1 = d2 = d3 = False
            for k in range(4):
                d1 = d1 or not see_through(blk(cx*4+i, cy*4+j, c*4+k))
                d2 = d2 or not see_through(blk(cx*4+i, cy*4+k, c*4+j))
                d3 = d3 or not see_through(blk(cx*4+k, cy*4+j, c*4+i))
            w &= d1 | (d2 << 1) | (d3 << 2)
    return w


random.seed(1)
cols = {(0, 0), (sx - 1, sy - 1), (0, sy - 1), (sx - 1, 0), (spx // 4, spy // 4)}
while len(cols) < samples:
    cols.add((random.randrange(sx), random.randrange(sy)))

for cx, cy in sorted(cols):
    b, t = col(cx, cy)
    for k in range(Z):
        for j in range(4):
            for i in range(4):
                n = i + j * 4 + k * 16
                x, y = cx * 4 + i, cy * 4 + j
                check((t[n] & 63) == faces(x, y, k), 'faces %d,%d,%d' % (x, y, k))
                ls = k == highest(x, y)
                if not ls and x < W - 1: ls = k > highest(x + 1, y)
                if not ls and y < H - 1: ls = k > highest(x, y + 1)
                if not ls and x > 0: ls = k > highest(x - 1, y)
                if not ls and y > 0: ls = k > highest(x, y - 1)
                check(((t[n] >> 6) & 1) == ls, 'light %d,%d,%d' % (x, y, k))
    for c in range(16):
        f = ((t[c*64] >> 7) & 1) | (((t[c*64+1] >> 7) & 1) << 1) | (((t[c*64+2] >> 7) & 1) << 2)
        check(f == walls(cx, cy, c), 'walls %d,%d cluster %d' % (cx, cy, c))
        for q in range(3, 64):
            check(not (t[c*64+q] >> 7), 'stray bit 7 at %d,%d' % (cx, cy))
    check(b[0] == 5, 'no bedrock at the bottom of %d,%d' % (cx, cy))

# whole-map content
hist = collections.Counter()
for n in range(sx * sy):
    o = 2048 + n * 2048
    hist.update(data[o:o + 1024])
FARMLAND, CARROT, PUMPKINS = 112, 127, (140, 141, 142, 143)
IRON = 103
allowed = {0, 1, 2, 5} if flat else {0, 1, 2, 3, 5, 6, 8, 10, 61, IRON, WATER, FARMLAND, CARROT} | set(PUMPKINS)
check(set(hist) <= allowed, 'unexpected blocks %s' % (set(hist) - allowed))
for needed in ((1, 2, 5) if flat else (1, 2, 3, 5, 8, 10, 61, IRON)):
    check(hist[needed] > 0, 'block %d missing' % needed)
if not flat:
    # pumpkin and carrot patches: pumpkins on grass, grown carrots on farmland, air above
    pumpkins = carrots = 0
    for n in range(sx * sy):
        o = 2048 + n * 2048
        for k in range(1, Z - 1):
            for q in range(16):
                t = data[o + k * 16 + q]
                if t in PUMPKINS or t == CARROT:
                    below, above = data[o + (k - 1) * 16 + q], data[o + (k + 1) * 16 + q]
                    check(below == (1 if t != CARROT else FARMLAND), 'patch block %d on %d' % (t, below))
                    check(above == 0, 'patch block %d under %d' % (t, above))
                    if t == CARROT: carrots += 1
                    else: pumpkins += 1
                if t == FARMLAND:
                    check(data[o + (k + 1) * 16 + q] == CARROT, 'farmland without carrots')
    check(pumpkins > 0 and carrots > 0, 'no pumpkin or carrot patches (%d, %d)' % (pumpkins, carrots))
    print('patches: %d pumpkins, %d carrots' % (pumpkins, carrots))
if flat:
    # every column: bedrock, two dirt, grass, then air
    layers = [5, 2, 2, 1] + [0] * (Z - 4)
    for n in range(sx * sy):
        o = 2048 + n * 2048
        for k in range(Z):
            if any(data[o + k * 16 + q] != layers[k] for q in range(16)):
                check(False, 'flat column %d layer %d' % (n, k))
                break
# bedrock: full bottom layer; above it only up to height 4, thinning out like
# Minecraft Beta (chance (5-y)/5 at height y); superflat has the bottom layer only
layer = [0] * 6
for n in range(sx * sy):
    o = 2048 + n * 2048
    for k in range(6):
        layer[k] += sum(1 for q in range(16) if data[o + k * 16 + q] == 5)
cells = sx * sy * 16
check(layer[0] == cells, 'bottom bedrock layer incomplete')
check(layer[5] == 0, 'bedrock above height 4')
check(hist[5] == sum(layer), 'bedrock above height 5')
if flat:
    check(sum(layer[1:]) == 0, 'superflat has extra bedrock')
else:
    for k in range(1, 5):
        share = layer[k] / cells
        check(abs(share - (5 - k) / 5) < 0.05, 'bedrock at height %d: %.2f' % (k, share))
if not flat:
    # coal ore is a small share of the stone, like Minecraft Beta's
    coal_share = hist[61] / float(hist[3] + hist[61])
    check(0.002 < coal_share < 0.02, 'coal share %.4f' % coal_share)
    print('coal ore: %.2f%% of the stone' % (100 * coal_share))
    # iron ore: rarer than coal and only deep down (veins start below 32)
    iron_share = hist[IRON] / float(hist[3] + hist[61] + hist[IRON])
    check(0.001 < iron_share < coal_share, 'iron share %.4f' % iron_share)
    top_iron = 0
    for n in range(sx * sy):
        o = 2048 + n * 2048
        for k in range(Z - 1, -1, -1):
            if any(data[o + k * 16 + q] == IRON for q in range(16)):
                top_iron = max(top_iron, k)
                break
    check(top_iron < 36, 'iron ore too high: %d' % top_iron)
    print('iron ore: %.2f%% of the stone, up to height %d' % (100 * iron_share, top_iron))
print('bedrock per height: ' + ', '.join('%d=%.0f%%' % (k, 100.0 * layer[k] / cells) for k in range(6)))

# spawn stands on grass, three blocks above the surface
top = max(k for k in range(Z) if blk(spx, spy, k) and not see_through(blk(spx, spy, k)))
check(blk(spx, spy, top) == 1, 'spawn not on grass (%d)' % blk(spx, spy, top))
check(spz == (top + 3 - Z // 2) * 4096, 'spawn height')
check(all(blk(spx, spy, k) == 0 for k in range(top + 1, Z)), 'spawn column not clear above the ground')

total = sum(hist.values())
print('blocks: ' + ', '.join('%d=%.1f%%' % (k, 100.0 * v / total) for k, v in sorted(hist.items())))
print('checked %d columns' % len(cols))
if errors:
    print('%d ERRORS, first: %s' % (len(errors), errors[:5]))
    sys.exit(1)
print('map OK')

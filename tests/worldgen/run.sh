#!/bin/sh
# Host-side test: generate a world with the game's generator and validate it
# against the game's own face/light/occlusion rules (needs gcc and python).
set -e
cd "$(dirname "$0")"
out=$(mktemp -d)
gcc -std=gnu99 -O2 -Wall -I. -I../../include -o "$out/test_worldgen" test_worldgen.c ../../source/game/worldgen.c -lm
for seed in 12345 777 4242; do
  map=$("$out/test_worldgen" "$out" $seed)
  python check_map.py "$map" 40
  rm -f "$map"
done
# superflat world with a name; the same name again gets a number
map=$("$out/test_worldgen" "$out" 5 flat "My Flat")
case "$map" in */"My Flat.map") ;; *) echo "FAIL name: $map"; exit 1;; esac
python check_map.py "$map" 40 flat
map2=$("$out/test_worldgen" "$out" 6 flat "My Flat")
case "$map2" in */"My Flat 2.map") echo "name collision OK";; *) echo "FAIL second name: $map2"; exit 1;; esac
rm -f "$map" "$map2"
# seeds: parsing, same seed gives the same world, another seed another world
"$out/test_worldgen" --seeds
mkdir -p "$out/a" "$out/b"
m1=$("$out/test_worldgen" "$out/a" 777)
m2=$("$out/test_worldgen" "$out/b" 777)
cmp -s "$m1" "$m2" && echo "same seed, same world OK" || { echo "FAIL same seed differs"; exit 1; }
m3=$("$out/test_worldgen" "$out/b" 778)
cmp -s "$m1" "$m3" && { echo "FAIL different seeds, same world"; exit 1; } || echo "different seed, different world OK"
rm -rf "$out/a" "$out/b"
# the header hook (survivalFormatHeader in the game) runs with the spawn point set
map=$("$out/test_worldgen" "$out" 99 mark)
python -c "import struct,sys; d=open(sys.argv[1],'rb').read(); x=struct.unpack('<H',d[8:10])[0]; sys.exit(0 if d[600]==(x^0x5A)&255 else 1)" "$map" && echo "header hook OK"
rm -f "$map"
rm -rf "$out"

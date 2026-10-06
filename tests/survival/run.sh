#!/bin/sh
# Host-side tests for the survival rules (needs a native gcc, not devkitARM).
cd "$(dirname "$0")" && gcc -std=gnu99 -Wall -I. -I../../include -o test_survival test_survival.c ../../source/game/survival.c ../../source/game/drops.c ../../source/game/inventory.c ../../source/game/creative.c ../../source/game/chest.c ../../source/game/furnace.c ../../source/game/plants.c ../../source/game/farm.c ../../source/game/farmart.c && ./test_survival

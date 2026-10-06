/* Host-side stand-in for the game headers, just enough to compile worldgen.c. */
#ifndef TEST_GAMEMAIN
#define TEST_GAMEMAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int16_t s16; typedef int32_t int32;
#define WATERTYPE 239
#define LADDERTYPE 40
#define DOORTYPE 44
#define VERSIONMAGIC 150325785
#define SCALEFACTOR 32
#define tilesize 1
#define tilesize2 (tilesize*2)
#define rTilesize2 ((tilesize2*SCALEFACTOR)<<6)
static inline bool isLadder(u8 t){ return t>=LADDERTYPE && t<LADDERTYPE+4; }
static inline bool isDoor(u8 t){ return t>=DOORTYPE && t<DOORTYPE+16; }
#include "game/blockids.h"
typedef struct { u16 sizeX, sizeY; u32 magicVersionNumber; u16 spawnX, spawnY; int32 spawnZ; } header_struct;

#include "game/worldgen.h"
#endif

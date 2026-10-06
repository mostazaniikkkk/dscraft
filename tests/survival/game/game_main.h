/* Host-side stand-in for the game headers, just enough to compile survival.c and crafting.c. */
#ifndef TEST_GAMEMAIN
#define TEST_GAMEMAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8; typedef int16_t s16; typedef int32_t int32;
#define BIT(n) (1u<<(n))
#define RGB15(r,g,b) ((r)|((g)<<5)|((b)<<10))

extern u16 SPRITE_GFX_SUB[65536];
typedef struct { u16 attribute[4]; } oamEntry;
typedef struct { oamEntry oamMemory[128]; } OamState;
extern OamState oamSub;
#define ATTR0_DISABLED (1<<9)
#define ATTR0_BMP (3<<10)
#define ATTR0_SQUARE 0
#define ATTR1_SIZE_16 (1<<14)
#define ATTR2_ALPHA(a) ((a)<<12)
#define ATTR2_PRIORITY(p) ((p)<<10)
extern int brightness;
static inline void setBrightness(int screen, int level){ (void)screen; brightness=level; }

#define KEY_TOUCH (1<<12)
#define KEY_L (1<<9)
#define KEY_R (1<<8)
typedef struct { u16 px, py; } touchPosition;
extern touchPosition thisXY, lastXY;
extern u32 keysDownValue, keysHeldValue, keysUpValue;
static inline u32 keysDown(void){ return keysDownValue; }
static inline u32 keysHeld(void){ return keysHeldValue; }
static inline u32 keysUp(void){ return keysUpValue; }
#define ATTR0_COLOR_256 (1<<13)
#define ATTR1_SIZE_32 (2<<14)
static inline int32 sinLerp(int a){ (void)a; return 0; }
static inline int32 cosLerp(int a){ (void)a; return 4096; }
#define BLOCKS 46

#define WATERTYPE 239
#define LADDERTYPE 40
#define DOORTYPE 44
#define SCALEFACTOR 32
#define tilesize 1
#define tilesize2 (tilesize*2)
#define rTilesize2 ((tilesize2*SCALEFACTOR)<<6)
static inline bool isLadder(u8 t){ return t>=LADDERTYPE && t<LADDERTYPE+4; }
static inline bool isDoor(u8 t){ return t>=DOORTYPE && t<DOORTYPE+16; }
#include "game/blockids.h"

typedef struct { int32 x, y, z; } vect3D;
typedef struct { u16 sizeX, sizeY; u32 magicVersionNumber; u16 spawnX, spawnY; int32 spawnZ; } header_struct;
typedef struct { header_struct* header; vect3D offset, size; } map_struct;
#define CLUSTERSIZE 4
#define SUPERCLUSTERSIZE 32
typedef struct { vect3D position, vector; s16 angleZ; u8 inWater, onLadder; } player_struct;
extern player_struct Player;
extern map_struct map;
extern bool noclip;
extern u8 cursorBlock;
extern u32 testCursor, testCursorI, testCursorJ, testCursorK;
extern u8 worldBlock;               /* the block under the crosshair */
static inline u8* getBlockP(map_struct* m, int i, int j, int k){ (void)m; (void)i; (void)j; (void)k; return &worldBlock; }
/* terrain seen by dropped items: provided by the test */
u8* getBlockPE(map_struct* m, int i, int j, int k);
static inline bool solid(u8 t){ return t && t<WATERTYPE && t!=13 && !isPlant(t) && !isLadder(t) && !isDoor(t); }
extern char packPath[255];

#define MAXITEMS 64
#define MAXSLOTS 64
typedef struct { vect3D position; u8 id, type; s8 slot; bool used; } item_struct;
typedef struct { vect3D position; s8 id; bool used; } slot_struct;
extern item_struct items[MAXITEMS];
extern slot_struct slots[MAXSLOTS];

typedef struct { int dummy; } DS_state;
extern DS_state Game_State;
extern int stateChanges, saves;
static inline void DS_ChangeState(DS_state* s){ (void)s; stateChanges++; }
void globalSaveMap(map_struct* m);

/* lodepng: the test pack has no items.png, so the drawn fallback icons are used */
typedef struct { unsigned error; struct { unsigned width, height; } infoPng; } LodePNG_Decoder;
static inline unsigned LodePNG_loadFile(unsigned char** out, size_t* size, const char* f){ (void)f; *out=NULL; *size=0; return 78; }
static inline void LodePNG_Decoder_init(LodePNG_Decoder* d){ d->error=0; }
static inline void LodePNG_Decoder_decode(LodePNG_Decoder* d, unsigned char** o, size_t* s, const unsigned char* i, size_t n){ (void)d;(void)o;(void)s;(void)i;(void)n; }
static inline void LodePNG_Decoder_cleanup(LodePNG_Decoder* d){ (void)d; }

#include "game/survival.h"
#include "game/chest.h"
#include "game/furnace.h"
#include "game/plants.h"
#include "game/farm.h"
#include "game/drops.h"
#include "game/creative.h"
#endif

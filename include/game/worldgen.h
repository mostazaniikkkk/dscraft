#ifndef WORLDGEN_9
#define WORLDGEN_9

// New world generator. Writes a .map file in the game's on-disk format:
// a 2048-byte header, then one 2048-byte record per 4x4 column of blocks
// (1024 block ids + 1024 precalculated face/light/occlusion bytes).
// Each call does a small unit of work (one column) so the menu can spread it
// over frames and stay responsive.

#define WORLDGEN_COLUMNS 64        // 64x64 columns = 256x256 blocks, 64 high
#define WORLDGEN_SEA 28
#define WORLDGEN_FLAT_TOP 3        // superflat: bedrock, 2 dirt, grass (Minecraft's classic flat)

// free file for a world called `name` in dir: "name.map", else "name 2.map", "name 3.map"...
bool worldgenNewPath(const char* dir, const char* name, char* out, int size);
// seed typed by the player: a number is used as it is, other text by its hash
// (Java String.hashCode, as Minecraft); false for empty text (random seed)
bool worldgenSeedFromText(const char* text, u32* seed);
// `decorate` (may be NULL) can add to the header once the spawn point is set,
// e.g. survivalFormatHeader for a survival world.
bool worldgenStart(const char* path, u32 seed, bool flat, void (*decorate)(u8* header));
int worldgenStep(void);            // progress 0..99 while working, 100 when done, -1 on error
void worldgenCancel(void);

#endif

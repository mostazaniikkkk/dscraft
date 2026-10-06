#ifndef FARM_9
#define FARM_9

// Farming, as in Minecraft 1.4 (where carrots come from; this game's base,
// Beta 1.7, has no carrots or pumpkin stems):
// - a hoe turns grass or dirt with air above into farmland;
// - farmland is wet while water is within 4 blocks (same level or one up);
//   away from water it dries out over a few random ticks and, with nothing
//   planted on it, turns back into dirt; a fall onto it can trample it;
// - carrots and pumpkin seeds are planted on top of farmland and grow in 8
//   stages on random ticks with light 9 or more above, faster on wet
//   farmland and slower when the same crop is planted all around;
// - a grown pumpkin stem grows a pumpkin on a free side (on farmland, dirt or
//   grass) and bends towards it;
// - a carrot is also food: it heals 1.5 hearts (its 3 hunger points of 1.4).
// Block ids are in blockids.h.

#define FARMLAND_WET_TILE 70       // terrain.png 6,5
#define FARMLAND_DRY_TILE 71       // terrain.png 7,5
#define PUMPKIN_TOP_TILE 72        // terrain.png 6,6
#define PUMPKIN_SIDE_TILE 73       // terrain.png 6,7
#define PUMPKIN_FACE_TILE 74       // terrain.png 7,7
#define CARROT_TILE 75             // 75..78: the four looks of the crop (farmart.c)
#define STEM_TILE 79               // 79..86: stem by stage, tinted
#define STEM_BENT_TILE 87          // 87: bent stem, 88: the same mirrored
#define CARROT_ITEM_TILE 89
#define SEEDS_TILE 90
#define WOOD_HOE_TILE 91           // items.png 0,8
#define STONE_HOE_TILE 92          // items.png 1,8
#define FARM_ART_FIRST CARROT_TILE // tiles drawn by farmart.c
#define FARM_ART_LAST SEEDS_TILE

#define CARROT_HEAL 3              // half-hearts
#define HOE_DURABILITY_WOOD 60     // uses, as the other wooden and stone tools
#define HOE_DURABILITY_STONE 132
#define HOE_DURABILITY_IRON 251

// direction of an attached stem's plane (engine quad directions 14..17)
#define STEM_PLANE_Y 14            // plane along j (pumpkin at +j or -j), 15: other side
#define STEM_PLANE_X 16            // plane along i, 17: other side

// pure rules (tested on the host); the world view is the one of plants.h
static inline int farmMoisture(u8 t){ return t-FARMLAND_FIRST; }
static inline int cropStage(u8 t){ return isCarrotCrop(t)?t-CARROT_FIRST:(isAttachedStem(t)?7:t-STEM_FIRST); }
u8 farmTexture(u8 block, u8 direction);                    // atlas tile of a face of a farm block
bool farmWaterNear(const plantWorld_struct* w, int i, int j, int k);
float farmGrowthChance(const plantWorld_struct* w, int i, int j, int k);   // BlockCrops.getGrowthChance
bool farmPlantStays(const plantWorld_struct* w, int i, int j, int k);      // farmland below, light 8 or sky
int farmCropDrops(u8 block, u8* items, int max);           // what a broken crop or stem drops
int farmStemFruitSide(const plantWorld_struct* w, int i, int j, int k);   // -1 or the side (+x -x +y -y) of an adjacent pumpkin
void farmArtTile(int tile, u16* out);                      // farmart.c: 16x16 tiles FARM_ART_FIRST..LAST

// world hooks (farmworld.c)
int farmUseItem(map_struct* m, u8 item, int i, int j, int k, int face);    // hoe or planting on the block pointed at: 0 no, 1 used, 2 planted (uses up one)
void farmTick(map_struct* m, int i, int j, int k);         // a random tick on a farmland, crop or stem
void farmBlockChanged(map_struct* m, int i, int j, int k); // a block was placed or removed at i,j,k
void farmLanded(map_struct* m, player_struct* p, int32 fall);   // the player landed after a fall (1/4096 block)

#endif

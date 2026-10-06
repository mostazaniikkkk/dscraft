#ifndef PLANTS_9
#define PLANTS_9

// Leaves, saplings and apples, as in Minecraft.
//
// Leaves broken by hand drop a sapling one time in 20 (Beta) and an apple one
// time in 200 (Minecraft 1.0's oak leaves), never themselves. When a log or a
// leaf is removed, natural leaves around it are marked; a marked leaf with no
// log within 4 steps through leaves decays on a later random tick and drops the
// same way. Leaves placed by the player never decay (modern Minecraft).
// A sapling planted on grass or dirt grows in two random-tick stages while the
// light above it is 9 or more, into an oak tree (a big oak one time in 10).
// Without sky or light 8 it pops off. Apples heal 2 hearts (Beta food).
//
// State lives in block ids (Minecraft keeps it in block metadata), so it is
// saved with the map and found again when a part of the map is loaded.

// block ids LEAVES_BLOCK .. ITEM_APPLE, isLeaves() and isSapling() are in map.h

#define SAPLING_TILE 68            // terrain.png 15,0
#define APPLE_TILE 69              // items.png 10,0
#define APPLE_HEAL 4               // half-hearts

// A view of the world for the rules below: the block at i,j,k (k up), or -1
// where it is not known (outside the world or not loaded).
typedef struct
{
	int (*get)(void* ctx, int i, int j, int k);
	void* ctx;
	int height;                    // blocks in k
}plantWorld_struct;

// pure rules (tested on the host)
bool plantsLeafSupported(const plantWorld_struct* w, int i, int j, int k);   // a log within 4 steps through leaves
int plantsSkyDarkness(int sunX);                                            // Minecraft's skylightSubtracted (0 day .. 11 night)
int plantsLight(const plantWorld_struct* w, int i, int j, int k, int darkness);
bool plantsSeesSky(const plantWorld_struct* w, int i, int j, int k);
bool plantsSaplingStays(const plantWorld_struct* w, int i, int j, int k);
int plantsLeafDrops(u8* items);                                             // 0..2 items for a broken leaf

// Trees, written into a box around the base (Minecraft's WorldGenTrees and
// WorldGenBigTree); false when there is no room.
#define TREE_BOX_R 12
#define TREE_BOX_H 25              // from one below the base
#define TREE_BOX_SIZE ((2*TREE_BOX_R+1)*(2*TREE_BOX_R+1)*TREE_BOX_H)
typedef struct
{
	int i, j, k;                   // base (where the sapling was)
	u8 cells[TREE_BOX_SIZE];       // block to set, 0: none
}treeBox_struct;
bool plantsGrowSmallTree(const plantWorld_struct* w, u32 seed, int i, int j, int k, treeBox_struct* out);
bool plantsGrowBigTree(const plantWorld_struct* w, u32 seed, int i, int j, int k, treeBox_struct* out);
static inline u8* treeBoxCell(treeBox_struct* b, int i, int j, int k)
{
	int x=i-b->i+TREE_BOX_R, y=j-b->j+TREE_BOX_R, z=k-b->k+1;
	if(x<0 || y<0 || z<0 || x>2*TREE_BOX_R || y>2*TREE_BOX_R || z>=TREE_BOX_H)return 0;
	return &b->cells[x+y*(2*TREE_BOX_R+1)+z*(2*TREE_BOX_R+1)*(2*TREE_BOX_R+1)];
}

// world hooks (plantsworld.c)
void plantsClear(void);
void plantsTrack(int i, int j, int k);                    // a sapling or marked leaf was loaded or set
void plantsUpdate(map_struct* m);                         // every 30 Hz game tick
void plantsBlockRemoved(map_struct* m, int i, int j, int k, u8 old);
void plantsFlush(map_struct* m);                          // finish growing trees (before saving)
void plantsLeafDropsAt(int i, int j, int k);              // survival: what a broken leaf drops

#endif

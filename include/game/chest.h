#ifndef CHEST_9
#define CHEST_9

// Chests, as in Minecraft: 27 slots, two chests side by side make a double
// chest (54 slots), opened with the place button, contents dropped when broken.
//
// Each placed chest is a block id that encodes its facing and, for a double
// chest, on which side its other half is (like doors encode theirs), so its
// faces never depend on neighbouring columns that may not be loaded yet.
// The contents are kept per world in "<world>.chests", next to the .map file,
// and written whenever the world is saved.

#define ITEM_CHEST 62
#define CHEST_STATE_FIRST 72       // placed chests: 72 + facing*3 + partner
#define CHEST_STATES 12
#define CHEST_SLOTS 27

// facing (front face) and partner side
#define CHEST_FACE_PX 0            // front towards +x (face direction 2)
#define CHEST_FACE_NX 1            // -x (3)
#define CHEST_FACE_PY 2            // +y (4)
#define CHEST_FACE_NY 3            // -y (5)
#define CHEST_SINGLE 0
#define CHEST_PARTNER_POS 1        // the other half is on the + side of the axis across the front
#define CHEST_PARTNER_NEG 2

// atlas tiles (terrain.png: top 9,1  side 10,1  front 11,1  double front 9,2 10,2  double back 9,3 10,3)
#define CHEST_TILE_TOP 56
#define CHEST_TILE_SIDE 57
#define CHEST_TILE_FRONT 58
#define CHEST_TILE_FRONT_L 59
#define CHEST_TILE_FRONT_R 60
#define CHEST_TILE_BACK_L 61
#define CHEST_TILE_BACK_R 62

typedef struct chest_s
{
	u16 i, j;
	u8 k;
	u8 used;
	stack_struct slots[CHEST_SLOTS];
}chest_struct;

static inline bool isChestBlock(u8 t){ return t>=CHEST_STATE_FIRST && t<CHEST_STATE_FIRST+CHEST_STATES; }
static inline u8 chestState(int facing, int partner){ return CHEST_STATE_FIRST+facing*3+partner; }
static inline int chestFacing(u8 t){ return (t-CHEST_STATE_FIRST)/3; }
static inline int chestPartner(u8 t){ return (t-CHEST_STATE_FIRST)%3; }

// pure rules (tested on the host)
u8 chestTexture(u8 state, u8 direction);
int chestFacingFromLook(s16 angleZ);                        // front towards the player
int chestNeighbourOffset(int side, int* di, int* dj);      // side 0..3: +x -x +y -y
// placement next to the four horizontal neighbours (+x -x +y -y); false: not allowed
bool chestPlan(const u8 neighbours[4], int lookFacing, u8* newState, int* pairSide, u8* neighbourState);

// contents
void chestsClear(void);
bool chestsLoad(const char* mapPath);
bool chestsSave(const char* mapPath);
void chestSidecarPath(const char* mapPath, char* out, int size);
chest_struct* chestAt(int i, int j, int k, bool create);
void chestRemove(int i, int j, int k);
int chestCount(void);

// game hooks (map.c / controls.c)
bool chestPlace(map_struct* m, int i, int j, int k);       // place an ITEM_CHEST at i,j,k
bool chestOpen(map_struct* m, int i, int j, int k);        // survival: open the chest screen
void interfaceOpenChest(chest_struct* a, chest_struct* b);  // interface.c: the chest screen
void chestBroken(map_struct* m, int i, int j, int k, u8 state);

#endif

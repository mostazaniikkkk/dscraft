#ifndef DROPS_9
#define DROPS_9

// Items dropped on the ground when a block is mined in survival.
// At most DROPS_MAX exist at once: a new drop beyond that removes the oldest one.
// Positions are global block coordinates in fixed point, 4096 units per block
// (the same scale as player positions); a block's centre is at index*4096.

#define DROPS_MAX 5
#define DROP_UNIT 4096
#define DROP_HALF 512              // drawn as a cube a quarter of a block wide
#define DROP_PICKUP_DELAY 10       // ticks before a mined drop can be picked up
#define DROP_THROW_DELAY 40        // 2 s for items thrown by the player (as in Minecraft)
#define DROP_LIFETIME (30*60*5)    // 5 minutes at 30 Hz

typedef struct
{
	int32 x, y, z;
	int32 vx, vy, vz;
	u32 serial;                    // spawn order, used to find the oldest drop
	u16 age;
	u16 wear;                      // tools keep their wear on the ground
	u8 item, count;
	u8 delay;                      // pickup delay in ticks
	bool used;
}drop_struct;

void dropsClear(void);
void dropsSpawn(u8 item, int i, int j, int k);
void dropsSpawnStack(u8 item, int count, u16 wear, int i, int j, int k);   // e.g. a broken chest's contents
void dropsThrow(u8 item, int count, u16 wear);   // from the player, forwards
void dropsUpdate(map_struct* m, player_struct* p);
const drop_struct* dropsGet(int n);            // NULL when slot n is empty
bool dropsLoaded(map_struct* m, int i, int j, int k);
void drawDrops(map_struct* m);                 // map.c, inside drawTestMap's transform

#endif

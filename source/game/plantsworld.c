#include "game/game_main.h"

// Saplings and leaves in the world: random ticks for the loaded ones, leaf
// decay, growing trees. The rules are in plants.c.
//
// Minecraft gives each loaded chunk (16x16x128 blocks) 80 random ticks per
// game tick, so a block is ticked with a chance of 80/32768 per tick. Only
// saplings and marked leaves react to them, so only those are kept here.

#define RANDOM_TICK_CHANCE 80      // out of 32768
#define TREE_WRITES_PER_FRAME 24   // a grown tree appears over a few frames

typedef struct
{
	u16 i, j;
	u8 k;
}plantPos_struct;

static plantPos_struct* tracked;
static int trackedCount, trackedCap;

typedef struct
{
	u16 i, j;
	u8 k, type;
}treeWrite_struct;

static treeWrite_struct* writes;
static int writeFirst, writeCount, writeCap;

/* ---------------------------------------------------------------------------
 * The world as the rules see it
 * ------------------------------------------------------------------------- */

// loaded, with a margin: changing a block also rebuilds its neighbours
static bool usable(map_struct* m, int i, int j, int k)
{
	return i>=(m->offset.x+2)*CLUSTERSIZE && i<(m->offset.x+SUPERCLUSTERSIZE-2)*CLUSTERSIZE
		&& j>=(m->offset.y+2)*CLUSTERSIZE && j<(m->offset.y+SUPERCLUSTERSIZE-2)*CLUSTERSIZE
		&& i<m->size.x && j<m->size.y && k>=0 && k<m->size.z;
}

static int mapGet(void* ctx, int i, int j, int k)
{
	map_struct* m=ctx;
	if(i<0 || j<0 || i>=m->size.x || j>=m->size.y || !dropsLoaded(m,i,j,k))return -1;
	return *getBlockP(m,i,j,k);
}

static plantWorld_struct view(map_struct* m)
{
	plantWorld_struct w={mapGet,m,m->size.z};
	return w;
}

/* ---------------------------------------------------------------------------
 * Tracked blocks
 * ------------------------------------------------------------------------- */

void plantsClear(void)
{
	if(tracked)free(tracked);
	if(writes)free(writes);
	tracked=NULL;
	writes=NULL;
	trackedCount=trackedCap=0;
	writeFirst=writeCount=writeCap=0;
}

void plantsTrack(int i, int j, int k)
{
	int n;
	for(n=0;n<trackedCount;n++)if(tracked[n].i==i && tracked[n].j==j && tracked[n].k==k)return;
	if(trackedCount==trackedCap)
	{
		int cap=trackedCap?trackedCap*2:32;
		plantPos_struct* t=realloc(tracked,cap*sizeof(plantPos_struct));
		if(!t)return;
		tracked=t;
		trackedCap=cap;
	}
	tracked[trackedCount].i=i;
	tracked[trackedCount].j=j;
	tracked[trackedCount].k=k;
	trackedCount++;
}

static void untrack(int n)
{
	tracked[n]=tracked[--trackedCount];
}

/* ---------------------------------------------------------------------------
 * Changes
 * ------------------------------------------------------------------------- */

// set a block id that looks the same (a leaf marked or unmarked, a sapling's stage)
static void setQuiet(map_struct* m, int i, int j, int k, u8 type)
{
	vect3D c=getCluster(m,i,j,k);
	*getBlockP(m,i,j,k)=type;
	m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->changed=1;
}

static void markLeaves(map_struct* m, int i, int j, int k, int r)
{
	int di, dj, dk;
	for(dk=-r;dk<=r;dk++)for(dj=-r;dj<=r;dj++)for(di=-r;di<=r;di++)
	{
		int x=i+di, y=j+dj, z=k+dk;
		if(!usable(m,x,y,z) || *getBlockP(m,x,y,z)!=LEAVES_BLOCK)continue;
		setQuiet(m,x,y,z,LEAVES_DECAY);
		plantsTrack(x,y,z);
	}
}

static void breakSapling(map_struct* m, int i, int j, int k)
{
	changeBlock(m,i,j,k,0);
	if(survivalEnabled())dropsSpawn(ITEM_SAPLING,i,j,k);
}

// Something was removed at i,j,k (mined, or a leaf decayed). Minecraft:
// BlockLog / BlockLeaves.onBlockRemoval mark the leaves around (radius 4 for
// a log, 1 for a leaf); a sapling on the block that went pops off.
void plantsBlockRemoved(map_struct* m, int i, int j, int k, u8 old)
{
	if(itemIsLog(old))markLeaves(m,i,j,k,4);
	else if(isLeaves(old))markLeaves(m,i,j,k,1);
	if(usable(m,i,j,k+1) && isSapling(*getBlockP(m,i,j,k+1)))breakSapling(m,i,j,k+1);
}

static void decayCheck(map_struct* m, int i, int j, int k)
{
	plantWorld_struct w=view(m);
	if(!usable(m,i-5,j-5,k) || !usable(m,i+5,j+5,k))return;     // Minecraft: chunks around must exist
	if(plantsLeafSupported(&w,i,j,k)){setQuiet(m,i,j,k,LEAVES_BLOCK);return;}
	changeBlock(m,i,j,k,0);
	if(survivalEnabled())plantsLeafDropsAt(i,j,k);
	plantsBlockRemoved(m,i,j,k,LEAVES_DECAY);
	farmBlockChanged(m,i,j,k);
}

/* ---------------------------------------------------------------------------
 * Trees
 * ------------------------------------------------------------------------- */

static treeBox_struct box;

static void queueTree(treeBox_struct* b)
{
	int n, x, y, z;
	for(n=0;n<TREE_BOX_SIZE;n++)
	{
		if(!b->cells[n])continue;
		if(writeFirst+writeCount==writeCap)
		{
			if(writeFirst){memmove(writes,writes+writeFirst,writeCount*sizeof(treeWrite_struct));writeFirst=0;}
			if(writeCount==writeCap)
			{
				int cap=writeCap?writeCap*2:256;
				treeWrite_struct* t=realloc(writes,cap*sizeof(treeWrite_struct));
				if(!t)return;
				writes=t;
				writeCap=cap;
			}
		}
		x=n%(2*TREE_BOX_R+1);
		y=(n/(2*TREE_BOX_R+1))%(2*TREE_BOX_R+1);
		z=n/((2*TREE_BOX_R+1)*(2*TREE_BOX_R+1));
		treeWrite_struct* w=&writes[writeFirst+writeCount++];
		w->i=b->i+x-TREE_BOX_R;
		w->j=b->j+y-TREE_BOX_R;
		w->k=b->k+z-1;
		w->type=b->cells[n];
	}
}

// Logs and leaves only go where there is air, leaves or the sapling (as the
// generators checked); dirt only replaces the grass under the trunk.
static void applyWrite(map_struct* m, const treeWrite_struct* w)
{
	u8 cur;
	if(!usable(m,w->i,w->j,w->k))return;
	cur=*getBlockP(m,w->i,w->j,w->k);
	if(cur==w->type)return;
	if(w->type==2)
	{
		if(cur!=1)return;
		vect3D c=getCluster(m,w->i,w->j,w->k);
		removeBlock(m,w->i,w->j,w->k,false);
		*getBlockP(m,w->i,w->j,w->k)=2;
		addBlock(m,w->i,w->j,w->k);
		m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->changed=1;
		return;
	}
	if(cur && !isLeaves(cur) && !isSapling(cur))return;
	if(w->type==LEAVES_BLOCK && cur)return;               // leaves never replace anything
	if(cur)changeBlock(m,w->i,w->j,w->k,0);
	changeBlock(m,w->i,w->j,w->k,w->type);
	farmBlockChanged(m,w->i,w->j,w->k);
}

static void applyWrites(map_struct* m, int budget)
{
	while(writeCount && budget--)
	{
		applyWrite(m,&writes[writeFirst]);
		writeFirst++;
		writeCount--;
	}
	if(!writeCount)writeFirst=0;
}

void plantsFlush(map_struct* m)
{
	applyWrites(m,writeCount);
}

static void growTree(map_struct* m, int i, int j, int k)
{
	plantWorld_struct w=view(m);
	u32 seed=((u32)rand()<<16)^(u32)rand();
	bool ok;
	// BlockSapling.growTree: a big oak one time in 10
	if(!(rand()%10))ok=plantsGrowBigTree(&w,seed,i,j,k,&box);
	else ok=plantsGrowSmallTree(&w,seed,i,j,k,&box);
	if(ok)queueTree(&box);
}

static void saplingTick(map_struct* m, int i, int j, int k)
{
	plantWorld_struct w=view(m);
	u8 t=*getBlockP(m,i,j,k);
	if(!plantsSaplingStays(&w,i,j,k)){breakSapling(m,i,j,k);return;}
	if(plantsLight(&w,i,j,k+1,plantsSkyDarkness(sunX))<9 || rand()%30)return;
	if(t==ITEM_SAPLING)setQuiet(m,i,j,k,SAPLING_GROWN);
	else growTree(m,i,j,k);
}

/* ---------------------------------------------------------------------------
 * Update
 * ------------------------------------------------------------------------- */

static void randomTicks(map_struct* m)
{
	int n;
	for(n=0;n<trackedCount;n++)
	{
		plantPos_struct p=tracked[n];
		u8 t;
		if(!usable(m,p.i,p.j,p.k))continue;
		t=*getBlockP(m,p.i,p.j,p.k);
		if(t!=LEAVES_DECAY && !isPlant(t) && !isFarmland(t)){untrack(n--);continue;}
		if(rand()%32768>=RANDOM_TICK_CHANCE)continue;
		if(t==LEAVES_DECAY)decayCheck(m,p.i,p.j,p.k);
		else if(isSapling(t))saplingTick(m,p.i,p.j,p.k);
		else farmTick(m,p.i,p.j,p.k);
	}
}

void plantsUpdate(map_struct* m)
{
	static int acc;
	acc+=2;                                   // 30 Hz here, 20 ticks per second in Minecraft
	while(acc>=3)
	{
		acc-=3;
		randomTicks(m);
	}
	applyWrites(m,TREE_WRITES_PER_FRAME);
}

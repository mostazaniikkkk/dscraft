#include "game/game_main.h"

#include <maxmod9.h>
#include "soundbank.h"

// Farming in the world: the hoe, planting, random ticks of farmland, crops
// and stems (handed out by plantsworld.c), pumpkins growing next to stems and
// farmland trampled by a fall. The rules are in farm.c.

static bool usable(map_struct* m, int i, int j, int k)
{
	return i>=(m->offset.x+2)*CLUSTERSIZE && i<(m->offset.x+SUPERCLUSTERSIZE-2)*CLUSTERSIZE
		&& j>=(m->offset.y+2)*CLUSTERSIZE && j<(m->offset.y+SUPERCLUSTERSIZE-2)*CLUSTERSIZE
		&& i>=0 && j>=0 && i<m->size.x && j<m->size.y && k>=0 && k<m->size.z;
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

// a solid block becomes another solid block that hides the same faces
static void replaceSolid(map_struct* m, int i, int j, int k, u8 type)
{
	vect3D c=getCluster(m,i,j,k);
	removeBlock(m,i,j,k,false);
	*getBlockP(m,i,j,k)=type;
	addBlock(m,i,j,k);
	m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->changed=1;
}

// farmland keeps its look from moisture 1 to 7: only the change to or from dry is redrawn
static void setFarmland(map_struct* m, int i, int j, int k, u8 type)
{
	u8 old=*getBlockP(m,i,j,k);
	if(old==type)return;
	if((farmMoisture(old)>0)!=(farmMoisture(type)>0))replaceSolid(m,i,j,k,type);
	else
	{
		vect3D c=getCluster(m,i,j,k);
		*getBlockP(m,i,j,k)=type;
		m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->changed=1;
	}
}

static void breakPlant(map_struct* m, int i, int j, int k)
{
	u8 t=*getBlockP(m,i,j,k), items[4];
	int n, c;
	changeBlock(m,i,j,k,0);
	if(!survivalEnabled())return;
	c=farmCropDrops(t,items,4);
	for(n=0;n<c;n++)dropsSpawn(items[n],i,j,k);
}

static void toDirt(map_struct* m, int i, int j, int k)
{
	replaceSolid(m,i,j,k,2);
	farmBlockChanged(m,i,j,k);
}

/* ---------------------------------------------------------------------------
 * Using items
 * ------------------------------------------------------------------------- */

// 0: not used; 1: used (the hoe wears); 2: planted (one item is used up)
int farmUseItem(map_struct* m, u8 item, int i, int j, int k, int face)
{
	u8 t;
	if(!usable(m,i,j,k))return 0;
	t=*getBlockP(m,i,j,k);
	if(isHoe(item))
	{
		// ItemHoe: grass or dirt with air above, not from below
		if(face==1 || (t!=1 && t!=2) || (k+1<m->size.z && *getBlockP(m,i,j,k+1)))return 0;
		mmEffect(SFX_STEP);
		replaceSolid(m,i,j,k,FARMLAND_FIRST);
		plantsTrack(i,j,k);
		survivalWearSelected();
		return 1;
	}
	if(item==ITEM_CARROT || item==ITEM_PUMPKIN_SEEDS)
	{
		// on the top of farmland, with air above
		if(face!=0 || !isFarmland(t) || k+1>=m->size.z || *getBlockP(m,i,j,k+1))return 0;
		mmEffect(SFX_ADD);
		changeBlock(m,i,j,k+1,item==ITEM_CARROT?CARROT_FIRST:STEM_FIRST);
		return 2;
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * Changes around
 * ------------------------------------------------------------------------- */

static void refreshStem(map_struct* m, int i, int j, int k)
{
	plantWorld_struct w=view(m);
	u8 t=*getBlockP(m,i,j,k);
	int side;
	if(!isStem(t) || cropStage(t)<7)return;
	side=farmStemFruitSide(&w,i,j,k);
	t=(side>=0)?STEM_ATTACHED+side:STEM_FIRST+7;
	if(*getBlockP(m,i,j,k)!=t)plantRefresh(m,i,j,k,t);
}

void farmBlockChanged(map_struct* m, int i, int j, int k)
{
	plantWorld_struct w=view(m);
	u8 t;
	if(!usable(m,i,j,k))return;
	t=*getBlockP(m,i,j,k);
	// BlockFarmland.onNeighborBlockChange: a solid block on top turns it to dirt
	if(k>0 && solid(t) && isFarmland(*getBlockP(m,i,j,k-1)))toDirt(m,i,j,k-1);
	// a crop or stem whose ground went pops off
	if(k+1<m->size.z)
	{
		u8 a=*getBlockP(m,i,j,k+1);
		if((isCarrotCrop(a) || isStem(a)) && !farmPlantStays(&w,i,j,k+1))breakPlant(m,i,j,k+1);
	}
	// grown stems bend towards a pumpkin next to them, or straighten
	if(usable(m,i+1,j,k))refreshStem(m,i+1,j,k);
	if(usable(m,i-1,j,k))refreshStem(m,i-1,j,k);
	if(usable(m,i,j+1,k))refreshStem(m,i,j+1,k);
	if(usable(m,i,j-1,k))refreshStem(m,i,j-1,k);
}

/* ---------------------------------------------------------------------------
 * Random ticks
 * ------------------------------------------------------------------------- */

static void farmlandTick(map_struct* m, int i, int j, int k, u8 t)
{
	plantWorld_struct w=view(m);
	u8 above=(k+1<m->size.z)?*getBlockP(m,i,j,k+1):0;
	// BlockFarmland.updateTick (no rain in this game)
	if(farmWaterNear(&w,i,j,k))setFarmland(m,i,j,k,FARMLAND_FIRST+7);
	else if(farmMoisture(t)>0)setFarmland(m,i,j,k,t-1);
	else if(!isCarrotCrop(above) && !isStem(above))toDirt(m,i,j,k);
}

static void growPumpkin(map_struct* m, int i, int j, int k)
{
	plantWorld_struct w=view(m);
	int x=i, y=j, side, r;
	u8 below;
	if(farmStemFruitSide(&w,i,j,k)>=0)return;          // one pumpkin at a time
	r=rand()%4;
	if(r==0){x--;side=1;}
	else if(r==1){x++;side=0;}
	else if(r==2){y--;side=3;}
	else{y++;side=2;}
	if(!usable(m,x,y,k) || *getBlockP(m,x,y,k))return;
	below=*getBlockP(m,x,y,k-1);
	if(!isFarmland(below) && below!=1 && below!=2)return;
	changeBlock(m,x,y,k,PUMPKIN_FIRST+2);                // Minecraft's metadata 0
	if(!isPumpkin(*getBlockP(m,x,y,k)))return;           // refused (the player stands there)
	plantRefresh(m,i,j,k,STEM_ATTACHED+side);
	farmBlockChanged(m,x,y,k);
}

static void plantTick(map_struct* m, int i, int j, int k, u8 t)
{
	plantWorld_struct w=view(m);
	float f;
	if(!farmPlantStays(&w,i,j,k)){breakPlant(m,i,j,k);return;}
	if(plantsLight(&w,i,j,k+1,plantsSkyDarkness(sunX))<9)return;
	f=farmGrowthChance(&w,i,j,k);
	if(rand()%((int)(25.0f/f)+1))return;
	if(cropStage(t)<7)plantRefresh(m,i,j,k,t+1);
	else if(isStem(t))growPumpkin(m,i,j,k);
}

void farmTick(map_struct* m, int i, int j, int k)
{
	u8 t=*getBlockP(m,i,j,k);
	if(isFarmland(t))farmlandTick(m,i,j,k,t);
	else if(isCarrotCrop(t) || isStem(t))plantTick(m,i,j,k,t);
}

/* ---------------------------------------------------------------------------
 * Trampling
 * ------------------------------------------------------------------------- */

// BlockFarmland.onFallenUpon: trampled with a chance of (fall distance - 0.5)
void farmLanded(map_struct* m, player_struct* p, int32 fall)
{
	vect3D b=getPointBlockPos(m,p->position.x,p->position.y,p->position.z-(rTilesize2*5)/2-256);
	if(fall<=2048 || !usable(m,b.x,b.y,b.z) || !isFarmland(*getBlockP(m,b.x,b.y,b.z)))return;
	if(rand()%4096<fall-2048)toDirt(m,b.x,b.y,b.z);
}

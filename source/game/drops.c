#include "game/game_main.h"

#include <maxmod9.h>
#include "soundbank.h"

#define GRAVITY_DROP 45
#define POP_SPEED 380
#define SPREAD 60
#define MAX_FALL 1200
#define WATER_SINK 60
#define PICKUP_XY (DROP_UNIT*13/10)
#define PICKUP_Z (DROP_UNIT*5/2)
#define MERGE_DIST DROP_UNIT
#define THROW_SPEED 220

static drop_struct drops[DROPS_MAX];
static u32 nextSerial;

void dropsClear(void)
{
	memset(drops,0,sizeof(drops));
	nextSerial=0;
}

const drop_struct* dropsGet(int n)
{
	return (n>=0 && n<DROPS_MAX && drops[n].used)?&drops[n]:NULL;
}

static inline int toBlock(int32 v)
{
	return (v+DROP_UNIT/2)>>12;    // floor((v+2048)/4096); positions are never negative inside the map
}

bool dropsLoaded(map_struct* m, int i, int j, int k)
{
	return i>=m->offset.x*CLUSTERSIZE && i<(m->offset.x+SUPERCLUSTERSIZE)*CLUSTERSIZE
		&& j>=m->offset.y*CLUSTERSIZE && j<(m->offset.y+SUPERCLUSTERSIZE)*CLUSTERSIZE
		&& k>=0 && k<m->size.z;
}

static u8 blockAt(map_struct* m, int32 x, int32 y, int32 z)
{
	int i=toBlock(x), j=toBlock(y), k=toBlock(z);
	if(k<0)return 0;               // the void under the bedrock
	if(k>=m->size.z)return 0;
	if(!dropsLoaded(m,i,j,k))return 5;
	return *getBlockPE(m,i,j,k);
}

// A new drop: merges into a nearby stack of the same item when it can, otherwise
// takes a free slot, otherwise replaces the oldest drop.
static drop_struct* newDrop(u8 item, int count, u16 wear, int32 x, int32 y, int32 z)
{
	int n, slot=-1;
	if(!item || count<=0)return NULL;
	if(itemMaxStack(item)>1)for(n=0;n<DROPS_MAX;n++)
	{
		drop_struct* d=&drops[n];
		if(d->used && d->item==item && d->count+count<=itemMaxStack(item)
		&& abs(d->x-x)<=MERGE_DIST && abs(d->y-y)<=MERGE_DIST && abs(d->z-z)<=MERGE_DIST)
		{
			d->count+=count;
			return NULL;
		}
	}
	for(n=0;n<DROPS_MAX;n++)if(!drops[n].used){slot=n;break;}
	if(slot<0)
	{
		slot=0;
		for(n=1;n<DROPS_MAX;n++)if(drops[n].serial<drops[slot].serial)slot=n;
	}
	drop_struct* d=&drops[slot];
	d->x=x; d->y=y; d->z=z;
	d->item=item;
	d->count=count;
	d->wear=wear;
	d->age=0;
	d->delay=DROP_PICKUP_DELAY;
	d->serial=nextSerial++;
	d->used=true;
	return d;
}

static void pop(drop_struct* d)
{
	u32 r=rand();
	d->vx=(int32)(r%(2*SPREAD+1))-SPREAD;
	d->vy=(int32)((r>>10)%(2*SPREAD+1))-SPREAD;
	d->vz=POP_SPEED;
}

void dropsSpawn(u8 item, int i, int j, int k)
{
	drop_struct* d=newDrop(item,1,0,i*DROP_UNIT,j*DROP_UNIT,k*DROP_UNIT);
	if(d)pop(d);
}

void dropsSpawnStack(u8 item, int count, u16 wear, int i, int j, int k)
{
	drop_struct* d=newDrop(item,count,wear,i*DROP_UNIT,j*DROP_UNIT,k*DROP_UNIT);
	if(d)pop(d);
}

// player position in the drops' global coordinates
static void playerGlobal(map_struct* m, player_struct* p, int32* x, int32* y, int32* z)
{
	*x=p->position.x+(SUPERCLUSTERSIZE*CLUSTERSIZE/2+m->offset.x*CLUSTERSIZE)*DROP_UNIT;
	*y=p->position.y+(SUPERCLUSTERSIZE*CLUSTERSIZE/2+m->offset.y*CLUSTERSIZE)*DROP_UNIT;
	*z=p->position.z+(m->size.z/2+m->offset.z*CLUSTERSIZE)*DROP_UNIT;
}

void dropsThrow(u8 item, int count, u16 wear)
{
	int32 x, y, z;
	int32 fx=sinLerp(Player.angleZ), fy=cosLerp(Player.angleZ);   // forwards, 4096 = 1
	playerGlobal(&map,&Player,&x,&y,&z);
	while(count>0)
	{
		int n=(count<itemMaxStack(item))?count:itemMaxStack(item);
		// in front of the player at chest height, thrown forwards
		drop_struct* d=newDrop(item,n,wear,x+fx/2,y+fy/2,z-DROP_UNIT/4);
		if(d)
		{
			d->vx=fx*THROW_SPEED/4096;
			d->vy=fy*THROW_SPEED/4096;
			d->vz=POP_SPEED/2;
			d->delay=DROP_THROW_DELAY;
		}
		count-=n;
	}
}

static void moveDrop(map_struct* m, drop_struct* d)
{
	int32 nz;
	bool inWater=blockAt(m,d->x,d->y,d->z)>=WATERTYPE;

	// a block placed over the drop pushes it up
	if(solid(blockAt(m,d->x,d->y,d->z)))
	{
		d->z=toBlock(d->z)*DROP_UNIT+DROP_UNIT/2+DROP_HALF;
		d->vz=0;
		return;
	}

	d->vz-=GRAVITY_DROP;
	if(d->vz<-MAX_FALL)d->vz=-MAX_FALL;
	if(inWater)
	{
		if(d->vz<-WATER_SINK)d->vz=-WATER_SINK;
		d->vx=d->vx*7/8;
		d->vy=d->vy*7/8;
	}else{
		// air drag keeps the drop close to where the block was
		d->vx=d->vx*15/16;
		d->vy=d->vy*15/16;
	}

	if(d->vx)
	{
		int32 edge=d->x+d->vx+(d->vx>0?DROP_HALF:-DROP_HALF);
		if(solid(blockAt(m,edge,d->y,d->z)))d->vx=0;
		else d->x+=d->vx;
	}
	if(d->vy)
	{
		int32 edge=d->y+d->vy+(d->vy>0?DROP_HALF:-DROP_HALF);
		if(solid(blockAt(m,d->x,edge,d->z)))d->vy=0;
		else d->y+=d->vy;
	}

	nz=d->z+d->vz;
	if(d->vz<0)
	{
		int32 bottom=nz-DROP_HALF;
		if(solid(blockAt(m,d->x,d->y,bottom)))
		{
			// land on top of the block below
			d->z=toBlock(bottom)*DROP_UNIT+DROP_UNIT/2+DROP_HALF;
			d->vz=0;
			d->vx=d->vx*3/5;
			d->vy=d->vy*3/5;
			return;
		}
	}else if(d->vz>0 && solid(blockAt(m,d->x,d->y,nz+DROP_HALF)))
	{
		d->vz=0;
		return;
	}
	d->z=nz;
}

void dropsUpdate(map_struct* m, player_struct* p)
{
	int n;
	int32 px, py, pz;
	playerGlobal(m,p,&px,&py,&pz);

	for(n=0;n<DROPS_MAX;n++)
	{
		drop_struct* d=&drops[n];
		if(!d->used)continue;
		if(++d->age>=DROP_LIFETIME || d->z<-VOID_DEATH_DEPTH*DROP_UNIT){d->used=false;continue;}   // the void destroys items
		// drops in unloaded parts of the world wait until the player comes back
		if(!dropsLoaded(m,toBlock(d->x),toBlock(d->y),toBlock(d->z)<0?0:toBlock(d->z)))continue;
		moveDrop(m,d);

		if(d->age>=d->delay && abs(d->x-px)<PICKUP_XY && abs(d->y-py)<PICKUP_XY && abs(d->z-pz)<PICKUP_Z)
		{
			// whatever does not fit in the inventory stays on the ground
			int left=inventoryAdd(d->item,d->count,d->wear);
			if(left<d->count)
			{
				d->count=left;
				if(!left)d->used=false;
				mmEffect(SFX_ADD);
			}
		}
	}
}

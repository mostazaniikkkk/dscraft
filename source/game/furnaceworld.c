#include "game/game_main.h"

// Furnaces in the world: placing, opening, breaking, and running them while
// their area is loaded (as Minecraft runs the furnaces of loaded chunks).
// The rules themselves are in furnace.c.

static bool inWorld(map_struct* m, int i, int j, int k)
{
	return i>=0 && j>=0 && k>=0 && i<m->size.x && j<m->size.y && k<m->size.z;
}

// rebuild a block's faces after its id changed (unlit <-> lit)
static void refreshBlock(map_struct* m, int i, int j, int k)
{
	vect3D c=getCluster(m,i,j,k);
	removeBlock(m,i,j,k,false);
	addBlock(m,i,j,k);
	m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->changed=1;
}

bool furnacePlace(map_struct* m, int i, int j, int k)
{
	u8 state;
	if(!inWorld(m,i,j,k) || *getBlockP(m,i,j,k))return false;
	// the front faces the player, as in Minecraft
	state=furnaceState(chestFacingFromLook(Player.angleZ),false);
	changeBlock(m,i,j,k,state);
	if(*getBlockP(m,i,j,k)!=state)return false;      // refused (e.g. where the player stands)
	furnaceRemove(i,j,k);                              // forget any stale contents there
	furnaceAt(i,j,k,true);
	return true;
}

bool furnaceOpen(map_struct* m, int i, int j, int k)
{
	furnace_struct* f;
	if(!isFurnaceBlock(*getBlockP(m,i,j,k)))return false;
	// creative has no item stacks to put in: the furnace is only a block there
	if(!survivalEnabled())return true;
	f=furnaceAt(i,j,k,true);
	if(f)interfaceOpenFurnace(f);
	return true;
}

void furnaceBroken(map_struct* m, int i, int j, int k)
{
	int n;
	furnace_struct* f=furnaceAt(i,j,k,false);
	if(!f)return;
	// the contents fall out, as in Minecraft (in survival, where items drop)
	if(survivalEnabled())
		for(n=0;n<FURNACE_SLOTS;n++)
			if(f->slots[n].count && f->slots[n].item)
				dropsSpawnStack(f->slots[n].item,f->slots[n].count,f->slots[n].wear,i,j,k);
	furnaceRemove(i,j,k);
}

static void setLit(map_struct* m, furnace_struct* f, bool lit)
{
	u8* b=getBlockP(m,f->i,f->j,f->k);
	u8 state=furnaceState(furnaceFacing(*b),lit);
	if(*b==state)return;
	*b=state;
	refreshBlock(m,f->i,f->j,f->k);
	if(lit)lightSourceOn(m,f->i,f->j,f->k);
	else lightSourceOff(m,f->i,f->j,f->k,FURNACE_LIGHT);
}

// The game runs at 30 Hz and Minecraft at 20: two furnace ticks every three frames.
void furnacesUpdate(map_struct* m)
{
	static int acc;
	int n;
	acc+=2;
	while(acc>=3)
	{
		acc-=3;
		for(n=0;n<furnaceRecords();n++)
		{
			furnace_struct* f=furnaceGet(n);
			if(!f || !dropsLoaded(m,f->i,f->j,f->k))continue;
			u8 b=*getBlockP(m,f->i,f->j,f->k);
			if(!isFurnaceBlock(b)){furnaceRemove(f->i,f->j,f->k);continue;}
			furnaceTick(f);
			// the block follows the fire (also if the contents file was lost)
			if(isLitFurnace(b)!=furnaceBurning(f))setLit(m,f,furnaceBurning(f));
		}
	}
}

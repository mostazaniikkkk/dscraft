#include "game/game_main.h"

// Chests in the world: placing (and joining into double chests), opening and
// breaking. The rules themselves are in chest.c.

static bool inWorld(map_struct* m, int i, int j, int k)
{
	return i>=0 && j>=0 && k>=0 && i<m->size.x && j<m->size.y && k<m->size.z;
}

// rebuild a block's faces after its id changed (e.g. a chest becoming half of a double one)
static void refreshBlock(map_struct* m, int i, int j, int k)
{
	vect3D c=getCluster(m,i,j,k);
	removeBlock(m,i,j,k,false);
	addBlock(m,i,j,k);
	m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->changed=1;
}

// position of the other half of a double chest
static bool partnerPos(u8 state, int i, int j, int* pi, int* pj)
{
	int f=chestFacing(state), p=chestPartner(state);
	if(p==CHEST_SINGLE)return false;
	int s=(p==CHEST_PARTNER_POS)?1:-1;
	*pi=i+((f<2)?0:s);
	*pj=j+((f<2)?s:0);
	return true;
}

bool chestPlace(map_struct* m, int i, int j, int k)
{
	u8 nb[4], newState, neighbourState;
	int s, side, di, dj;
	if(!inWorld(m,i,j,k) || *getBlockP(m,i,j,k))return false;
	for(s=0;s<4;s++)
	{
		chestNeighbourOffset(s,&di,&dj);
		nb[s]=inWorld(m,i+di,j+dj,k)?*getBlockP(m,i+di,j+dj,k):0;
	}
	if(!chestPlan(nb,chestFacingFromLook(Player.angleZ),&newState,&side,&neighbourState))return false;
	changeBlock(m,i,j,k,newState);
	if(*getBlockP(m,i,j,k)!=newState)return false;     // refused (e.g. where the player stands)
	chestRemove(i,j,k);                                 // forget any stale contents there
	chestAt(i,j,k,true);
	if(side>=0)
	{
		chestNeighbourOffset(side,&di,&dj);
		*getBlockP(m,i+di,j+dj,k)=neighbourState;
		refreshBlock(m,i+di,j+dj,k);
	}
	return true;
}

bool chestOpen(map_struct* m, int i, int j, int k)
{
	u8 state=*getBlockP(m,i,j,k);
	int pi, pj;
	chest_struct *a, *b=NULL;
	if(!isChestBlock(state))return false;
	// creative has no item stacks to move in or out: the chest is only a block there
	if(!survivalEnabled())return true;
	// Minecraft: a chest with a solid block on top does not open
	if(k+1<m->size.z && solid(*getBlockP(m,i,j,k+1)))return true;
	a=chestAt(i,j,k,true);
	if(partnerPos(state,i,j,&pi,&pj) && inWorld(m,pi,pj,k) && isChestBlock(*getBlockP(m,pi,pj,k)))
	{
		if(k+1<m->size.z && solid(*getBlockP(m,pi,pj,k+1)))return true;
		b=chestAt(pi,pj,k,true);
		// the half at the lower coordinate holds the first 27 slots, as in Minecraft
		if(pi<i || pj<j){chest_struct* t=a;a=b;b=t;}
	}
	if(!a)return true;
	interfaceOpenChest(a,b);
	return true;
}

void chestBroken(map_struct* m, int i, int j, int k, u8 state)
{
	int pi, pj, n;
	chest_struct* c=chestAt(i,j,k,false);
	if(c)
	{
		// the contents fall out, as in Minecraft (in survival, where items drop)
		if(survivalEnabled())
			for(n=0;n<CHEST_SLOTS;n++)
				if(c->slots[n].count && c->slots[n].item)
					dropsSpawnStack(c->slots[n].item,c->slots[n].count,c->slots[n].wear,i,j,k);
		chestRemove(i,j,k);
	}
	// the other half becomes a single chest
	if(partnerPos(state,i,j,&pi,&pj) && inWorld(m,pi,pj,k))
	{
		u8 other=*getBlockP(m,pi,pj,k);
		if(isChestBlock(other))
		{
			*getBlockP(m,pi,pj,k)=chestState(chestFacing(other),CHEST_SINGLE);
			refreshBlock(m,pi,pj,k);
		}
	}
}

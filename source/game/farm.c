#include "game/game_main.h"

// Farming rules from Minecraft 1.4 (BlockFarmland, BlockCrops, BlockCarrot,
// BlockStem). They see the world through a plantWorld_struct (plants.h), so
// they run on the host tests too. Minecraft's y is up; here k is up.

static inline int at(const plantWorld_struct* w, int i, int j, int k)
{
	if(k<0 || k>=w->height)return 0;
	return w->get(w->ctx,i,j,k);
}

/* ---------------------------------------------------------------------------
 * Faces
 * ------------------------------------------------------------------------- */

// An attached stem is one plane through the block's centre, drawn from both
// sides (quad directions 14/15 along j, 16/17 along i). The bent picture
// bends towards u=1, which is +j on 14, -j on 15, +i on 16 and -i on 17.
static u8 attachedTile(int side, u8 direction)
{
	bool towardsU;
	switch(side)
	{
		case 0: towardsU=direction==16; break;   // pumpkin at +x
		case 1: towardsU=direction==17; break;   // -x
		case 2: towardsU=direction==14; break;   // +y
		default: towardsU=direction==15; break;  // -y
	}
	return towardsU?STEM_BENT_TILE:STEM_BENT_TILE+1;
}

u8 farmTexture(u8 b, u8 direction)
{
	if(isFarmland(b))return direction?0:(farmMoisture(b)?FARMLAND_WET_TILE:FARMLAND_DRY_TILE);   // dirt below and around
	if(isPumpkin(b))
	{
		if(direction<=1)return PUMPKIN_TOP_TILE;
		return (direction==2+(b-PUMPKIN_FIRST))?PUMPKIN_FACE_TILE:PUMPKIN_SIDE_TILE;
	}
	if(isCarrotCrop(b))
	{
		// BlockCarrot.getIcon: stages 0-1, 2-3, 4-6, 7
		int s=cropStage(b);
		return CARROT_TILE+((s<7)?((s==6?5:s)>>1):3);
	}
	if(isAttachedStem(b))return attachedTile(b-STEM_ATTACHED,direction);
	if(isStem(b))return STEM_TILE+cropStage(b);
	return 0;
}

/* ---------------------------------------------------------------------------
 * Farmland and growth
 * ------------------------------------------------------------------------- */

// BlockFarmland.isWaterNearby: water within 4 blocks, at the same level or one up
bool farmWaterNear(const plantWorld_struct* w, int i, int j, int k)
{
	int di, dj, dk;
	for(dk=0;dk<=1;dk++)for(dj=-4;dj<=4;dj++)for(di=-4;di<=4;di++)
		if(at(w,i+di,j+dj,k+dk)>=WATERTYPE)return true;
	return false;
}

static bool sameCrop(u8 self, int t)
{
	if(t<0)return false;
	return isCarrotCrop(self)?isCarrotCrop(t):(isStem(self) && isStem(t));
}

float farmGrowthChance(const plantWorld_struct* w, int i, int j, int k)
{
	u8 self=at(w,i,j,k);
	float f=1.0f;
	int di, dj;
	bool alongX=sameCrop(self,at(w,i-1,j,k)) || sameCrop(self,at(w,i+1,j,k));
	bool alongY=sameCrop(self,at(w,i,j-1,k)) || sameCrop(self,at(w,i,j+1,k));
	bool diagonal=sameCrop(self,at(w,i-1,j-1,k)) || sameCrop(self,at(w,i+1,j-1,k))
	            || sameCrop(self,at(w,i+1,j+1,k)) || sameCrop(self,at(w,i-1,j+1,k));
	for(dj=-1;dj<=1;dj++)for(di=-1;di<=1;di++)
	{
		int t=at(w,i+di,j+dj,k-1);
		float g=0;
		if(t>0 && isFarmland(t))g=farmMoisture(t)?3.0f:1.0f;
		if(di || dj)g/=4.0f;
		f+=g;
	}
	if(diagonal || (alongX && alongY))f/=2.0f;
	return f;
}

// BlockCrops / BlockStem.canBlockStay
bool farmPlantStays(const plantWorld_struct* w, int i, int j, int k)
{
	int below=at(w,i,j,k-1);
	if(below<=0 || !isFarmland(below))return false;
	return plantsLight(w,i,j,k,0)>=8 || plantsSeesSky(w,i,j,k);
}

int farmCropDrops(u8 b, u8* items, int max)
{
	int n=0, r, s=cropStage(b);
	if(isCarrotCrop(b))
	{
		// one carrot; a grown one rolls three more times (BlockCrops)
		if(n<max)items[n++]=ITEM_CARROT;
		if(s>=7)for(r=0;r<3;r++)if(rand()%15<=s && n<max)items[n++]=ITEM_CARROT;
	}else if(isStem(b))
	{
		// BlockStem: three rolls for a seed, better the more it grew
		for(r=0;r<3;r++)if(rand()%15<=s && n<max)items[n++]=ITEM_PUMPKIN_SEEDS;
	}
	return n;
}

// BlockStem.getState: the first pumpkin found at -x, +x, -y, +y
int farmStemFruitSide(const plantWorld_struct* w, int i, int j, int k)
{
	int t;
	if((t=at(w,i-1,j,k))>0 && isPumpkin(t))return 1;
	if((t=at(w,i+1,j,k))>0 && isPumpkin(t))return 0;
	if((t=at(w,i,j-1,k))>0 && isPumpkin(t))return 3;
	if((t=at(w,i,j+1,k))>0 && isPumpkin(t))return 2;
	return -1;
}

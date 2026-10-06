#include "game/game_main.h"

// Chest rules and contents. The parts that touch the world (placing, opening,
// breaking) are in chestworld.c; everything here is plain logic.

/* ---------------------------------------------------------------------------
 * Faces
 * ------------------------------------------------------------------------- */

// Direction in which a face's texture u coordinate grows, from initXYmap:
// face 2 (+x) runs towards -y, 3 (-x) towards +y, 4 (+y) towards +x, 5 (-y) towards -x.
static const s8 faceU[6][2]={{0,0},{0,0},{0,-1},{0,1},{1,0},{-1,0}};

u8 chestTexture(u8 state, u8 direction)
{
	int f=chestFacing(state), p=chestPartner(state);
	int front=2+f, back=front^1;
	if(direction<=1)return CHEST_TILE_TOP;
	if(direction!=front && direction!=back)return CHEST_TILE_SIDE;
	if(p==CHEST_SINGLE)return direction==front?CHEST_TILE_FRONT:CHEST_TILE_SIDE;
	// the other half lies across the front: along y for an x-facing chest, along x otherwise
	int px=(f<2)?0:(p==CHEST_PARTNER_POS?1:-1);
	int py=(f<2)?(p==CHEST_PARTNER_POS?1:-1):0;
	// the double texture reads left to right along the face's u: the half whose
	// partner is further along u shows the left tile
	bool left=faceU[direction][0]*px+faceU[direction][1]*py>0;
	if(direction==front)return left?CHEST_TILE_FRONT_L:CHEST_TILE_FRONT_R;
	return left?CHEST_TILE_BACK_L:CHEST_TILE_BACK_R;
}

// The front faces the player, as doors do (an angle of 0 looks towards +y)
int chestFacingFromLook(s16 angleZ)
{
	int a=((u16)angleZ)&32767;
	if(a<4096 || a>=32768-4096)return CHEST_FACE_NY;
	if(a<4096+8192)return CHEST_FACE_NX;
	if(a<4096+8192*2)return CHEST_FACE_PY;
	return CHEST_FACE_PX;
}

int chestNeighbourOffset(int side, int* di, int* dj)
{
	static const s8 off[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
	*di=off[side][0];
	*dj=off[side][1];
	return 2+side;
}

// Minecraft's rules: no chest next to two chests or next to a double chest;
// next to a single chest the two become a double chest facing the same way.
bool chestPlan(const u8 nb[4], int look, u8* newState, int* pairSide, u8* neighbourState)
{
	int s, count=0, side=-1;
	for(s=0;s<4;s++)if(isChestBlock(nb[s])){count++;side=s;}
	*pairSide=-1;
	if(count>=2)return false;
	if(!count)
	{
		*newState=chestState(look,CHEST_SINGLE);
		return true;
	}
	u8 n=nb[side];
	if(chestPartner(n)!=CHEST_SINGLE)return false;
	bool alongX=side<2;                          // the pair runs along x: faces must be along y
	int nf=chestFacing(n), f;
	#define ACROSS(face) (alongX?((face)>=2):((face)<2))
	if(ACROSS(nf))f=nf;                          // the new half takes the existing chest's facing
	else if(ACROSS(look))f=look;                 // or the existing chest turns to the player's
	else f=alongX?CHEST_FACE_NY:CHEST_FACE_NX;
	#undef ACROSS
	int mine=(side==0 || side==2)?CHEST_PARTNER_POS:CHEST_PARTNER_NEG;
	int theirs=(mine==CHEST_PARTNER_POS)?CHEST_PARTNER_NEG:CHEST_PARTNER_POS;
	*newState=chestState(f,mine);
	*neighbourState=chestState(f,theirs);
	*pairSide=side;
	return true;
}

/* ---------------------------------------------------------------------------
 * Contents
 * ------------------------------------------------------------------------- */

#define CHEST_FILE_MAGIC 0x48435344   // "DSCH"
#define CHEST_FILE_VERSION 1

static chest_struct* chests;
static int chestUsed, chestCap;

void chestsClear(void)
{
	if(chests)free(chests);
	chests=NULL;
	chestUsed=chestCap=0;
}

int chestCount(void)
{
	int n=0, i;
	for(i=0;i<chestUsed;i++)if(chests[i].used)n++;
	return n;
}

chest_struct* chestAt(int i, int j, int k, bool create)
{
	int n, freeSlot=-1;
	for(n=0;n<chestUsed;n++)
	{
		if(!chests[n].used){if(freeSlot<0)freeSlot=n;continue;}
		if(chests[n].i==i && chests[n].j==j && chests[n].k==k)return &chests[n];
	}
	if(!create)return NULL;
	if(freeSlot<0)
	{
		if(chestUsed==chestCap)
		{
			int cap=chestCap?chestCap*2:8;
			chest_struct* c=realloc(chests,cap*sizeof(chest_struct));
			if(!c)return NULL;
			chests=c;
			chestCap=cap;
		}
		freeSlot=chestUsed++;
	}
	chest_struct* c=&chests[freeSlot];
	memset(c,0,sizeof(chest_struct));
	c->i=i; c->j=j; c->k=k;
	c->used=1;
	return c;
}

void chestRemove(int i, int j, int k)
{
	chest_struct* c=chestAt(i,j,k,false);
	if(c)c->used=0;
}

void chestSidecarPath(const char* mapPath, char* out, int size)
{
	int len=strlen(mapPath);
	if(len>4 && !strcasecmp(mapPath+len-4,".map"))len-=4;
	snprintf(out,size,"%.*s.chests",len,mapPath);
}

bool chestsLoad(const char* mapPath)
{
	char path[300];
	u32 magic=0;
	u16 version=0, count=0;
	int n;
	chestsClear();
	chestSidecarPath(mapPath,path,sizeof(path));
	FILE* f=fopen(path,"rb");
	if(!f)return false;                          // no chests yet
	if(fread(&magic,4,1,f)!=1 || fread(&version,2,1,f)!=1 || fread(&count,2,1,f)!=1
	|| magic!=CHEST_FILE_MAGIC || version!=CHEST_FILE_VERSION){fclose(f);return false;}
	for(n=0;n<count;n++)
	{
		chest_struct c;
		if(fread(&c,sizeof(c),1,f)!=1)break;
		chest_struct* d=chestAt(c.i,c.j,c.k,true);
		if(d)memcpy(d->slots,c.slots,sizeof(c.slots));
	}
	fclose(f);
	return true;
}

bool chestsSave(const char* mapPath)
{
	char path[300];
	u32 magic=CHEST_FILE_MAGIC;
	u16 version=CHEST_FILE_VERSION, count=chestCount();
	int n;
	bool ok;
	chestSidecarPath(mapPath,path,sizeof(path));
	FILE* f=fopen(path,"wb");
	if(!f)return false;
	ok=fwrite(&magic,4,1,f)==1 && fwrite(&version,2,1,f)==1 && fwrite(&count,2,1,f)==1;
	for(n=0;n<chestUsed && ok;n++)if(chests[n].used)ok=fwrite(&chests[n],sizeof(chest_struct),1,f)==1;
	if(fclose(f))ok=false;
	return ok;
}

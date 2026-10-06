#include "game/game_main.h"

// Furnace rules and contents, following Minecraft Beta 1.7's TileEntityFurnace.
// The parts that touch the world (placing, opening, breaking, lighting up) are
// in furnaceworld.c; everything here is plain logic.

/* ---------------------------------------------------------------------------
 * Faces
 * ------------------------------------------------------------------------- */

u8 furnaceTexture(u8 state, u8 direction)
{
	if(direction<=1)return FURNACE_TILE_TOP;
	if(direction==2+furnaceFacing(state))return isLitFurnace(state)?FURNACE_TILE_FRONT_LIT:FURNACE_TILE_FRONT;
	return FURNACE_TILE_SIDE;
}

/* ---------------------------------------------------------------------------
 * Fuel and recipes (the items of Beta 1.7's lists that exist in this game)
 * ------------------------------------------------------------------------- */

int furnaceFuelTime(u8 item)
{
	// blocks made of wood burn for 300 ticks (1.5 items)
	if(item==ITEM_PLANKS || itemIsLog(item) || item==ITEM_CRAFTING_TABLE || item==ITEM_CHEST)return 300;
	if(item==ITEM_STICK)return 100;
	if(item==ITEM_COAL || item==ITEM_CHARCOAL)return 1600;   // 8 items
	return 0;
}

u8 furnaceSmeltResult(u8 item)
{
	if(item==ITEM_COBBLESTONE)return 3;          // stone
	if(item==6)return 12;                        // sand -> glass
	if(item==ITEM_IRON_ORE)return ITEM_IRON_INGOT;
	if(itemIsLog(item))return ITEM_CHARCOAL;
	return 0;
}

/* ---------------------------------------------------------------------------
 * Smelting
 * ------------------------------------------------------------------------- */

static inline bool emptySlot(const stack_struct* s){ return !s->count || !s->item; }

static bool canSmelt(const furnace_struct* f)
{
	const stack_struct* in=&f->slots[FURNACE_IN];
	const stack_struct* out=&f->slots[FURNACE_OUT];
	u8 r;
	if(emptySlot(in))return false;
	r=furnaceSmeltResult(in->item);
	if(!r)return false;
	if(emptySlot(out))return true;
	if(out->item!=r)return false;
	return out->count<itemMaxStack(r);
}

static void smelt(furnace_struct* f)
{
	stack_struct* in=&f->slots[FURNACE_IN];
	stack_struct* out=&f->slots[FURNACE_OUT];
	u8 r=furnaceSmeltResult(in->item);
	if(emptySlot(out)){out->item=r; out->count=1; out->wear=0;}
	else out->count++;
	if(!--in->count){in->item=0; in->wear=0;}
}

bool furnaceBurning(const furnace_struct* f)
{
	return f->burnTime>0;
}

bool furnaceTick(furnace_struct* f)
{
	bool was=f->burnTime>0;
	if(f->burnTime>0)f->burnTime--;
	if(!f->burnTime && canSmelt(f))
	{
		stack_struct* fuel=&f->slots[FURNACE_FUEL];
		f->burnMax=f->burnTime=emptySlot(fuel)?0:furnaceFuelTime(fuel->item);
		if(f->burnTime && !--fuel->count){fuel->item=0; fuel->wear=0;}
	}
	if(f->burnTime>0 && canSmelt(f))
	{
		if(++f->cookTime==FURNACE_COOK_TIME)
		{
			f->cookTime=0;
			smelt(f);
		}
	}else f->cookTime=0;
	return was!=(f->burnTime>0);
}

int furnaceCookScaled(const furnace_struct* f, int n)
{
	return f->cookTime*n/FURNACE_COOK_TIME;
}

int furnaceBurnScaled(const furnace_struct* f, int n)
{
	int max=f->burnMax?f->burnMax:FURNACE_COOK_TIME;
	return f->burnTime*n/max;
}

/* ---------------------------------------------------------------------------
 * Contents
 * ------------------------------------------------------------------------- */

#define FURNACE_FILE_MAGIC 0x52465344   // "DSFR"
#define FURNACE_FILE_VERSION 1

static furnace_struct* furnaces;
static int furnaceUsed, furnaceCap;

void furnacesClear(void)
{
	if(furnaces)free(furnaces);
	furnaces=NULL;
	furnaceUsed=furnaceCap=0;
}

int furnaceCount(void)
{
	int n=0, i;
	for(i=0;i<furnaceUsed;i++)if(furnaces[i].used)n++;
	return n;
}

int furnaceRecords(void)
{
	return furnaceUsed;
}

furnace_struct* furnaceGet(int n)
{
	if(n<0 || n>=furnaceUsed || !furnaces[n].used)return NULL;
	return &furnaces[n];
}

furnace_struct* furnaceAt(int i, int j, int k, bool create)
{
	int n, freeSlot=-1;
	for(n=0;n<furnaceUsed;n++)
	{
		if(!furnaces[n].used){if(freeSlot<0)freeSlot=n;continue;}
		if(furnaces[n].i==i && furnaces[n].j==j && furnaces[n].k==k)return &furnaces[n];
	}
	if(!create)return NULL;
	if(freeSlot<0)
	{
		if(furnaceUsed==furnaceCap)
		{
			int cap=furnaceCap?furnaceCap*2:8;
			furnace_struct* f=realloc(furnaces,cap*sizeof(furnace_struct));
			if(!f)return NULL;
			furnaces=f;
			furnaceCap=cap;
		}
		freeSlot=furnaceUsed++;
	}
	furnace_struct* f=&furnaces[freeSlot];
	memset(f,0,sizeof(furnace_struct));
	f->i=i; f->j=j; f->k=k;
	f->used=1;
	return f;
}

void furnaceRemove(int i, int j, int k)
{
	furnace_struct* f=furnaceAt(i,j,k,false);
	if(f)f->used=0;
}

void furnaceSidecarPath(const char* mapPath, char* out, int size)
{
	int len=strlen(mapPath);
	if(len>4 && !strcasecmp(mapPath+len-4,".map"))len-=4;
	snprintf(out,size,"%.*s.furnaces",len,mapPath);
}

bool furnacesLoad(const char* mapPath)
{
	char path[300];
	u32 magic=0;
	u16 version=0, count=0;
	int n;
	furnacesClear();
	furnaceSidecarPath(mapPath,path,sizeof(path));
	FILE* f=fopen(path,"rb");
	if(!f)return false;                          // no furnaces yet
	if(fread(&magic,4,1,f)!=1 || fread(&version,2,1,f)!=1 || fread(&count,2,1,f)!=1
	|| magic!=FURNACE_FILE_MAGIC || version!=FURNACE_FILE_VERSION){fclose(f);return false;}
	for(n=0;n<count;n++)
	{
		furnace_struct c;
		if(fread(&c,sizeof(c),1,f)!=1)break;
		furnace_struct* d=furnaceAt(c.i,c.j,c.k,true);
		if(!d)continue;
		memcpy(d->slots,c.slots,sizeof(c.slots));
		d->burnTime=c.burnTime;
		d->burnMax=c.burnMax;
		d->cookTime=c.cookTime;
	}
	fclose(f);
	return true;
}

bool furnacesSave(const char* mapPath)
{
	char path[300];
	u32 magic=FURNACE_FILE_MAGIC;
	u16 version=FURNACE_FILE_VERSION, count=furnaceCount();
	int n;
	bool ok;
	furnaceSidecarPath(mapPath,path,sizeof(path));
	if(!count && !furnaceUsed)
	{
		FILE* t=fopen(path,"rb");
		if(!t)return true;                       // never had a furnace: no file
		fclose(t);
	}
	FILE* f=fopen(path,"wb");
	if(!f)return false;
	ok=fwrite(&magic,4,1,f)==1 && fwrite(&version,2,1,f)==1 && fwrite(&count,2,1,f)==1;
	for(n=0;n<furnaceUsed && ok;n++)if(furnaces[n].used)ok=fwrite(&furnaces[n],sizeof(furnace_struct),1,f)==1;
	if(fclose(f))ok=false;
	return ok;
}

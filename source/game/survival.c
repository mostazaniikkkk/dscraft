#include "game/game_main.h"

bool cursorValid=false;
bool packHasItems=true;

static bool enabled;
static survivalSave_struct* save;

static bool dead;
static bool respawning;
static u16 deathTimer;
static u8 flashTimer;
static u16 air;
static u8 drownTimer;
static u16 sinceDamage;
static u8 regenTimer;
static bool airborne;
static int32 fallTop;

static u32 miningTarget;
static u8 miningBlock;
static u8 miningTool;
static u16 miningTicks;
static u16 miningNeed;

static u8 heartSprite;

// Item icons live in sub-screen sprite VRAM as 16x16 bitmaps, 8 per row of a
// 128-pixel-wide area (see getIcon/makeIcon in map.c). Stack counts and tool
// wear are drawn over a pristine copy of each icon so no extra sprites are needed.
#define ICON_BASE (256*288/2)
#define ICON_HEART_FULL 96
#define ICON_HEART_HALF 97
#define ICON_HEART_EMPTY 98

#define HEARTX 59
#define HEARTY 156
#define HEARTD 9

#define WHITE (RGB15(31,31,31)|BIT(15))
#define BLACK (RGB15(0,0,0)|BIT(15))
#define RED (RGB15(31,6,6)|BIT(15))

// pristine 16x16 icon of every item, copied from VRAM or drawn once at start
static u16* iconBase[SURVIVAL_ITEMTYPES];

static const u8 digitFont[10][5]={
	{7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
	{7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};

// 7x6 heart, drawn with a 1-pixel outline
static const u8 heartShape[6]={0x36,0x7F,0x7F,0x3E,0x1C,0x08};

/* ---------------------------------------------------------------------------
 * Block rules (Minecraft values). Break time in seconds is
 * hardness * (harvestable ? 1.5 : 5) / speed, where speed is the tool speed
 * when the right kind of tool is used and 1 otherwise.
 * ------------------------------------------------------------------------- */

typedef struct
{
	u8 hardness;   // tenths; 255 = unbreakable
	u8 tool;       // TOOLKIND_*
	u8 minTier;    // 0 = drops by hand, otherwise pickaxe tier needed (1 wood, 2 stone, 3 iron, 4 diamond)
}blockRule_struct;

#define HARD_UNBREAKABLE 255

static blockRule_struct blockRule(u8 t)
{
	if(isLadder(t))return (blockRule_struct){4,TOOLKIND_AXE,0};
	if(isDoor(t))return (blockRule_struct){30,TOOLKIND_AXE,0};
	if(isLeaves(t))return (blockRule_struct){2,TOOLKIND_NONE,0};     // hardness 0.2
	if(isSapling(t))return (blockRule_struct){0,TOOLKIND_NONE,0};    // breaks at once
	if(isCarrotCrop(t) || isStem(t))return (blockRule_struct){0,TOOLKIND_NONE,0};
	if(isFarmland(t))return (blockRule_struct){6,TOOLKIND_SHOVEL,0};  // hardness 0.6
	if(isPumpkin(t))return (blockRule_struct){10,TOOLKIND_AXE,0};     // hardness 1
	if(isChestBlock(t))return (blockRule_struct){25,TOOLKIND_AXE,0};   // hardness 2.5, like planks with a hand
	if(isFurnaceBlock(t))return (blockRule_struct){35,TOOLKIND_PICKAXE,1};   // hardness 3.5, drops with a pickaxe
	if(t>=14 && t<=28)return (blockRule_struct){8,TOOLKIND_NONE,0}; // wool
	switch(t)
	{
		case 1: return (blockRule_struct){6,TOOLKIND_SHOVEL,0};          // grass
		case 2: return (blockRule_struct){5,TOOLKIND_SHOVEL,0};          // dirt
		case 3: return (blockRule_struct){15,TOOLKIND_PICKAXE,1};        // stone
		case 4: return (blockRule_struct){20,TOOLKIND_PICKAXE,1};        // cobblestone
		case 5: return (blockRule_struct){HARD_UNBREAKABLE,0,0};         // bedrock
		case 6: return (blockRule_struct){5,TOOLKIND_SHOVEL,0};          // sand
		case 7: return (blockRule_struct){20,TOOLKIND_AXE,0};            // planks
		case 8: case 9: case 29: case 30:
			return (blockRule_struct){20,TOOLKIND_AXE,0};                // logs
		case 10: return (blockRule_struct){2,TOOLKIND_NONE,0};           // leaves
		case 12: return (blockRule_struct){3,TOOLKIND_NONE,0};           // glass
		case 13: return (blockRule_struct){0,TOOLKIND_NONE,0};           // torch
		case 31: return (blockRule_struct){50,TOOLKIND_PICKAXE,2};       // iron block
		case 32: return (blockRule_struct){30,TOOLKIND_PICKAXE,3};       // gold block
		case 33: return (blockRule_struct){50,TOOLKIND_PICKAXE,3};       // diamond block
		case 34: return (blockRule_struct){30,TOOLKIND_PICKAXE,2};       // lapis block
		case 35: return (blockRule_struct){20,TOOLKIND_PICKAXE,1};       // mossy cobblestone
		case 36: return (blockRule_struct){HARD_UNBREAKABLE-1,TOOLKIND_PICKAXE,4}; // obsidian
		case 37: return (blockRule_struct){4,TOOLKIND_PICKAXE,1};        // netherrack
		case 38: return (blockRule_struct){5,TOOLKIND_SHOVEL,0};         // soul sand
		case 39: return (blockRule_struct){3,TOOLKIND_NONE,0};           // glowstone
		case ITEM_CRAFTING_TABLE: return (blockRule_struct){25,TOOLKIND_AXE,0};
		case ITEM_COAL_ORE: return (blockRule_struct){30,TOOLKIND_PICKAXE,1};
		case ITEM_IRON_ORE: return (blockRule_struct){30,TOOLKIND_PICKAXE,2};   // a stone pickaxe or better
		default: return (blockRule_struct){10,TOOLKIND_NONE,0};
	}
}

static inline u8 toolSpeed(u8 tier){ return tier==1?2:(tier==2?4:(tier==3?6:1)); }   // EnumToolMaterial efficiency

u16 survivalToolDurability(u8 tool)
{
	if(tool==ITEM_WOOD_HOE)return HOE_DURABILITY_WOOD;
	if(tool==ITEM_STONE_HOE)return HOE_DURABILITY_STONE;
	if(tool==ITEM_IRON_HOE)return HOE_DURABILITY_IRON;
	return toolTier(tool)==1?60:(toolTier(tool)==2?132:(toolTier(tool)==3?251:0));
}

bool survivalCanHarvest(u8 block, u8 tool)
{
	blockRule_struct r=blockRule(block);
	if(!r.minTier)return true;
	return toolKind(tool)==r.tool && toolTier(tool)>=r.minTier;
}

u16 survivalBreakTicks(u8 block, u8 tool)
{
	blockRule_struct r=blockRule(block);
	if(r.hardness==HARD_UNBREAKABLE)return SURVIVAL_UNBREAKABLE;
	if(!r.hardness)return 1;
	u32 speed=(r.tool && toolKind(tool)==r.tool)?toolSpeed(toolTier(tool)):1;
	u32 factor=survivalCanHarvest(block,tool)?15:50;
	u32 ticks=(r.hardness*3*factor+10*speed-1)/(10*speed); // tenths * 30 Hz * factor/10, rounded up
	if(ticks<1)ticks=1;
	if(ticks>0xFFFE)ticks=0xFFFE;
	return ticks;
}

/* ---------------------------------------------------------------------------
 * Save data
 * ------------------------------------------------------------------------- */

static u32 checksumBytes(const u8* b, int len)
{
	u32 sum=0x1234;
	int i;
	for(i=0;i<len;i++)sum=(sum<<1|sum>>31)^b[i];
	return sum;
}

static u32 saveChecksum(survivalSave_struct* s)
{
	return checksumBytes((u8*)s,sizeof(survivalSave_struct)-sizeof(u32));
}


// Versions 1 and 2 kept one count per item type (and the wear of one tool of each
// kind); they are turned into stacks in the 36 slots.
#define SAVE_V1_SIZE 80            // 64 counts, checksum at byte 80
#define SAVE_V2_SIZE 100           // 72 counts + 6 tool wears, checksum at byte 100

static bool readOldSave(const survivalSave_struct* s, u8* counts, int* ncounts, u16* wear)
{
	const u8* b=(const u8*)s;
	u32 old;
	int size=(s->version==1)?SAVE_V1_SIZE:SAVE_V2_SIZE;
	memcpy(&old,b+size,sizeof(u32));
	if(old!=checksumBytes(b,size))return false;
	*ncounts=(s->version==1)?64:72;
	memcpy(counts,b+16,*ncounts);
	if(s->version==2)memcpy(wear,b+88,SURVIVAL_TOOLS*sizeof(u16));
	return true;
}

static bool upgradeSave(survivalSave_struct* s)
{
	u8 counts[72];
	u16 wear[SURVIVAL_TOOLS];
	int n, i;
	if(s->magic!=SURVIVAL_MAGIC)return false;
	if(s->version==SURVIVAL_VERSION)return s->checksum==saveChecksum(s);
	if(s->version!=1 && s->version!=2)return false;
	memset(wear,0,sizeof(wear));
	if(!readOldSave(s,counts,&n,wear))return false;
	memset((u8*)s+16,0,sizeof(survivalSave_struct)-16);
	s->version=SURVIVAL_VERSION;
	s->selected=0;
	for(i=1;i<n;i++)
	{
		bool tool=i>=ITEM_TOOL_FIRST && i<ITEM_TOOL_FIRST+SURVIVAL_TOOLS;
		int c=counts[i];
		// the worn tool of each kind keeps its wear, the others are new
		if(tool && c){inventoryAdd(i,1,wear[i-ITEM_TOOL_FIRST]);c--;}
		if(c)inventoryAdd(i,c,0);
	}
	return true;
}

/* ---------------------------------------------------------------------------
 * Icons
 * ------------------------------------------------------------------------- */

static inline u16* iconPixel(int id, int x, int y)
{
	return &SPRITE_GFX_SUB[ICON_BASE+x+(id%8)*16+(y+(id/8)*16)*128];
}

u16* survivalIconPixel(int id, int x, int y)
{
	return iconPixel(id,x,y);
}

u16 survivalIconTile(int id)
{
	return 8*2*(36+(id/8)*2)+2*(id%8);
}

static void drawDigits(int id, int n, u16 color)
{
	char str[12];
	int len, d, x, y, dx, dy, pass;
	if(n>999)n=999;
	sprintf(str,"%d",n);
	len=strlen(str);
	int x0=15-(len*4-1), y0=10;
	for(pass=0;pass<2;pass++)
	{
		for(d=0;d<len;d++)
		{
			const u8* glyph=digitFont[str[d]-'0'];
			for(y=0;y<5;y++)for(x=0;x<3;x++)
			{
				if(!((glyph[y]>>(2-x))&1))continue;
				int px=x0+d*4+x, py=y0+y;
				if(pass)*iconPixel(id,px,py)=color;
				else for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++)
				{
					if(px+dx>=0 && px+dx<16 && py+dy>=0 && py+dy<16)*iconPixel(id,px+dx,py+dy)=BLACK;
				}
			}
		}
	}
}

static void drawWearBar(int id, u16 wear, u16 durability)
{
	int x, left=(durability && wear<durability)?13*(durability-wear)/durability:0;
	u16 c=RGB15(31-31*left/13,31*left/13,0)|BIT(15);
	for(x=0;x<15;x++)
	{
		*iconPixel(id,x,14)=BLACK;
		*iconPixel(id,x,15)=BLACK;
	}
	for(x=1;x<=left;x++)*iconPixel(id,x,14)=c;
}

static void copyIcon(int dst, u8 item)
{
	int x, y;
	if(item>=SURVIVAL_ITEMTYPES || !iconBase[item])
	{
		for(y=0;y<16;y++)for(x=0;x<16;x++)*iconPixel(dst,x,y)=0;
		return;
	}
	for(y=0;y<16;y++)for(x=0;x<16;x++)*iconPixel(dst,x,y)=iconBase[item][x+y*16];
}

// Draw a stack into icon `dst`: the item, its count when above one (as in
// Minecraft) and the wear bar of a used tool.
void survivalDrawItemIcon(int dst, const stack_struct* s)
{
	if(!s->count || !s->item){copyIcon(dst,0xFF);return;}
	copyIcon(dst,s->item);
	if((isTool(s->item) || isHoe(s->item)) && s->wear)drawWearBar(dst,s->wear,survivalToolDurability(s->item));
	if(s->count>1)drawDigits(dst,s->count,WHITE);
}

static void drawHeart(int id, u16 left, u16 right)
{
	int x, y, dx, dy;
	for(y=0;y<16;y++)for(x=0;x<16;x++)*iconPixel(id,x,y)=0;
	for(y=0;y<6;y++)for(x=0;x<7;x++)
	{
		if(!((heartShape[y]>>(6-x))&1))continue;
		for(dy=0;dy<=2;dy++)for(dx=0;dx<=2;dx++)*iconPixel(id,x+dx,y+dy)=BLACK;
	}
	for(y=0;y<6;y++)for(x=0;x<7;x++)
	{
		if((heartShape[y]>>(6-x))&1)*iconPixel(id,x+1,y+1)=(x<4)?left:right;
	}
}

// Simple drawn tool, used when the texture pack has no gui/items.png
static void drawToolFallback(int id, u8 kind, u8 tier)
{
	int x, y, i;
	u16 handle=RGB15(17,11,5)|BIT(15);
	u16 head=(tier==1)?(RGB15(22,16,8)|BIT(15)):((tier==2)?(RGB15(18,18,18)|BIT(15)):(RGB15(27,27,28)|BIT(15)));
	for(y=0;y<16;y++)for(x=0;x<16;x++)*iconPixel(id,x,y)=0;
	for(i=2;i<13;i++)*iconPixel(id,i,15-i)=handle;
	switch(kind)
	{
		case TOOLKIND_PICKAXE:
			for(i=0;i<9;i++){*iconPixel(id,5+i,2+i/4)=head;*iconPixel(id,13-i/4,5+i)=head;}
			for(i=0;i<5;i++)*iconPixel(id,8+i,2+i)=head;
			break;
		case TOOLKIND_SHOVEL:
			for(y=1;y<6;y++)for(x=10;x<15;x++)if(abs(x-12)+abs(y-3)<4)*iconPixel(id,x,y)=head;
			break;
		case TOOLKIND_HOE:
			for(x=7;x<13;x++){*iconPixel(id,x,2)=head;*iconPixel(id,x,3)=head;}
			break;
		default:
			for(y=1;y<8;y++)for(x=8;x<13;x++)if(x-8>=y-4 || y<4)*iconPixel(id,x,y)=head;
			break;
	}
}

// The stick, drawn when the texture pack has no gui/items.png
static void drawStickFallback(int id)
{
	int x, y, i;
	for(y=0;y<16;y++)for(x=0;x<16;x++)*iconPixel(id,x,y)=0;
	for(i=3;i<13;i++)
	{
		*iconPixel(id,i,15-i)=RGB15(17,11,5)|BIT(15);
		*iconPixel(id,i+1,15-i)=RGB15(11,7,3)|BIT(15);
	}
}

// A lump of coal, drawn when the texture pack has no gui/items.png
static void drawCoalFallback(int id)
{
	int x, y;
	for(y=0;y<16;y++)for(x=0;x<16;x++)
	{
		int dx=x*2-15, dy=y*2-15;
		int d=dx*dx+dy*dy;
		*iconPixel(id,x,y)=(d<120)?((((x+y)&3)?RGB15(5,5,5):RGB15(10,10,10))|BIT(15)):((d<150)?(RGB15(0,0,0)|BIT(15)):0);
	}
}

// Without gui/items.png the item icons are drawn here; with it, map.c already
// copied them from the pack (loadBlockTextures).
// An apple, drawn when the texture pack has no gui/items.png
static void drawAppleFallback(int id)
{
	int x, y;
	for(y=0;y<16;y++)for(x=0;x<16;x++)
	{
		int dx=x*2-15, dy=y*2-17, d=dx*dx+dy*dy;
		u16 c=0;
		if(d<130)c=((x<7 && y<9)?RGB15(31,14,12):RGB15(26,3,3))|BIT(15);
		else if(d<160)c=RGB15(12,1,1)|BIT(15);
		if(x==8 && y>=1 && y<=4)c=RGB15(10,6,2)|BIT(15);
		if((x==9 || x==10) && y==2)c=RGB15(6,20,4)|BIT(15);
		*iconPixel(id,x,y)=c;
	}
}

static void loadItemIcons(void)
{
	int t;
	if(packHasItems)return;
	for(t=0;t<SURVIVAL_TOOLS;t++)drawToolFallback(ITEM_TOOL_FIRST+t,toolKind(ITEM_TOOL_FIRST+t),toolTier(ITEM_TOOL_FIRST+t));
	drawStickFallback(ITEM_STICK);
	drawCoalFallback(ITEM_COAL);
	drawCoalFallback(ITEM_CHARCOAL);
	drawAppleFallback(ITEM_APPLE);
	drawToolFallback(ITEM_WOOD_HOE,TOOLKIND_HOE,1);
	drawToolFallback(ITEM_STONE_HOE,TOOLKIND_HOE,2);
	for(t=ITEM_IRON_PICKAXE;t<=ITEM_IRON_AXE;t++)drawToolFallback(t,toolKind(t),3);
	drawToolFallback(ITEM_IRON_HOE,TOOLKIND_HOE,3);
	{
		// an ingot
		int x, y;
		for(y=0;y<16;y++)for(x=0;x<16;x++)
		{
			bool in=y>=6 && y<=10 && x>=3+(10-y) && x<=12-(y-6);
			*iconPixel(ITEM_IRON_INGOT,x,y)=in?(((y==6)?RGB15(29,29,29):RGB15(21,21,22))|BIT(15)):0;
		}
	}
	{
		int x, y;
		for(y=0;y<16;y++)for(x=0;x<16;x++)*iconPixel(ITEM_CHARCOAL,x,y)=charcoalTint(*iconPixel(ITEM_CHARCOAL,x,y));
	}
}

static void initIcons(void)
{
	int i, x, y;
	for(i=1;i<SURVIVAL_ITEMTYPES;i++)
	{
		bool hasIcon=(i<BLOCKS) || i==ITEM_CRAFTING_TABLE || i==ITEM_COAL_ORE || i==ITEM_CHEST || i==ITEM_FURNACE || i==ITEM_CHARCOAL || i==ITEM_SAPLING || i==ITEM_APPLE || (i>=ITEM_CARROT && i<=ITEM_IRON_ORE) || (i>=ITEM_IRON_INGOT && i<=ITEM_IRON_HOE) || (i>=ITEM_TOOL_FIRST && i<=ITEM_COAL);
		if(!hasIcon || iconBase[i])continue;
		iconBase[i]=malloc(16*16*sizeof(u16));
		if(!iconBase[i])continue;
		for(y=0;y<16;y++)for(x=0;x<16;x++)iconBase[i][x+y*16]=*iconPixel(i,x,y);
	}
	u16 red=RGB15(31,3,3)|BIT(15), dark=RGB15(8,2,2)|BIT(15);
	drawHeart(ICON_HEART_FULL,red,red);
	drawHeart(ICON_HEART_HALF,red,dark);
	drawHeart(ICON_HEART_EMPTY,dark,dark);
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

void survivalInitSprites(u8 firstSprite)
{
	int i;
	heartSprite=firstSprite;
	for(i=0;i<SURVIVAL_HEARTS;i++)oamSub.oamMemory[heartSprite+i].attribute[0]=ATTR0_DISABLED;
}

void survivalInit(map_struct* m, player_struct* p)
{
	setBrightness(1,0);
	enabled=false;
	save=NULL;
	dead=false;
	respawning=false;
	deathTimer=0;
	flashTimer=0;
	air=SURVIVAL_MAXAIR;
	drownTimer=0;
	sinceDamage=0;
	regenTimer=0;
	airborne=false;
	survivalStopMining();
	dropsClear();
	inventoryReset();

	// The game mode belongs to the world: survival worlds are created with a
	// survival block in their header (survivalFormatHeader), creative ones have none.
	save=(survivalSave_struct*)((u8*)m->header+SURVIVAL_HEADER_OFFSET);
	enabled=true;                  // upgrading an old save uses the inventory functions
	if(!upgradeSave(save))
	{
		enabled=false;
		save=NULL;
		return;
	}
	if(!save->health || save->health>SURVIVAL_MAXHEALTH)save->health=SURVIVAL_MAXHEALTH;
	save->selected%=INV_HOTBAR;
	noclip=false;
	loadItemIcons();
	initIcons();
	creativeDrawArrowIcons();   // page arrows of a double chest
	// items left in the crafting grid or on the stylus when the world was saved
	inventoryClose();
}

void survivalKill(void)
{
	int i;
	for(i=0;i<SURVIVAL_ITEMTYPES;i++)
	{
		if(iconBase[i])free(iconBase[i]);
		iconBase[i]=NULL;
	}
	setBrightness(1,0);
	dropsClear();
	inventoryReset();
	enabled=false;
	save=NULL;
}

// Called by the world generator for a new survival world: the survival block
// with full health, an empty inventory and the world's spawn point.
void survivalFormatHeader(u8* header)
{
	header_struct* h=(header_struct*)header;
	survivalSave_struct* s=(survivalSave_struct*)(header+SURVIVAL_HEADER_OFFSET);
	memset(s,0,sizeof(survivalSave_struct));
	s->magic=SURVIVAL_MAGIC;
	s->version=SURVIVAL_VERSION;
	s->health=SURVIVAL_MAXHEALTH;
	s->worldSpawnX=h->spawnX;
	s->worldSpawnY=h->spawnY;
	s->worldSpawnZ=h->spawnZ;
	s->checksum=saveChecksum(s);
}

survivalSave_struct* survivalData(void)
{
	return enabled?save:NULL;
}

bool survivalEnabled(void)
{
	return enabled;
}

bool survivalInputLocked(void)
{
	return enabled && dead;
}

/* ---------------------------------------------------------------------------
 * Inventory
 * ------------------------------------------------------------------------- */

static u8 itemFromBlock(u8 t)
{
	if(!t || t>=WATERTYPE)return 0;
	if(t==1)return 2; // grass drops dirt
	if(t==3)return 4; // stone drops cobblestone
	if(isLadder(t))return LADDERTYPE;
	if(isDoor(t))return DOORTYPE;
	if(t==ITEM_COAL_ORE)return ITEM_COAL;
	if(isChestBlock(t))return ITEM_CHEST;
	if(isFurnaceBlock(t))return ITEM_FURNACE;
	if(isSapling(t))return ITEM_SAPLING;
	if(isLeaves(t))return 0;          // never themselves without shears: see plantsLeafDrops
	if(isFarmland(t))return 2;        // dirt
	if(isPumpkin(t))return ITEM_PUMPKIN;
	if(isCarrotCrop(t) || isStem(t))return 0;   // see farmCropDrops
	if(t==ITEM_IRON_ORE)return t;
	if(t>=ITEM_TOOL_FIRST)return 0;
	return t;
}

// placing uses the selected hotbar slot
bool survivalCanPlace(u8 item)
{
	if(item>=ITEM_TOOL_FIRST && item!=ITEM_SAPLING && !isCubeItem(item))return false;
	if(!enabled)return true;
	stack_struct* s=inventorySelected();
	return s->count && s->item==item;
}

// Beta food: eaten at once with the place button, it heals and is used up
// (also at full health, as in Beta)
bool survivalEat(void)
{
	stack_struct* s;
	if(!enabled || dead)return false;
	s=inventorySelected();
	int heal;
	if(!s->count)return false;
	if(s->item==ITEM_APPLE)heal=APPLE_HEAL;
	else if(s->item==ITEM_CARROT)heal=CARROT_HEAL;
	else return false;
	save->health=(save->health+heal>SURVIVAL_MAXHEALTH)?SURVIVAL_MAXHEALTH:save->health+heal;
	if(!--s->count){s->item=0;s->wear=0;}
	return true;
}

// a hoe wears by one use per tilled block and breaks when its uses run out
void survivalWearSelected(void)
{
	stack_struct* s;
	if(!enabled)return;
	s=inventorySelected();
	if(!s->count || !isHoe(s->item))return;
	if(++s->wear>=survivalToolDurability(s->item)){s->item=0;s->count=0;s->wear=0;}
}

void survivalConsume(u8 item)
{
	if(!enabled)return;
	stack_struct* s=inventorySelected();
	if(!s->count || s->item!=item)return;
	if(!--s->count){s->item=0;s->wear=0;}
}

bool survivalCanBreak(u8 block)
{
	return !enabled || blockRule(block).hardness!=HARD_UNBREAKABLE;
}

/* ---------------------------------------------------------------------------
 * Mining
 * ------------------------------------------------------------------------- */

u8 survivalHeldTool(void)
{
	if(!enabled)return 0;
	stack_struct* s=inventorySelected();
	return (s->count && isTool(s->item))?s->item:0;
}

void survivalStopMining(void)
{
	miningTicks=0;
	miningNeed=0;
}

bool survivalMine(void)
{
	if(!enabled || dead || !cursorValid){survivalStopMining();return false;}
	u8 block=*getBlockP(&map,testCursorI,testCursorJ,testCursorK);
	u8 tool=survivalHeldTool();
	if(!miningNeed || testCursor!=miningTarget || block!=miningBlock || tool!=miningTool)
	{
		miningTarget=testCursor;
		miningBlock=block;
		miningTool=tool;
		miningTicks=0;
		miningNeed=survivalBreakTicks(block,tool);
	}
	if(miningNeed==SURVIVAL_UNBREAKABLE || !block)return false;
	if(++miningTicks<miningNeed)return false;
	survivalStopMining();
	return true;
}

u16 survivalMiningTicks(void)
{
	return miningTicks;
}

u8 survivalMiningProgress(void)
{
	if(!miningNeed || miningNeed==SURVIVAL_UNBREAKABLE)return 0;
	return (u32)miningTicks*255/miningNeed;
}

void survivalBlockBroken(u8 block, int i, int j, int k)
{
	if(!enabled)return;
	u8 tool=survivalHeldTool();
	if(survivalCanHarvest(block,tool))
	{
		u8 item=itemFromBlock(block);
		if(item)dropsSpawn(item,i,j,k);
		if(isLeaves(block))plantsLeafDropsAt(i,j,k);
		if(isCarrotCrop(block) || isStem(block))
		{
			u8 it[4];
			int n, c=farmCropDrops(block,it,4);
			for(n=0;n<c;n++)dropsSpawn(it[n],i,j,k);
		}
	}
	// the tool in hand wears out; it breaks when its uses run out
	if(tool && blockRule(block).hardness)
	{
		stack_struct* s=inventorySelected();
		if(++s->wear>=survivalToolDurability(tool)){s->item=0;s->count=0;s->wear=0;}
	}
}

/* ---------------------------------------------------------------------------
 * Health
 * ------------------------------------------------------------------------- */

void survivalDamage(u8 amount)
{
	if(!enabled || dead || !amount)return;
	sinceDamage=0;
	flashTimer=8;
	if(amount>=save->health)
	{
		save->health=0;
		dead=true;
		deathTimer=0;
		survivalStopMining();
	}else save->health-=amount;
}

static void respawn(void)
{
	// Save the world with the spawn point in the header, then reload the game
	// state so the map streams in around the spawn point.
	save->health=SURVIVAL_MAXHEALTH;
	respawning=true;
	globalSaveMap(&map);
	respawning=false;
	DS_ChangeState(&Game_State);
}

void survivalUpdate(player_struct* p)
{
	if(!enabled)return;

	if(dead)
	{
		deathTimer++;
		int fade=deathTimer/2;
		if(fade>16)fade=16;
		setBrightness(1,-fade);
		if(deathTimer==45)respawn();
		return;
	}

	// fall damage: one half-heart per block fallen beyond three
	if(p->onLadder || p->inWater || noclip)airborne=false;
	else if(p->vector.z)
	{
		// survivalUpdate runs after the move, so the take-off height is one step back
		if(!airborne)fallTop=p->position.z-p->vector.z;
		if(p->position.z>fallTop)fallTop=p->position.z;
		airborne=true;
	}else if(airborne)
	{
		airborne=false;
		int blocks=(fallTop-p->position.z)/rTilesize2;
		if(blocks>3)survivalDamage(blocks-3);
		farmLanded(&map,p,(fallTop-p->position.z)*4096/rTilesize2);
	}

	// the void's death zone
	if(p->position.z<(-VOID_DEATH_DEPTH-map.size.z/2)*rTilesize2)survivalDamage(VOID_DAMAGE);

	// drowning
	if(p->inWater==2)
	{
		if(air)air--;
		else if(++drownTimer>=30){drownTimer=0;survivalDamage(2);}
	}else{
		air=SURVIVAL_MAXAIR;
		drownTimer=0;
	}

	// natural regeneration after 4 s without damage
	if(sinceDamage<120)sinceDamage++;
	else if(save->health<SURVIVAL_MAXHEALTH && ++regenTimer>=60)
	{
		regenTimer=0;
		save->health++;
	}

	if(flashTimer)flashTimer--;
	if(!dead)setBrightness(1,-flashTimer);
	dropsUpdate(&map,p);
}

void survivalUpdateHUD(bool inventoryOpen)
{
	int i;
	for(i=0;i<SURVIVAL_HEARTS;i++)
	{
		if(!enabled || inventoryOpen)
		{
			oamSub.oamMemory[heartSprite+i].attribute[0]=ATTR0_DISABLED;
			continue;
		}
		int icon=ICON_HEART_EMPTY;
		if(save->health>=(i+1)*2)icon=ICON_HEART_FULL;
		else if(save->health==i*2+1)icon=ICON_HEART_HALF;
		oamSub.oamMemory[heartSprite+i].attribute[0]=ATTR0_BMP | ATTR0_SQUARE | HEARTY;
		oamSub.oamMemory[heartSprite+i].attribute[1]=ATTR1_SIZE_16 | (HEARTX+i*HEARTD);
		oamSub.oamMemory[heartSprite+i].attribute[2]=ATTR2_ALPHA(1) | ATTR2_PRIORITY(0) | survivalIconTile(icon);
	}
	if(!enabled)return;
	inventoryUpdateUI(inventoryOpen);
}

void survivalWriteHeader(map_struct* m)
{
	if(!enabled)return;
	if(respawning)
	{
		m->header->spawnX=save->worldSpawnX;
		m->header->spawnY=save->worldSpawnY;
		m->header->spawnZ=save->worldSpawnZ;
	}
	save->checksum=saveChecksum(save);
}

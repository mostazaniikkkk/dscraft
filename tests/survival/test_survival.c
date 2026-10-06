/* Host tests for survival.c, inventory.c, drops.c, chest.c, furnace.c, plants.c and farm.c (rules only, no rendering). */
#include "game/game_main.h"
#include "maxmod9.h"

u16 SPRITE_GFX_SUB[65536];
OamState oamSub;
int brightness;
map_struct map;
bool noclip;
u8 cursorBlock;
u8 cursorSprite=29;
u32 testCursor, testCursorI, testCursorJ, testCursorK;
u8 worldBlock;
char packPath[255]=".";
item_struct items[MAXITEMS];
slot_struct slots[MAXSLOTS];
DS_state Game_State;
int stateChanges, saves, sfxPlayed;
touchPosition thisXY, lastXY;
u32 keysDownValue, keysHeldValue, keysUpValue;
player_struct Player;
static u8 headerBuf[2048];

void globalSaveMap(map_struct* m)
{
	/* mirrors writeMapHeader: spawn = player position, then the survival hook */
	m->header->spawnX=1; m->header->spawnY=2; m->header->spawnZ=Player.position.z;
	survivalWriteHeader(m);
	saves++;
}

/* farmworld.c is not built here: the landing hook only records the fall */
static int32 lastLanding=-1;
void farmLanded(map_struct* m, player_struct* p, int32 fall){ (void)m; (void)p; lastLanding=fall; }

static int failures;
#define CHECK(c) do{ if(!(c)){ printf("FAIL %s:%d: %s\n",__FILE__,__LINE__,#c); failures++; } }while(0)

static survivalSave_struct* S(void){ return (survivalSave_struct*)(headerBuf+SURVIVAL_HEADER_OFFSET); }
static u8 health(void){ return S()->health; }
static stack_struct* slot(int i){ return &S()->slots[i]; }
static stack_struct* cell(int i){ return &S()->grid[i]; }
static void tick(int n){ while(n--) survivalUpdate(&Player); }
static bool is(const stack_struct* s, u8 item, int count){ return s->item==item && s->count==count; }
static bool emptyStack(const stack_struct* s){ return !s->count && !s->item; }

/* terrain seen by dropped items: stone up to groundTop, optional water above it,
   plus one extra solid cell */
static int groundTop=20;
static bool waterAbove;
static int solidI=-1, solidJ, solidK;
u8* getBlockPE(map_struct* m, int i, int j, int k)
{
	static u8 v;
	(void)m;
	if(i==solidI && j==solidJ && k==solidK) v=3;
	else if(k<=groundTop) v=3;
	else if(waterAbove && k<=groundTop+6) v=WATERTYPE;
	else v=0;
	return &v;
}

static void standOn(const drop_struct* d)
{
	Player.position.x=d->x-(SUPERCLUSTERSIZE*CLUSTERSIZE/2)*DROP_UNIT;
	Player.position.y=d->y-(SUPERCLUSTERSIZE*CLUSTERSIZE/2)*DROP_UNIT;
	Player.position.z=d->z-(map.size.z/2)*DROP_UNIT;
}

static void collect(void)
{
	int n, t;
	for(n=0;n<DROPS_MAX;n++)
		for(t=0;t<60 && dropsGet(n);t++){ standOn(dropsGet(n)); dropsUpdate(&map,&Player); }
}

static int dropsUsed(void){ int n, c=0; for(n=0;n<DROPS_MAX;n++) if(dropsGet(n)) c++; return c; }
static const drop_struct* findDrop(u8 item){ int n; for(n=0;n<DROPS_MAX;n++) if(dropsGet(n) && dropsGet(n)->item==item) return dropsGet(n); return NULL; }

void fall(int blocks);
static void brk(u8 block){ survivalBlockBroken(block,100,100,30); collect(); }

static void setupInterface(void)
{
	int i;
	memset(items,0,sizeof(items));
	memset(slots,0,sizeof(slots));
	for(i=0;i<MAXITEMS;i++) items[i].id=40+i>127?127:40+i;
	for(i=0;i<MAXSLOTS;i++) slots[i].id=-1;
}

static void setup(bool survival, bool garbage)
{
	int i;
	memset(headerBuf, garbage?0xAB:0, sizeof(headerBuf));
	map.header=(header_struct*)headerBuf;
	map.header->spawnX=100; map.header->spawnY=200; map.header->spawnZ=5000;
	map.offset=(vect3D){0,0,0}; map.size=(vect3D){256,256,64};
	groundTop=20; waterAbove=false; solidI=-1;
	memset(&Player,0,sizeof(Player));
	Player.position.z=5000;
	setupInterface();
	for(i=0;i<65536;i++) SPRITE_GFX_SUB[i]=0x8000|(i&0x7fff);
	if(survival) survivalFormatHeader(headerBuf);    /* created as a survival world */
	stateChanges=saves=0;
	keysDownValue=keysHeldValue=keysUpValue=0;
	survivalInitSprites(30);
	survivalInit(&map,&Player);
}

static u32 checksumBytes(const u8* b, int len)
{
	u32 sum=0x1234; int i;
	for(i=0;i<len;i++) sum=(sum<<1|sum>>31)^b[i];
	return sum;
}

/* ------------------------------------------------------------------------- */

static void testHealth(void)
{
	setup(false,true);
	CHECK(!survivalEnabled() && !survivalData());
	CHECK(survivalCanPlace(2) && survivalCanBreak(5) && !survivalCanPlace(ITEM_WOOD_PICKAXE));
	survivalBlockBroken(1,100,100,30); survivalDamage(5);
	CHECK(dropsUsed()==0 && !survivalMine());
	survivalUpdateHUD(false);
	CHECK(oamSub.oamMemory[30].attribute[0]==ATTR0_DISABLED);
	survivalKill();

	setup(true,true);
	CHECK(survivalEnabled() && survivalData()==S());
	CHECK(health()==SURVIVAL_MAXHEALTH && S()->version==SURVIVAL_VERSION);
	CHECK(S()->worldSpawnX==100 && S()->worldSpawnY==200 && S()->worldSpawnZ==5000);
	{ int i, n=0; for(i=0;i<INV_SLOTS;i++) if(!emptyStack(slot(i))) n++; CHECK(n==0); }

	fall(3);
	CHECK(health()==20);
	fall(10); CHECK(health()==13);
	Player.inWater=1; Player.vector.z=-100; tick(5); Player.vector.z=0; tick(1); CHECK(health()==13);
	Player.inWater=0;
	tick(119); CHECK(health()==13);
	tick(1+60); CHECK(health()==14);
	tick(60); CHECK(health()==15);
	tick(60*10); CHECK(health()==SURVIVAL_MAXHEALTH);
	Player.inWater=2;
	tick(SURVIVAL_MAXAIR); CHECK(health()==20);
	tick(30); CHECK(health()==18);
	Player.inWater=0; tick(1);
	Player.inWater=2; tick(SURVIVAL_MAXAIR-1); CHECK(health()>=18);
	Player.inWater=0;
	survivalDamage(1); tick(1); CHECK(brightness<0);
	tick(10); CHECK(brightness==0);

	inventoryAdd(4,10,0);
	survivalDamage(50);
	CHECK(health()==0 && survivalInputLocked());
	tick(44); CHECK(saves==0 && brightness==-16);
	tick(1);
	CHECK(saves==1 && stateChanges==1);
	CHECK(map.header->spawnX==100 && map.header->spawnY==200 && map.header->spawnZ==5000);
	CHECK(health()==SURVIVAL_MAXHEALTH);
	survivalKill();
	CHECK(brightness==0);
	setupInterface(); survivalInit(&map,&Player);
	CHECK(survivalEnabled() && is(slot(0),4,10));                          /* the inventory is kept */
	survivalKill();

	/* the mode belongs to the world: a corrupted or missing survival block means creative */
	headerBuf[SURVIVAL_HEADER_OFFSET+20]^=0xFF;
	setupInterface(); survivalInit(&map,&Player);
	CHECK(!survivalEnabled() && !survivalData());
	survivalKill();
	memset(headerBuf+SURVIVAL_HEADER_OFFSET,0,sizeof(survivalSave_struct));
	setupInterface(); survivalInit(&map,&Player);
	CHECK(!survivalEnabled());
	survivalKill();
	/* a new survival world: full health, empty inventory, spawn from the generator */
	map.header->spawnX=7; map.header->spawnY=9; map.header->spawnZ=-4096;
	survivalFormatHeader(headerBuf);
	setupInterface(); survivalInit(&map,&Player);
	CHECK(survivalEnabled() && health()==SURVIVAL_MAXHEALTH && S()->worldSpawnX==7 && S()->worldSpawnY==9 && S()->worldSpawnZ==-4096);
	CHECK(emptyStack(slot(0)) && emptyStack(inventoryHeld()));
	survivalKill();
}

/* simulate a fall of `blocks` blocks */
void fall(int blocks);
void fall(int blocks)
{
	int i;
	Player.inWater=0; Player.onLadder=0;
	Player.vector.z=0; tick(1);
	for(i=0;i<blocks;i++){ Player.vector.z=-rTilesize2; Player.position.z-=rTilesize2; tick(1); }
	Player.vector.z=0; tick(1);
}

static void testStacks(void)
{
	setup(true,false);
	CHECK(itemMaxStack(2)==64 && itemMaxStack(ITEM_WOOD_PICKAXE)==1 && itemMaxStack(DOORTYPE)==1 && itemMaxStack(ITEM_STICK)==64);
	CHECK(inventoryAdd(2,70,0)==0);
	CHECK(is(slot(0),2,64) && is(slot(1),2,6));
	CHECK(inventoryAdd(2,60,0)==0);
	CHECK(is(slot(1),2,64) && is(slot(2),2,2));        /* fills existing stacks first */
	CHECK(inventoryAdd(ITEM_WOOD_PICKAXE,3,7)==0);
	CHECK(is(slot(3),ITEM_WOOD_PICKAXE,1) && is(slot(5),ITEM_WOOD_PICKAXE,1) && slot(4)->wear==7);
	CHECK(inventoryCount(2)==130);
	{ int i; for(i=6;i<INV_SLOTS;i++) inventoryAdd(3,64,0); }
	CHECK(inventoryAdd(3,5,0)==5);                     /* full */
	CHECK(inventoryAdd(2,62,0)==0 && inventoryAdd(2,1,0)==1);   /* dirt slot 2 had room for 62 */
	survivalKill();
}

static void testClicks(void)
{
	setup(true,false);
	inventoryAdd(2,40,0);                             /* slot 0 */
	inventoryAdd(4,30,0);                             /* slot 1 */
	inventoryOpen(false);

	/* left: pick the whole stack, put it down */
	inventoryClick(0,false,false);
	CHECK(emptyStack(slot(0)) && is(inventoryHeld(),2,40));
	inventoryClick(9,false,false);
	CHECK(is(slot(9),2,40) && emptyStack(inventoryHeld()));

	/* right: take half (rounded up), put one at a time */
	inventoryClick(1,true,false);
	CHECK(is(inventoryHeld(),4,15) && is(slot(1),4,15));
	inventoryClick(10,true,false); inventoryClick(10,true,false);
	CHECK(is(slot(10),4,2) && is(inventoryHeld(),4,13));
	inventoryClick(1,false,false);                  /* merge back */
	CHECK(is(slot(1),4,28) && emptyStack(inventoryHeld()));

	/* merging stops at 64, the rest stays on the stylus */
	inventoryAdd(2,40,0);                             /* 64 at slot 9 now (40+24) and 16 in slot 0 */
	CHECK(is(slot(9),2,64) && is(slot(0),2,16));
	/* a full stack of the same item takes nothing (Beta 1.7: no swap either) */
	inventoryClick(0,false,false);
	inventoryClick(9,false,false);
	CHECK(is(slot(9),2,64) && is(inventoryHeld(),2,16));
	inventoryClick(0,false,false);
	CHECK(is(slot(0),2,16) && emptyStack(inventoryHeld()));

	/* different items swap */
	inventoryClick(1,false,false);
	inventoryClick(0,false,false);
	CHECK(is(slot(0),4,28) && is(inventoryHeld(),2,16));
	inventoryClick(1,false,false);
	CHECK(is(slot(1),2,16) && emptyStack(inventoryHeld()));

	/* shift: hotbar -> main inventory (merging with the stack there) and back */
	inventoryClick(0,false,true);
	CHECK(emptyStack(slot(0)) && is(slot(10),4,30));
	inventoryClick(10,false,true);
	CHECK(is(slot(0),4,30) && emptyStack(slot(10)));

	/* drag: press on a slot, lift the stylus on another */
	inventoryPress(1,false,false);
	inventoryRelease(20);
	CHECK(emptyStack(slot(1)) && is(slot(20),2,16) && emptyStack(inventoryHeld()));
	/* lifting on the same slot keeps the stack on the stylus (tap, then tap again) */
	inventoryPress(20,false,false);
	inventoryRelease(20);
	CHECK(is(inventoryHeld(),2,16));
	inventoryPress(21,false,false); inventoryRelease(21);
	CHECK(is(slot(21),2,16) && emptyStack(inventoryHeld()));

	/* outside the window: throw (L: one) */
	dropsClear();
	inventoryClick(21,false,false);
	inventoryClick(SLOT_OUTSIDE,true,false);
	CHECK(is(inventoryHeld(),2,15) && findDrop(2) && findDrop(2)->count==1 && findDrop(2)->delay==DROP_THROW_DELAY);
	inventoryClick(SLOT_OUTSIDE,false,false);
	CHECK(emptyStack(inventoryHeld()) && findDrop(2)->count==16);   /* merged with the first one */

	/* the 2x2 grid only has four cells */
	inventoryAdd(2,5,0);
	inventoryClick(0,false,false);                    /* cobblestone */
	inventoryClick(SLOT_GRID+2,false,false);
	CHECK(emptyStack(cell(2)) && !emptyStack(inventoryHeld()));
	inventoryClick(SLOT_GRID+4,false,false);
	CHECK(is(cell(4),4,30));

	/* closing returns the grid and the stylus to the inventory */
	inventoryClick(9,false,false);                    /* pick dirt up */
	inventoryClose();
	CHECK(emptyStack(cell(4)) && emptyStack(inventoryHeld()));
	CHECK(inventoryCount(4)==30 && !inventoryIsOpen());

	/* closing with a full inventory throws what does not fit */
	{
		int i;
		dropsClear();
		inventoryOpen(false);
		inventoryAdd(ITEM_STICK,10,0);
		inventoryClick(slot(0)->item==ITEM_STICK?0:0,false,false);
		for(i=0;i<INV_SLOTS;i++) if(emptyStack(slot(i))) inventoryAdd(3,64,0);
		CHECK(inventoryAdd(3,1,0)==1);
		inventoryClose();
	}
	survivalKill();
}

static void testDragSpread(void)
{
	setup(true,false);
	inventoryOpen(false);

	/* the crafting table square: pick up planks, drag over the 2x2 grid with L */
	inventoryAdd(ITEM_PLANKS,10,0);                    /* slot 0 */
	inventoryPress(0,true,false);                      /* L: half (5) */
	CHECK(is(inventoryHeld(),ITEM_PLANKS,5));
	inventoryDragOver(SLOT_GRID+0); inventoryDragOver(SLOT_GRID+1);
	inventoryDragOver(SLOT_GRID+4); inventoryDragOver(SLOT_GRID+3);
	inventoryRelease(SLOT_GRID+3);
	CHECK(is(cell(0),ITEM_PLANKS,1) && is(cell(1),ITEM_PLANKS,1) && is(cell(3),ITEM_PLANKS,1) && is(cell(4),ITEM_PLANKS,1));
	CHECK(is(inventoryHeld(),ITEM_PLANKS,1) && is(slot(0),ITEM_PLANKS,5));
	{ stack_struct r; CHECK(craftingResult(&r) && r.item==ITEM_CRAFTING_TABLE); }
	inventoryClose();
	CHECK(inventoryCount(ITEM_PLANKS)==10);

	/* without L the stack is spread evenly; the rest stays on the stylus */
	inventoryOpen(false);
	inventoryPress(0,false,false);                     /* all 10 */
	inventoryDragOver(SLOT_GRID+0); inventoryDragOver(SLOT_GRID+1); inventoryDragOver(SLOT_GRID+3);
	inventoryRelease(SLOT_GRID+3);
	CHECK(is(cell(0),ITEM_PLANKS,3) && is(cell(1),ITEM_PLANKS,3) && is(cell(3),ITEM_PLANKS,3) && is(inventoryHeld(),ITEM_PLANKS,1));
	inventoryClose();

	/* passing over the same slot twice counts once; going back to the origin counts */
	inventoryOpen(false);
	inventoryPress(0,false,false);                     /* 10 planks picked from slot 0 */
	inventoryDragOver(1); inventoryDragOver(0); inventoryDragOver(1);
	inventoryRelease(1);
	CHECK(is(slot(0),ITEM_PLANKS,5) && is(slot(1),ITEM_PLANKS,5) && emptyStack(inventoryHeld()));

	/* holding a stack, a new press starts the spread from that slot */
	inventoryClick(0,false,false);                     /* 5 on the stylus */
	inventoryPress(20,true,false);
	inventoryDragOver(21); inventoryDragOver(22);
	inventoryRelease(22);
	CHECK(is(slot(20),ITEM_PLANKS,1) && is(slot(21),ITEM_PLANKS,1) && is(slot(22),ITEM_PLANKS,1) && is(inventoryHeld(),ITEM_PLANKS,2));

	/* fewer items than slots: one each until they run out */
	inventoryPress(23,false,false);
	inventoryDragOver(24); inventoryDragOver(25); inventoryDragOver(26);
	inventoryRelease(26);
	CHECK(is(slot(23),ITEM_PLANKS,1) && is(slot(24),ITEM_PLANKS,1) && emptyStack(slot(25)) && emptyStack(inventoryHeld()));

	/* slots holding something else are skipped */
	inventoryAdd(4,1,0);                               /* cobblestone lands in slot 0 */
	CHECK(is(slot(0),4,1));
	inventoryClick(1,false,false);                     /* 5 planks */
	inventoryPress(2,false,false);
	inventoryDragOver(0); inventoryDragOver(3);
	inventoryRelease(3);
	CHECK(is(slot(0),4,1) && is(slot(2),ITEM_PLANKS,2) && is(slot(3),ITEM_PLANKS,2) && is(inventoryHeld(),ITEM_PLANKS,1));

	/* a single slot is a normal click: put everything down */
	inventoryPress(30,false,false); inventoryRelease(30);
	CHECK(is(slot(30),ITEM_PLANKS,1) && emptyStack(inventoryHeld()));
	inventoryClose();
	survivalKill();
}

static void put(int c, u8 item, int count){ cell(c)->item=item; cell(c)->count=count; cell(c)->wear=0; }
static void clearGrid(void){ int i; for(i=0;i<CRAFT_CELLS;i++) put(i,0,0); }
static bool result(u8 item, int count){ stack_struct r; return craftingResult(&r) && r.item==item && r.count==count; }
static bool noResult(void){ stack_struct r; return !craftingResult(&r); }

static void testCrafting(void)
{
	int c;
	setup(true,false);
	CHECK(craftingRecipes()==21);

	/* 2x2 grid in the inventory */
	inventoryOpen(false);
	CHECK(inventoryGridSize()==2 && inventoryCellActive(4) && !inventoryCellActive(2) && !inventoryCellActive(6));
	for(c=0;c<CRAFT_CELLS;c++)
	{
		if(!inventoryCellActive(c)) continue;
		clearGrid(); put(c,ITEM_LOG,1);
		CHECK(result(ITEM_PLANKS,4));                 /* a log anywhere: 4 planks */
	}
	clearGrid(); put(0,29,1); CHECK(result(ITEM_PLANKS,4));          /* spruce log */
	clearGrid(); put(0,ITEM_PLANKS,1); put(3,ITEM_PLANKS,1); CHECK(result(ITEM_STICK,4));
	clearGrid(); put(1,ITEM_PLANKS,1); put(4,ITEM_PLANKS,1); CHECK(result(ITEM_STICK,4));
	clearGrid(); put(0,ITEM_PLANKS,1); put(1,ITEM_PLANKS,1); CHECK(noResult());   /* sideways: nothing */
	clearGrid(); put(0,ITEM_PLANKS,1); put(1,ITEM_PLANKS,1); put(3,ITEM_PLANKS,1); put(4,ITEM_PLANKS,1);
	CHECK(result(ITEM_CRAFTING_TABLE,1));
	put(4,ITEM_LOG,1); CHECK(noResult());
	clearGrid(); put(0,ITEM_LOG,1); put(4,ITEM_LOG,1); CHECK(noResult());

	/* taking the result uses one of each ingredient */
	clearGrid(); put(0,ITEM_LOG,3);
	inventoryClick(SLOT_RESULT,false,false);
	CHECK(is(inventoryHeld(),ITEM_PLANKS,4) && is(cell(0),ITEM_LOG,2));
	inventoryClick(SLOT_RESULT,false,false);
	CHECK(is(inventoryHeld(),ITEM_PLANKS,8) && is(cell(0),ITEM_LOG,1));
	inventoryClick(9,false,false);                    /* put the planks down */
	/* a different item on the stylus blocks the result */
	inventoryAdd(4,1,0); inventoryClick(0,false,false);
	inventoryClick(SLOT_RESULT,false,false);
	CHECK(is(cell(0),ITEM_LOG,1) && is(inventoryHeld(),4,1));
	inventoryClick(0,false,false);
	/* shift on the result crafts everything into the inventory */
	put(0,ITEM_LOG,5);
	inventoryClick(SLOT_RESULT,false,true);
	CHECK(emptyStack(cell(0)) && inventoryCount(ITEM_PLANKS)==8+20);
	CHECK(is(slot(9),ITEM_PLANKS,28));                /* shift-crafted: existing stack first */
	clearGrid(); put(0,ITEM_PLANKS,1); put(3,ITEM_PLANKS,1);
	inventoryClick(SLOT_RESULT,false,true);
	CHECK(is(slot(8),ITEM_STICK,4));                  /* then empty slots, hotbar from the right */
	/* the same tool on top of another does nothing */
	inventoryAdd(ITEM_WOOD_AXE,2,0);
	{
		int a=-1, b=-1, i;
		for(i=0;i<INV_SLOTS;i++) if(slot(i)->item==ITEM_WOOD_AXE){ if(a<0) a=i; else b=i; }
		slot(a)->wear=5;
		inventoryClick(a,false,false); inventoryClick(b,false,false);
		CHECK(is(inventoryHeld(),ITEM_WOOD_AXE,1) && inventoryHeld()->wear==5 && is(slot(b),ITEM_WOOD_AXE,1) && !slot(b)->wear);
		inventoryClick(a,false,false);
	}
	/* the 3x3 tools do not fit the inventory grid */
	clearGrid(); put(0,ITEM_PLANKS,1); put(1,ITEM_PLANKS,1); put(4,ITEM_STICK,1);
	CHECK(noResult());
	clearGrid();
	inventoryClose();

	/* crafting table: 3x3 */
	inventoryOpen(true);
	CHECK(inventoryIsTable() && inventoryGridSize()==3 && inventoryCellActive(8));
	clearGrid(); put(0,ITEM_PLANKS,1); put(1,ITEM_PLANKS,1); put(2,ITEM_PLANKS,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_WOOD_PICKAXE,1));
	put(1,ITEM_COBBLESTONE,1); CHECK(noResult());     /* mixed materials */
	clearGrid(); put(0,ITEM_COBBLESTONE,1); put(1,ITEM_COBBLESTONE,1); put(2,ITEM_COBBLESTONE,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_STONE_PICKAXE,1));
	for(c=0;c<3;c++)
	{
		clearGrid(); put(c,ITEM_PLANKS,1); put(c+3,ITEM_STICK,1); put(c+6,ITEM_STICK,1);
		CHECK(result(ITEM_WOOD_SHOVEL,1));            /* any column */
	}
	clearGrid(); put(1,ITEM_COBBLESTONE,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1); CHECK(result(ITEM_STONE_SHOVEL,1));
	clearGrid(); put(0,ITEM_PLANKS,1); put(1,ITEM_PLANKS,1); put(3,ITEM_PLANKS,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_WOOD_AXE,1));
	clearGrid(); put(1,ITEM_PLANKS,1); put(2,ITEM_PLANKS,1); put(5,ITEM_PLANKS,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_WOOD_AXE,1));                   /* mirrored */
	clearGrid(); put(0,ITEM_COBBLESTONE,1); put(1,ITEM_COBBLESTONE,1); put(3,ITEM_COBBLESTONE,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_STONE_AXE,1));
	clearGrid(); put(0,ITEM_STICK,1); put(2,ITEM_STICK,1); put(3,ITEM_STICK,1); put(4,ITEM_STICK,1); put(5,ITEM_STICK,1); put(6,ITEM_STICK,1); put(8,ITEM_STICK,1);
	CHECK(result(LADDERTYPE,2));
	clearGrid(); for(c=0;c<6;c++) put((c/2)*3+c%2,ITEM_PLANKS,1);
	CHECK(result(DOORTYPE,1));
	clearGrid(); for(c=0;c<6;c++) put((c/2)*3+c%2+1,ITEM_PLANKS,1);
	CHECK(result(DOORTYPE,1));                        /* shifted right */
	clearGrid(); put(4,ITEM_LOG,1); CHECK(result(ITEM_PLANKS,4));
	clearGrid(); put(0,ITEM_PLANKS,1); put(1,ITEM_PLANKS,1); put(3,ITEM_PLANKS,1); put(4,ITEM_PLANKS,1);
	CHECK(result(ITEM_CRAFTING_TABLE,1));
	put(8,ITEM_PLANKS,1); CHECK(noResult());          /* an extra item breaks the shape */

	/* a crafted tool goes to the stylus alone: tools do not stack */
	clearGrid(); put(0,ITEM_PLANKS,2); put(1,ITEM_PLANKS,2); put(2,ITEM_PLANKS,2); put(4,ITEM_STICK,2); put(7,ITEM_STICK,2);
	inventoryClick(SLOT_RESULT,false,false);
	CHECK(is(inventoryHeld(),ITEM_WOOD_PICKAXE,1));
	inventoryClick(SLOT_RESULT,false,false);
	CHECK(is(inventoryHeld(),ITEM_WOOD_PICKAXE,1) && is(cell(0),ITEM_PLANKS,1));
	inventoryClose();
	CHECK(inventoryCount(ITEM_WOOD_PICKAXE)==1 && inventoryCount(ITEM_PLANKS)>=28+3);
	survivalKill();
}

static void testMiningAndTools(void)
{
	int i;
	CHECK(survivalBreakTicks(2,0)==23);
	CHECK(survivalBreakTicks(3,0)==225 && survivalBreakTicks(3,ITEM_WOOD_PICKAXE)==34 && survivalBreakTicks(3,ITEM_STONE_PICKAXE)==17);
	CHECK(survivalBreakTicks(8,ITEM_WOOD_AXE)==45 && survivalBreakTicks(8,ITEM_STONE_AXE)==23);
	CHECK(survivalBreakTicks(ITEM_CRAFTING_TABLE,0)==113 && survivalBreakTicks(ITEM_CRAFTING_TABLE,ITEM_WOOD_AXE)==57);
	CHECK(survivalBreakTicks(5,ITEM_STONE_PICKAXE)==SURVIVAL_UNBREAKABLE);
	CHECK(!survivalCanHarvest(3,0) && survivalCanHarvest(3,ITEM_WOOD_PICKAXE));

	setup(true,false);
	cursorValid=true; worldBlock=3; testCursor=1234;

	/* the tool in hand is the selected hotbar slot */
	inventoryAdd(2,5,0);                              /* slot 0 */
	inventoryAdd(ITEM_WOOD_PICKAXE,1,0);              /* slot 1 */
	CHECK(survivalHeldTool()==0);
	inventorySelect(1);
	CHECK(survivalHeldTool()==ITEM_WOOD_PICKAXE);
	for(i=0;i<33;i++) CHECK(!survivalMine());
	CHECK(survivalMine());
	brk(3);
	CHECK(inventoryCount(4)==1 && slot(1)->wear==1);

	/* wear: the wooden pickaxe breaks after 60 blocks */
	for(i=0;i<58;i++) survivalBlockBroken(3,100,100,30);
	CHECK(dropsUsed()==1 && findDrop(4)->count==58 && slot(1)->wear==59);
	brk(3);
	CHECK(emptyStack(slot(1)) && survivalHeldTool()==0 && inventoryCount(4)==60);

	/* placing uses the selected slot */
	inventorySelect(0);
	CHECK(survivalCanPlace(2) && !survivalCanPlace(4));
	survivalConsume(2);
	CHECK(is(slot(0),2,4));
	for(i=0;i<4;i++) survivalConsume(2);
	CHECK(emptyStack(slot(0)) && !survivalCanPlace(2));
	survivalKill();
}

static void testMigration(void)
{
	u8* b=headerBuf+SURVIVAL_HEADER_OFFSET;
	u32 magic=SURVIVAL_MAGIC, sum;
	u16 wear=17;

	/* version 1: 64 counts, checksum at byte 80 */
	memset(headerBuf,0xCD,sizeof(headerBuf));
	map.header=(header_struct*)headerBuf;
	memcpy(b,&magic,4); b[4]=1; b[5]=15;
	memset(b+16,0,64); b[16+4]=70; b[16+2]=3;
	sum=checksumBytes(b,80); memcpy(b+80,&sum,4);
	setupInterface(); survivalInit(&map,&Player);
	CHECK(S()->version==SURVIVAL_VERSION && health()==15);
	CHECK(inventoryCount(4)==70 && inventoryCount(2)==3);
	CHECK(is(slot(0),2,3) && is(slot(1),4,64) && is(slot(2),4,6));
	survivalKill();

	/* version 2: 72 counts and the wear of one tool per kind, checksum at byte 100 */
	memset(headerBuf,0xCD,sizeof(headerBuf));
	memcpy(b,&magic,4); b[4]=2; b[5]=9;
	memset(b+16,0,84); b[16+ITEM_WOOD_PICKAXE]=2; b[16+ITEM_PLANKS]=99;
	memcpy(b+88,&wear,2);
	sum=checksumBytes(b,100); memcpy(b+100,&sum,4);
	setupInterface(); survivalInit(&map,&Player);
	CHECK(S()->version==SURVIVAL_VERSION && health()==9);
	CHECK(inventoryCount(ITEM_PLANKS)==99 && inventoryCount(ITEM_WOOD_PICKAXE)==2);
	{
		int i, worn=0, fresh=0;
		for(i=0;i<INV_SLOTS;i++) if(slot(i)->item==ITEM_WOOD_PICKAXE){ if(slot(i)->wear==17) worn++; else if(!slot(i)->wear) fresh++; }
		CHECK(worn==1 && fresh==1);
	}

	/* version 3 round trip, with items left in the grid and on the stylus */
	inventoryOpen(true);
	put(4,ITEM_STICK,3);
	inventoryClick(0,false,false);                    /* picks up whatever is in slot 0 */
	survivalWriteHeader(&map);
	survivalKill();
	setupInterface(); survivalInit(&map,&Player);
	CHECK(inventoryCount(ITEM_STICK)==3 && inventoryCount(ITEM_PLANKS)==99 && inventoryCount(ITEM_WOOD_PICKAXE)==2);
	CHECK(emptyStack(cell(4)) && emptyStack(inventoryHeld()) && !inventoryIsOpen());
	survivalKill();
}

static void testDrops(void)
{
	int i;
	setup(true,false);

	dropsSpawn(2,100,100,30);
	CHECK(dropsUsed()==1 && findDrop(2)->vz>0);
	Player.position=(vect3D){0,0,0};
	for(i=0;i<80;i++) dropsUpdate(&map,&Player);
	CHECK(findDrop(2)->z==20*DROP_UNIT+DROP_UNIT/2+DROP_HALF && findDrop(2)->vz==0);
	CHECK(abs(findDrop(2)->x-100*DROP_UNIT)<DROP_UNIT && abs(findDrop(2)->y-100*DROP_UNIT)<DROP_UNIT);

	/* at most five on the ground: the sixth removes the oldest */
	dropsClear();
	for(i=0;i<6;i++) dropsSpawn(14+i,10+i*10,100,25);
	CHECK(dropsUsed()==5 && !findDrop(14));
	for(i=1;i<6;i++) CHECK(findDrop(14+i)!=NULL);
	dropsSpawn(30,200,100,25);
	CHECK(dropsUsed()==5 && !findDrop(15) && findDrop(30));

	/* a free slot is used before anything is removed */
	standOn(findDrop(16)); for(i=0;i<DROP_PICKUP_DELAY;i++) dropsUpdate(&map,&Player);
	CHECK(!findDrop(16) && inventoryCount(16)==1 && dropsUsed()==4);
	dropsSpawn(31,220,100,25);
	CHECK(dropsUsed()==5 && findDrop(17) && findDrop(31));

	/* merging: same stackable item nearby */
	dropsClear();
	dropsSpawn(4,50,50,25); dropsSpawn(4,50,50,25); dropsSpawn(4,51,50,25);
	CHECK(dropsUsed()==1 && findDrop(4)->count==3);
	dropsSpawn(4,60,50,25);
	CHECK(dropsUsed()==2);

	/* pickup delay and sound */
	dropsClear();
	dropsSpawn(7,100,100,21);
	sfxPlayed=0;
	for(i=0;i<DROP_PICKUP_DELAY-1;i++){ standOn(findDrop(7)); dropsUpdate(&map,&Player); }
	CHECK(findDrop(7) && inventoryCount(7)==0);
	standOn(findDrop(7)); dropsUpdate(&map,&Player);
	CHECK(!findDrop(7) && inventoryCount(7)==1 && sfxPlayed==1);

	/* a full inventory picks up what fits */
	dropsClear();
	{
		int pl=-1;
		for(i=0;i<INV_SLOTS;i++) if(slot(i)->item==7) pl=i;
		CHECK(pl>=0);
		for(i=0;i<INV_SLOTS;i++) if(emptyStack(slot(i))) inventoryAdd(3,64,0);
		slot(pl)->count=62;                            /* room for 2 planks */
		for(i=0;i<5;i++) dropsSpawn(7,100,100,21);
		collect();
		CHECK(slot(pl)->count==64 && findDrop(7) && findDrop(7)->count==3);
	}

	/* thrown tools keep their wear and do not merge */
	{
		int k;
		for(k=0;k<INV_SLOTS;k++){ slot(k)->item=0; slot(k)->count=0; slot(k)->wear=0; }
	}
	dropsClear();
	dropsThrow(ITEM_STONE_AXE,2,40);
	CHECK(dropsUsed()==2 && findDrop(ITEM_STONE_AXE)->wear==40 && findDrop(ITEM_STONE_AXE)->delay==DROP_THROW_DELAY);
	collect();
	CHECK(inventoryCount(ITEM_STONE_AXE)==2 && slot(0)->wear==40);

	/* lifetime, pushing up, water, unloaded areas */
	dropsClear();
	dropsSpawn(2,100,100,21);
	Player.position=(vect3D){0,0,0};
	for(i=0;i<DROP_LIFETIME-1;i++) dropsUpdate(&map,&Player);
	CHECK(findDrop(2)!=NULL);
	dropsUpdate(&map,&Player);
	CHECK(!findDrop(2));
	dropsSpawn(2,100,100,21);
	for(i=0;i<60;i++) dropsUpdate(&map,&Player);
	solidI=100; solidJ=100; solidK=21;
	dropsUpdate(&map,&Player);
	CHECK(findDrop(2)->z==21*DROP_UNIT+DROP_UNIT/2+DROP_HALF);
	solidI=-1;
	dropsClear(); waterAbove=true;
	dropsSpawn(2,100,100,26);
	for(i=0;i<20;i++) dropsUpdate(&map,&Player);
	CHECK(findDrop(2)->vz>=-60 && findDrop(2)->z>20*DROP_UNIT+DROP_UNIT/2+DROP_HALF);
	for(i=0;i<400;i++) dropsUpdate(&map,&Player);
	CHECK(findDrop(2)->z==20*DROP_UNIT+DROP_UNIT/2+DROP_HALF);
	waterAbove=false;
	dropsClear();
	dropsSpawn(2,100,100,30);
	map.offset.x=40;
	for(i=0;i<30;i++) dropsUpdate(&map,&Player);
	CHECK(findDrop(2)->z==30*DROP_UNIT);
	map.offset.x=0;
	survivalKill();
}

static void testDisplay(void)
{
	setup(true,false);
	inventoryAdd(2,20,0);
	inventoryAdd(ITEM_WOOD_AXE,1,30);
	inventoryAdd(4,64*10,0);                          /* fills hotbar and main slots */

	/* closed: only the hotbar is shown, the selected item is in hand */
	survivalUpdateHUD(false);
	CHECK(oamSub.oamMemory[items[0].id].attribute[0]!=ATTR0_DISABLED);
	CHECK(oamSub.oamMemory[items[9].id].attribute[0]==ATTR0_DISABLED);
	CHECK(cursorBlock==2);
	/* tapping the hotbar selects a slot */
	thisXY.px=59+20*1+4; thisXY.py=170+4; keysDownValue=KEY_TOUCH;
	survivalUpdateHUD(false); keysDownValue=0;
	CHECK(inventorySelectedIndex()==1 && cursorBlock==ITEM_WOOD_AXE);

	/* open: main inventory and the 2x2 grid are shown */
	survivalUpdateHUD(true);
	CHECK(inventoryIsOpen() && !inventoryIsTable());
	CHECK(oamSub.oamMemory[items[9].id].attribute[0]!=ATTR0_DISABLED);
	CHECK(inventorySlotAt(135+5,52+5)==SLOT_GRID+0 && inventorySlotAt(153+5,70+5)==SLOT_GRID+4);
	CHECK(inventorySlotAt(191+5,62+5)==SLOT_RESULT && inventorySlotAt(55+5,168+5)==0 && inventorySlotAt(55+18*8+5,110+18*2+5)==35);
	CHECK(inventorySlotAt(20,100)==SLOT_OUTSIDE && inventorySlotAt(120,10)==SLOT_NONE);

	/* touch: tap a slot, drag to the grid; L held makes it a right click */
	thisXY.px=55+5; thisXY.py=168+5; keysDownValue=KEY_TOUCH; keysHeldValue=KEY_TOUCH|KEY_L;
	survivalUpdateHUD(true);
	keysDownValue=0;
	CHECK(is(inventoryHeld(),2,10) && is(slot(0),2,10));
	lastXY.px=135+5; lastXY.py=52+5; keysHeldValue=0; keysUpValue=KEY_TOUCH;
	survivalUpdateHUD(true); keysUpValue=0;
	CHECK(is(cell(0),2,1) && is(inventoryHeld(),2,9));   /* L: one item, the rest stays on the stylus */

	/* closing the window gives the grid and the stylus back */
	survivalUpdateHUD(false);
	CHECK(!inventoryIsOpen() && emptyStack(cell(0)) && emptyStack(inventoryHeld()) && inventoryCount(2)==20);
	survivalKill();
}

static void tap(int x, int y, bool open)
{
	thisXY.px=x; thisXY.py=y; keysDownValue=KEY_TOUCH; keysHeldValue=KEY_TOUCH;
	creativeUpdateUI(open);
	keysDownValue=0; keysHeldValue=0;
}
static void lift(int x, int y, bool open)
{
	lastXY.px=x; lastXY.py=y; keysUpValue=KEY_TOUCH;
	creativeUpdateUI(open);
	keysUpValue=0;
}
#define CELL(c) (55+((c)%9)*18+5), (110+((c)/9)*18+5)
#define BAR(s) (55+(s)*18+5), (168+5)

static void testCreative(void)
{
	int i, found=0;
	setup(false,false);
	creativeInit();
	CHECK(creativeCatalogueSize()==51);
	CHECK(cursorBlock==1 && creativeHotbar(0)==1);

	/* the catalogue scrolls a row at a time; three rows are on screen */
	CHECK(creativeScroll()==0);
	creativeUpdateUI(true);
	CHECK(oamSub.oamMemory[items[37].id].attribute[0]==ATTR0_DISABLED);   /* no up arrow at the top */
	CHECK(oamSub.oamMemory[items[38].id].attribute[0]!=ATTR0_DISABLED);
	tap(236+5,146+5,true);                                                /* down arrow */
	CHECK(creativeScroll()==1);
	tap(236+5,146+5,true);
	CHECK(creativeScroll()==2);
	tap(236+5,146+5,true);
	CHECK(creativeScroll()==3);                                           /* 51 blocks: 6 rows, 3 shown */
	CHECK(oamSub.oamMemory[items[38].id].attribute[0]==ATTR0_DISABLED);
	/* the crafting table is reachable: last block of the catalogue */
	for(i=0;i<27;i++)
	{
		u16 a2=oamSub.oamMemory[items[9+i].id].attribute[2]&1023;
		if(a2==survivalIconTile(ITEM_CRAFTING_TABLE)) found=1;
	}
	CHECK(found);

	/* dragging from the catalogue copies into the bar */
	tap(CELL(41-3*9),true);                                               /* block 42nd: the crafting table */
	lift(BAR(4),true);
	CHECK(creativeHotbar(4)==ITEM_CRAFTING_TABLE && cursorBlock==ITEM_CRAFTING_TABLE);
	tap(CELL(41-3*9),true); lift(BAR(5),true);
	CHECK(creativeHotbar(5)==ITEM_CRAFTING_TABLE);                        /* the catalogue never runs out */

	/* bar to bar swaps; bar to catalogue empties the slot */
	tap(BAR(0),true); lift(BAR(4),true);
	CHECK(creativeHotbar(0)==ITEM_CRAFTING_TABLE && creativeHotbar(4)==1);
	tap(BAR(4),true); lift(CELL(0),true);
	CHECK(creativeHotbar(4)==0 && cursorBlock==0);                         /* empty hand */
	creativeUpdateUI(true);
	CHECK(oamSub.oamMemory[items[4].id].attribute[0]==ATTR0_DISABLED);

	/* closed: tapping the bar selects, the catalogue and arrows are hidden */
	thisXY.px=59+20*0+5; thisXY.py=170+5; keysDownValue=KEY_TOUCH;
	creativeUpdateUI(false); keysDownValue=0;
	CHECK(cursorBlock==ITEM_CRAFTING_TABLE);
	CHECK(oamSub.oamMemory[items[9].id].attribute[0]==ATTR0_DISABLED && oamSub.oamMemory[items[38].id].attribute[0]==ATTR0_DISABLED);
	creativeSelect(1);
	CHECK(cursorBlock==3);
	survivalKill();
}

static void testVoid(void)
{
	int i;
	int32 deathZ=(-VOID_DEATH_DEPTH-64/2)*rTilesize2;   /* map.size.z is 64 */
	setup(true,false);

	/* between the bedrock and the death zone nothing happens */
	Player.position.z=deathZ+rTilesize2; Player.vector.z=0;
	tick(30);
	CHECK(health()==SURVIVAL_MAXHEALTH);

	/* in the death zone: 4 damage every tick (Minecraft Beta), so death comes fast */
	Player.position.z=deathZ-1;
	tick(1); CHECK(health()==SURVIVAL_MAXHEALTH-VOID_DAMAGE);
	tick(4); CHECK(health()==0 && survivalInputLocked());
	survivalKill();

	/* dropped items fall through the void and are destroyed in the death zone */
	setup(true,false);
	groundTop=-1;                                       /* no ground at all under this column */
	dropsSpawn(2,100,100,2);
	Player.position=(vect3D){0,0,0};
	for(i=0;i<40;i++) dropsUpdate(&map,&Player);
	CHECK(findDrop(2) && findDrop(2)->z<0);            /* below the world, still falling */
	for(i=0;i<400 && findDrop(2);i++) dropsUpdate(&map,&Player);
	CHECK(!findDrop(2));
	survivalKill();
}

static void testCoal(void)
{
	/* coal ore: hardness 3, needs a pickaxe, drops one coal */
	CHECK(survivalBreakTicks(ITEM_COAL_ORE,0)==450);
	CHECK(survivalBreakTicks(ITEM_COAL_ORE,ITEM_WOOD_PICKAXE)==68);
	CHECK(survivalBreakTicks(ITEM_COAL_ORE,ITEM_STONE_PICKAXE)==34);
	CHECK(!survivalCanHarvest(ITEM_COAL_ORE,0) && !survivalCanHarvest(ITEM_COAL_ORE,ITEM_WOOD_AXE));
	CHECK(survivalCanHarvest(ITEM_COAL_ORE,ITEM_WOOD_PICKAXE));
	CHECK(itemMaxStack(ITEM_COAL)==64);

	setup(true,false);
	brk(ITEM_COAL_ORE);
	CHECK(inventoryCount(ITEM_COAL)==0 && inventoryCount(ITEM_COAL_ORE)==0);   /* by hand: nothing */
	inventoryAdd(ITEM_WOOD_PICKAXE,1,0);
	inventorySelect(0);
	brk(ITEM_COAL_ORE);
	CHECK(inventoryCount(ITEM_COAL)==1 && inventoryCount(ITEM_COAL_ORE)==0);

	/* torch: coal on top of a stick, 4 torches, in the 2x2 grid too */
	inventoryOpen(false);
	put(0,ITEM_COAL,1); put(3,ITEM_STICK,1);
	CHECK(result(13,4));
	clearGrid(); put(1,ITEM_COAL,1); put(4,ITEM_STICK,1);
	CHECK(result(13,4));
	clearGrid(); put(0,ITEM_STICK,1); put(3,ITEM_COAL,1);
	CHECK(noResult());                                  /* upside down */
	clearGrid(); put(0,ITEM_COAL,1); put(1,ITEM_STICK,1);
	CHECK(noResult());                                  /* side by side */
	clearGrid();
	inventoryClose();
	inventoryOpen(true);
	put(2,ITEM_COAL,1); put(5,ITEM_STICK,1);
	CHECK(result(13,4));
	clearGrid();
	inventoryClose();
	survivalKill();

	/* creative: the coal ore block is in the catalogue */
	setup(false,false);
	creativeInit();
	{
		int i, found=0;
		creativeSetScroll(2);
		creativeUpdateUI(true);
		for(i=0;i<27;i++) if((oamSub.oamMemory[items[9+i].id].attribute[2]&1023)==survivalIconTile(ITEM_COAL_ORE)) found=1;
		CHECK(found);
	}
	survivalKill();
}

static void testChest(void)
{
	int n;
	u8 st, nst; int side;
	/* faces: a single chest shows its front on one side only */
	st=chestState(CHEST_FACE_NY,CHEST_SINGLE);
	CHECK(isChestBlock(st) && !isChestBlock(CHEST_STATE_FIRST+CHEST_STATES) && !isChestBlock(ITEM_CHEST));
	CHECK(chestTexture(st,0)==CHEST_TILE_TOP && chestTexture(st,1)==CHEST_TILE_TOP);
	CHECK(chestTexture(st,5)==CHEST_TILE_FRONT && chestTexture(st,4)==CHEST_TILE_SIDE && chestTexture(st,2)==CHEST_TILE_SIDE);
	/* a double chest along x: the two halves show opposite halves of the wide texture */
	CHECK(chestTexture(chestState(CHEST_FACE_NY,CHEST_PARTNER_POS),5)==CHEST_TILE_FRONT_R);
	CHECK(chestTexture(chestState(CHEST_FACE_NY,CHEST_PARTNER_NEG),5)==CHEST_TILE_FRONT_L);
	CHECK(chestTexture(chestState(CHEST_FACE_NY,CHEST_PARTNER_POS),4)==CHEST_TILE_BACK_L);
	CHECK(chestTexture(chestState(CHEST_FACE_NY,CHEST_PARTNER_NEG),4)==CHEST_TILE_BACK_R);
	CHECK(chestTexture(chestState(CHEST_FACE_NY,CHEST_PARTNER_POS),2)==CHEST_TILE_SIDE);
	/* along y */
	CHECK(chestTexture(chestState(CHEST_FACE_PX,CHEST_PARTNER_POS),2)==CHEST_TILE_FRONT_R);
	CHECK(chestTexture(chestState(CHEST_FACE_PX,CHEST_PARTNER_NEG),2)==CHEST_TILE_FRONT_L);
	CHECK(chestTexture(chestState(CHEST_FACE_NX,CHEST_PARTNER_POS),3)==CHEST_TILE_FRONT_L);
	/* the front faces the player (the same quadrants as doors) */
	CHECK(chestFacingFromLook(0)==CHEST_FACE_NY && chestFacingFromLook(8192)==CHEST_FACE_NX);
	CHECK(chestFacingFromLook(16384)==CHEST_FACE_PY && chestFacingFromLook(24576)==CHEST_FACE_PX);
	CHECK(chestFacingFromLook(-1)==CHEST_FACE_NY);

	/* placement rules */
	{
		u8 none[4]={0,0,0,0};
		u8 one[4]={chestState(CHEST_FACE_NY,CHEST_SINGLE),0,0,0};          /* single chest at +x */
		u8 turned[4]={chestState(CHEST_FACE_PX,CHEST_SINGLE),0,0,0};
		u8 two[4]={chestState(CHEST_FACE_NY,CHEST_SINGLE),chestState(CHEST_FACE_NY,CHEST_SINGLE),0,0};
		u8 dbl[4]={0,0,chestState(CHEST_FACE_PX,CHEST_PARTNER_POS),0};     /* half of a double chest at +y */
		CHECK(chestPlan(none,CHEST_FACE_PX,&nst,&side,&st) && nst==chestState(CHEST_FACE_PX,CHEST_SINGLE) && side==-1);
		CHECK(chestPlan(one,CHEST_FACE_PY,&nst,&side,&st) && side==0);
		CHECK(nst==chestState(CHEST_FACE_NY,CHEST_PARTNER_POS) && st==chestState(CHEST_FACE_NY,CHEST_PARTNER_NEG));
		CHECK(chestPlan(turned,CHEST_FACE_PY,&nst,&side,&st));                  /* turns to face the player */
		CHECK(nst==chestState(CHEST_FACE_PY,CHEST_PARTNER_POS) && st==chestState(CHEST_FACE_PY,CHEST_PARTNER_NEG));
		CHECK(chestPlan(turned,CHEST_FACE_NX,&nst,&side,&st) && chestFacing(nst)==CHEST_FACE_NY);
		CHECK(!chestPlan(two,CHEST_FACE_PY,&nst,&side,&st));                    /* between two chests */
		CHECK(!chestPlan(dbl,CHEST_FACE_PY,&nst,&side,&st));                    /* next to a double chest */
	}

	/* contents survive a save and a reload, in a file next to the world */
	{
		char p[64];
		chest_struct* c;
		chestSidecarPath("worlds/My World.map",p,sizeof(p));
		CHECK(!strcmp(p,"worlds/My World.chests"));
		chestsClear();
		c=chestAt(10,20,30,true);
		c->slots[5]=(stack_struct){ITEM_COAL,12,0};
		c->slots[26]=(stack_struct){ITEM_WOOD_AXE,1,7};
		chestAt(11,20,30,true);
		chestAt(12,20,30,true);
		chestRemove(11,20,30);
		CHECK(chestCount()==2 && !chestAt(11,20,30,false));
		CHECK(chestsSave("chesttest.map"));
		chestsClear();
		CHECK(chestCount()==0 && !chestAt(10,20,30,false));
		CHECK(chestsLoad("chesttest.map") && chestCount()==2);
		c=chestAt(10,20,30,false);
		CHECK(c && is(&c->slots[5],ITEM_COAL,12) && is(&c->slots[26],ITEM_WOOD_AXE,1) && c->slots[26].wear==7);
		CHECK(chestAt(12,20,30,false) && !chestAt(11,20,30,false));
		remove("chesttest.chests");
		CHECK(!chestsLoad("chesttest.map") && chestCount()==0);
		chestsClear();
	}

	/* mining: hardness 2.5 (axe), drops the chest item, and the recipe */
	CHECK(survivalBreakTicks(chestState(CHEST_FACE_PY,CHEST_SINGLE),0)==113);
	CHECK(survivalBreakTicks(chestState(CHEST_FACE_PY,CHEST_PARTNER_POS),ITEM_WOOD_AXE)==57);
	setup(true,false);
	brk(chestState(CHEST_FACE_PY,CHEST_PARTNER_NEG));
	CHECK(inventoryCount(ITEM_CHEST)==1);
	CHECK(itemMaxStack(ITEM_CHEST)==64 && survivalCanPlace(ITEM_CHEST));
	inventoryOpen(true);
	for(n=0;n<9;n++) put(n,ITEM_PLANKS,1);
	put(4,0,0);
	CHECK(result(ITEM_CHEST,1));
	put(4,ITEM_PLANKS,1);
	CHECK(noResult());
	clearGrid();
	inventoryClose();
	inventoryOpen(false);
	put(0,ITEM_PLANKS,1); put(1,ITEM_PLANKS,1); put(3,ITEM_PLANKS,1); put(4,ITEM_PLANKS,1);
	CHECK(result(ITEM_CRAFTING_TABLE,1));                  /* the 2x2 grid cannot make one */
	clearGrid();
	inventoryClose();
	survivalKill();

	/* the chest screen: clicks move stacks between the chest and the inventory */
	{
		chest_struct a, b;
		memset(&a,0,sizeof(a)); memset(&b,0,sizeof(b));
		setup(true,false);
		inventoryInitChestSprites(0);
		a.slots[0]=(stack_struct){4,20,0};
		inventoryOpenChest(&a,NULL);
		CHECK(inventoryIsChest() && inventoryChestPage()==0);
		CHECK(inventorySlot(SLOT_CHEST)==&a.slots[0] && inventorySlot(SLOT_CHEST+26)==&a.slots[26]);
		CHECK(!inventorySlot(SLOT_CHEST+27));                  /* single chest: 27 slots */
		CHECK(inventorySlotAt(55+1,41+1)==SLOT_CHEST && inventorySlotAt(55+18*8+1,41+36+1)==SLOT_CHEST+26);
		CHECK(inventorySlotAt(236+2,77+2)!=SLOT_PAGE_DOWN);    /* no pages */
		CHECK(inventorySlotAt(86+1,44+1)<SLOT_GRID || inventorySlotAt(86+1,44+1)>=SLOT_GRID+CRAFT_CELLS);
		inventoryClick(SLOT_CHEST,false,false);
		CHECK(is(inventoryHeld(),4,20) && emptyStack(&a.slots[0]));
		inventoryClick(9,false,false);
		CHECK(is(slot(9),4,20));
		/* shift: inventory -> chest (first free slot), chest -> inventory (item bar from the right) */
		a.slots[0]=(stack_struct){4,50,0};
		inventoryClick(9,false,true);
		CHECK(is(&a.slots[0],4,64) && is(&a.slots[1],4,6) && emptyStack(slot(9)));
		inventoryClick(SLOT_CHEST+1,false,true);
		CHECK(is(slot(8),4,6) && emptyStack(&a.slots[1]));
		/* dragging spreads over chest slots too */
		inventoryClick(SLOT_CHEST,false,false);               /* 64 on the stylus */
		inventoryPress(SLOT_CHEST+3,false,false);
		inventoryDragOver(SLOT_CHEST+4);
		inventoryDragOver(SLOT_CHEST+5);
		inventoryRelease(SLOT_CHEST+5);
		CHECK(is(&a.slots[3],4,21) && is(&a.slots[4],4,21) && is(&a.slots[5],4,21) && is(inventoryHeld(),4,1));
		inventoryClick(SLOT_CHEST+3,false,false);
		CHECK(is(&a.slots[3],4,22) && emptyStack(inventoryHeld()));
		/* the chest screen draws the chest's stacks */
		inventoryUpdateUI(true);
		CHECK(oamSub.oamMemory[3].attribute[0]!=ATTR0_DISABLED && oamSub.oamMemory[0].attribute[0]==ATTR0_DISABLED);
		inventoryClose();
		CHECK(!inventoryIsChest() && !inventorySlot(SLOT_CHEST));
		inventoryUpdateUI(false);
		CHECK(oamSub.oamMemory[3].attribute[0]==ATTR0_DISABLED);

		/* a double chest: 54 slots on two pages */
		memset(&a,0,sizeof(a));
		for(n=0;n<CHEST_SLOTS;n++) a.slots[n]=(stack_struct){2,64,0};
		inventoryOpenChest(&a,&b);
		CHECK(inventorySlot(SLOT_CHEST+27)==&b.slots[0] && inventorySlot(SLOT_CHEST+53)==&b.slots[26] && !inventorySlot(SLOT_CHEST+54));
		inventoryAdd(2,10,0);                                   /* slot 0 */
		inventoryClick(0,false,true);                           /* the first half is full: goes to the second */
		CHECK(is(&b.slots[0],2,10) && emptyStack(slot(0)));
		CHECK(inventorySlotAt(236+2,77+2)==SLOT_PAGE_DOWN);
		CHECK(inventorySlotAt(55+1,41+1)==SLOT_CHEST);
		inventoryPress(SLOT_PAGE_DOWN,false,false); inventoryRelease(SLOT_PAGE_DOWN);
		CHECK(inventoryChestPage()==1 && inventorySlotAt(55+1,41+1)==SLOT_CHEST+27);
		CHECK(inventorySlotAt(236+2,41+2)==SLOT_PAGE_UP);
		inventoryClick(SLOT_CHEST+27,false,false);
		CHECK(is(inventoryHeld(),2,10));
		inventoryPress(SLOT_PAGE_UP,false,false); inventoryRelease(SLOT_PAGE_UP);
		CHECK(inventoryChestPage()==0 && is(inventoryHeld(),2,10));   /* the stylus keeps its stack */
		inventoryClose();                                       /* the held stack goes back to the inventory */
		CHECK(inventoryCount(2)==10);
		inventoryOpenChest(&a,&b);
		CHECK(inventoryChestPage()==0);
		inventoryClose();
		survivalKill();
	}

	/* a broken chest's stacks fall out whole */
	setup(true,false);
	dropsSpawnStack(ITEM_STONE_AXE,1,33,100,100,30);
	dropsSpawnStack(4,40,0,100,100,30);
	CHECK(findDrop(ITEM_STONE_AXE) && findDrop(ITEM_STONE_AXE)->wear==33 && findDrop(4) && findDrop(4)->count==40);
	collect();
	CHECK(inventoryCount(4)==40 && inventoryCount(ITEM_STONE_AXE)==1);
	survivalKill();

	/* creative: the chest is in the catalogue */
	setup(false,false);
	creativeInit();
	{
		int i, s, found=0;
		for(s=0;s<4;s++)
		{
			creativeSetScroll(s);
			creativeUpdateUI(true);
			for(i=0;i<27;i++) if((oamSub.oamMemory[items[9+i].id].attribute[2]&1023)==survivalIconTile(ITEM_CHEST)) found=1;
		}
		CHECK(found);
	}
	survivalKill();
}

static furnace_struct* newFurnace(furnace_struct* f, u8 in, int inCount, u8 fuel, int fuelCount)
{
	memset(f,0,sizeof(*f));
	f->used=1;
	f->slots[FURNACE_IN]=(stack_struct){in,inCount,0};
	f->slots[FURNACE_FUEL]=(stack_struct){fuel,fuelCount,0};
	return f;
}
static int ticks(furnace_struct* f, int n){ int c=0; while(n--) if(furnaceTick(f)) c++; return c; }

static void testFurnace(void)
{
	furnace_struct f;
	int n, changes;

	/* faces: the front (lit or not) on one side, the top on top and bottom */
	CHECK(isFurnaceBlock(furnaceState(CHEST_FACE_NY,false)) && isFurnaceBlock(furnaceState(CHEST_FACE_PX,true)));
	CHECK(!isFurnaceBlock(ITEM_FURNACE) && !isFurnaceBlock(FURNACE_STATE_FIRST+FURNACE_STATES) && !isChestBlock(FURNACE_STATE_FIRST));
	CHECK(isLitFurnace(furnaceState(CHEST_FACE_NY,true)) && !isLitFurnace(furnaceState(CHEST_FACE_NY,false)));
	CHECK(furnaceFacing(furnaceState(CHEST_FACE_PY,true))==CHEST_FACE_PY);
	CHECK(furnaceTexture(furnaceState(CHEST_FACE_NY,false),5)==FURNACE_TILE_FRONT);
	CHECK(furnaceTexture(furnaceState(CHEST_FACE_NY,true),5)==FURNACE_TILE_FRONT_LIT);
	CHECK(furnaceTexture(furnaceState(CHEST_FACE_NY,true),4)==FURNACE_TILE_SIDE && furnaceTexture(furnaceState(CHEST_FACE_NY,true),2)==FURNACE_TILE_SIDE);
	CHECK(furnaceTexture(furnaceState(CHEST_FACE_PX,false),2)==FURNACE_TILE_FRONT);
	CHECK(furnaceTexture(furnaceState(CHEST_FACE_PX,false),0)==FURNACE_TILE_TOP && furnaceTexture(furnaceState(CHEST_FACE_PX,false),1)==FURNACE_TILE_TOP);

	/* Beta 1.7's fuel and smelting lists */
	CHECK(furnaceFuelTime(ITEM_PLANKS)==300 && furnaceFuelTime(ITEM_LOG)==300 && furnaceFuelTime(9)==300 && furnaceFuelTime(29)==300 && furnaceFuelTime(30)==300);
	CHECK(furnaceFuelTime(ITEM_CRAFTING_TABLE)==300 && furnaceFuelTime(ITEM_CHEST)==300);
	CHECK(furnaceFuelTime(ITEM_STICK)==100 && furnaceFuelTime(ITEM_COAL)==1600 && furnaceFuelTime(ITEM_CHARCOAL)==1600);
	CHECK(!furnaceFuelTime(ITEM_COBBLESTONE) && !furnaceFuelTime(ITEM_WOOD_PICKAXE) && !furnaceFuelTime(DOORTYPE) && !furnaceFuelTime(LADDERTYPE) && !furnaceFuelTime(0));
	CHECK(furnaceSmeltResult(ITEM_COBBLESTONE)==3 && furnaceSmeltResult(6)==12);
	CHECK(furnaceSmeltResult(ITEM_LOG)==ITEM_CHARCOAL && furnaceSmeltResult(29)==ITEM_CHARCOAL && furnaceSmeltResult(9)==ITEM_CHARCOAL);
	CHECK(!furnaceSmeltResult(3) && !furnaceSmeltResult(ITEM_COAL_ORE) && !furnaceSmeltResult(ITEM_PLANKS));

	/* one coal smelts 8 items: 1600 ticks, 200 each */
	newFurnace(&f,ITEM_COBBLESTONE,10,ITEM_COAL,2);
	CHECK(furnaceTick(&f) && furnaceBurning(&f) && f.burnTime==1600 && f.burnMax==1600 && is(&f.slots[FURNACE_FUEL],ITEM_COAL,1));
	CHECK(f.cookTime==1);
	changes=ticks(&f,198);
	CHECK(!changes && emptyStack(&f.slots[FURNACE_OUT]) && f.cookTime==199);
	ticks(&f,1);
	CHECK(is(&f.slots[FURNACE_OUT],3,1) && is(&f.slots[FURNACE_IN],ITEM_COBBLESTONE,9) && f.cookTime==0);
	CHECK(furnaceCookScaled(&f,24)==0);
	ticks(&f,100);
	CHECK(furnaceCookScaled(&f,24)==12 && furnaceBurnScaled(&f,12)==(1600-300)*12/1600);
	ticks(&f,1600-300);
	CHECK(is(&f.slots[FURNACE_OUT],3,8) && furnaceBurning(&f) && is(&f.slots[FURNACE_FUEL],ITEM_COAL,1));
	/* the next coal is lit only when the first one is spent: the fire never goes out */
	CHECK(!furnaceTick(&f) && f.burnTime==1600 && emptyStack(&f.slots[FURNACE_FUEL]) && f.cookTime==1);

	/* planks: 1.5 items, the half-done one is lost when the fire goes out */
	newFurnace(&f,ITEM_COBBLESTONE,5,ITEM_PLANKS,1);
	changes=ticks(&f,300);
	CHECK(changes==1 && furnaceBurning(&f) && is(&f.slots[FURNACE_OUT],3,1) && f.cookTime==100);
	CHECK(furnaceTick(&f) && !furnaceBurning(&f) && f.cookTime==0 && is(&f.slots[FURNACE_IN],ITEM_COBBLESTONE,4));
	CHECK(furnaceBurnScaled(&f,12)==0);

	/* no fuel is used while there is nothing to smelt */
	newFurnace(&f,0,0,ITEM_COAL,3);
	CHECK(!ticks(&f,50) && is(&f.slots[FURNACE_FUEL],ITEM_COAL,3));
	newFurnace(&f,3,5,ITEM_COAL,3);                       /* stone cannot be smelted */
	CHECK(!ticks(&f,50) && is(&f.slots[FURNACE_FUEL],ITEM_COAL,3));
	newFurnace(&f,ITEM_COBBLESTONE,5,ITEM_COBBLESTONE,3);  /* not fuel */
	CHECK(!ticks(&f,50) && !furnaceBurning(&f) && f.cookTime==0);
	newFurnace(&f,ITEM_COBBLESTONE,5,ITEM_COAL,3);
	f.slots[FURNACE_OUT]=(stack_struct){12,1,0};           /* another item in the output */
	CHECK(!ticks(&f,50) && is(&f.slots[FURNACE_FUEL],ITEM_COAL,3));

	/* a full output stops the smelting, but the fuel already lit burns on */
	newFurnace(&f,ITEM_COBBLESTONE,5,ITEM_STICK,4);
	f.slots[FURNACE_OUT]=(stack_struct){3,63,0};
	ticks(&f,100);
	CHECK(!is(&f.slots[FURNACE_OUT],3,64) && furnaceBurning(&f) && is(&f.slots[FURNACE_FUEL],ITEM_STICK,3));
	newFurnace(&f,ITEM_COBBLESTONE,5,ITEM_COAL,1);
	f.slots[FURNACE_OUT]=(stack_struct){3,63,0};
	ticks(&f,400);
	CHECK(is(&f.slots[FURNACE_OUT],3,64) && is(&f.slots[FURNACE_IN],ITEM_COBBLESTONE,4) && f.cookTime==0 && furnaceBurning(&f));

	/* logs give charcoal, which is fuel too and makes torches */
	newFurnace(&f,ITEM_LOG,2,ITEM_PLANKS,2);
	ticks(&f,400);
	CHECK(is(&f.slots[FURNACE_OUT],ITEM_CHARCOAL,2) && emptyStack(&f.slots[FURNACE_IN]));
	CHECK(itemMaxStack(ITEM_CHARCOAL)==64 && itemMaxStack(ITEM_FURNACE)==64);

	/* contents and timers survive a save and a reload */
	{
		char p[64];
		furnace_struct* g;
		furnaceSidecarPath("worlds/My World.map",p,sizeof(p));
		CHECK(!strcmp(p,"worlds/My World.furnaces"));
		furnacesClear();
		remove("furnacetest.furnaces");
		CHECK(furnacesSave("furnacetest.map"));                  /* never had one: no file */
		CHECK(!furnacesLoad("furnacetest.map"));
		g=furnaceAt(5,6,7,true);
		*g=*newFurnace(&f,ITEM_COBBLESTONE,3,ITEM_COAL,2);
		g->i=5; g->j=6; g->k=7;
		ticks(g,250);
		furnaceAt(8,6,7,true);
		furnaceRemove(8,6,7);
		CHECK(furnaceCount()==1 && furnaceRecords()==2 && furnaceGet(0)==g && !furnaceGet(1));
		CHECK(furnacesSave("furnacetest.map"));
		furnacesClear();
		CHECK(furnacesLoad("furnacetest.map") && furnaceCount()==1);
		g=furnaceAt(5,6,7,false);
		CHECK(g && is(&g->slots[FURNACE_OUT],3,1) && is(&g->slots[FURNACE_IN],ITEM_COBBLESTONE,2) && is(&g->slots[FURNACE_FUEL],ITEM_COAL,1));
		CHECK(g->burnTime==1600-249 && g->burnMax==1600 && g->cookTime==50);
		furnaceRemove(5,6,7);
		CHECK(furnacesSave("furnacetest.map") && furnacesLoad("furnacetest.map") && furnaceCount()==0);
		remove("furnacetest.furnaces");
		furnacesClear();
	}

	/* mining: hardness 3.5, a pickaxe to drop it; the recipe needs a crafting table */
	n=furnaceState(CHEST_FACE_PY,true);
	CHECK(survivalBreakTicks(n,0)==525 && survivalBreakTicks(n,ITEM_WOOD_PICKAXE)==79 && survivalBreakTicks(n,ITEM_STONE_PICKAXE)==40);
	CHECK(!survivalCanHarvest(n,0) && !survivalCanHarvest(n,ITEM_WOOD_AXE) && survivalCanHarvest(n,ITEM_WOOD_PICKAXE));
	setup(true,false);
	brk(furnaceState(CHEST_FACE_PY,false));
	CHECK(inventoryCount(ITEM_FURNACE)==0);
	inventoryAdd(ITEM_WOOD_PICKAXE,1,0);
	inventorySelect(0);
	brk(furnaceState(CHEST_FACE_PY,true));
	CHECK(inventoryCount(ITEM_FURNACE)==1);
	CHECK(is(slot(1),ITEM_FURNACE,1) && !survivalCanPlace(ITEM_FURNACE));
	inventorySelect(1);
	CHECK(survivalCanPlace(ITEM_FURNACE) && !survivalCanPlace(ITEM_CHARCOAL));
	inventoryOpen(true);
	for(n=0;n<9;n++) put(n,ITEM_COBBLESTONE,1);
	put(4,0,0);
	CHECK(result(ITEM_FURNACE,1));
	put(4,ITEM_COBBLESTONE,1);
	CHECK(noResult());
	clearGrid();
	put(1,ITEM_CHARCOAL,1); put(4,ITEM_STICK,1);
	CHECK(result(13,4));                                  /* torches from charcoal */
	clearGrid();
	inventoryClose();
	survivalKill();

	/* the furnace screen */
	{
		u16 png[256*32*4/2];
		unsigned char* rgba=(unsigned char*)png;
		setup(true,false);
		inventoryInitChestSprites(0);
		newFurnace(&f,0,0,0,0);
		inventoryOpenFurnace(&f);
		CHECK(inventoryIsFurnace() && !inventoryIsChest());
		CHECK(inventorySlot(SLOT_FURNACE+FURNACE_IN)==&f.slots[FURNACE_IN] && inventorySlot(SLOT_FURNACE+FURNACE_OUT)==&f.slots[FURNACE_OUT]);
		CHECK(!inventorySlot(SLOT_CHEST));
		CHECK(inventorySlotAt(103+1,43+1)==SLOT_FURNACE+FURNACE_IN && inventorySlotAt(103+15,79+15)==SLOT_FURNACE+FURNACE_FUEL);
		CHECK(inventorySlotAt(163+8,61+8)==SLOT_FURNACE+FURNACE_OUT);
		CHECK(inventorySlotAt(55+1,41+1)==SLOT_NONE);                 /* no chest cells, no crafting grid */
		CHECK(inventorySlotAt(135+1,52+1)!=SLOT_GRID);

		/* put things in */
		inventoryAdd(ITEM_COBBLESTONE,20,0);                          /* slot 0 */
		inventoryAdd(ITEM_COAL,5,0);                                  /* slot 1 */
		inventoryClick(0,false,false);
		inventoryClick(SLOT_FURNACE+FURNACE_IN,false,false);
		inventoryClick(1,false,false);
		inventoryClick(SLOT_FURNACE+FURNACE_FUEL,true,false);         /* one coal */
		CHECK(is(&f.slots[FURNACE_IN],ITEM_COBBLESTONE,20) && is(&f.slots[FURNACE_FUEL],ITEM_COAL,1) && is(inventoryHeld(),ITEM_COAL,4));
		/* nothing can be put into the output */
		inventoryClick(SLOT_FURNACE+FURNACE_OUT,false,false);
		CHECK(emptyStack(&f.slots[FURNACE_OUT]) && is(inventoryHeld(),ITEM_COAL,4));
		inventoryClick(1,false,false);
		CHECK(emptyStack(inventoryHeld()));

		/* it runs with the screen open: the flame and arrow are drawn */
		ticks(&f,300);
		CHECK(is(&f.slots[FURNACE_OUT],3,1) && f.cookTime==100);
		inventoryUpdateUI(true);
		CHECK(oamSub.oamMemory[0].attribute[0]!=ATTR0_DISABLED && oamSub.oamMemory[2].attribute[0]!=ATTR0_DISABLED);
		CHECK(oamSub.oamMemory[3].attribute[0]!=ATTR0_DISABLED && oamSub.oamMemory[4].attribute[0]!=ATTR0_DISABLED && oamSub.oamMemory[5].attribute[0]!=ATTR0_DISABLED);
		CHECK(oamSub.oamMemory[6].attribute[0]==ATTR0_DISABLED);       /* chest sprites stay hidden */
		/* arrow: 12 of 24 columns (+1); flame: 12*1300/1600 rows */
		CHECK(*survivalIconPixel(76,12,8)!=0 && *survivalIconPixel(76,13,8)==0 && *survivalIconPixel(77,0,8)==0);

		/* the output: take it, or add all of it to a held stack of the same item */
		f.slots[FURNACE_OUT]=(stack_struct){3,10,0};
		inventoryClick(SLOT_FURNACE+FURNACE_OUT,true,false);         /* half */
		CHECK(is(inventoryHeld(),3,5) && is(&f.slots[FURNACE_OUT],3,5));
		inventoryClick(SLOT_FURNACE+FURNACE_OUT,false,false);        /* all of it onto the stylus */
		CHECK(is(inventoryHeld(),3,10) && emptyStack(&f.slots[FURNACE_OUT]));
		f.slots[FURNACE_OUT]=(stack_struct){3,60,0};
		inventoryClick(SLOT_FURNACE+FURNACE_OUT,false,false);        /* would not fit: nothing */
		CHECK(is(inventoryHeld(),3,10) && is(&f.slots[FURNACE_OUT],3,60));
		inventoryClick(5,false,false);
		/* shift: output to the item bar from the right, input back to the main inventory */
		inventoryClick(SLOT_FURNACE+FURNACE_OUT,false,true);
		CHECK(is(slot(5),3,64) && is(slot(8),3,6) && emptyStack(&f.slots[FURNACE_OUT]));   /* merges first */
		inventoryClick(SLOT_FURNACE+FURNACE_IN,false,true);
		CHECK(is(slot(9),ITEM_COBBLESTONE,19) && emptyStack(&f.slots[FURNACE_IN]));
		/* from the inventory, shift moves between bar and inventory, never into the furnace */
		inventoryClick(9,false,true);
		CHECK(is(slot(0),ITEM_COBBLESTONE,19) && emptyStack(&f.slots[FURNACE_IN]));
		/* dragging spreads over input and fuel, not the output */
		inventoryClick(0,false,false);
		inventoryPress(SLOT_FURNACE+FURNACE_IN,true,false);
		inventoryDragOver(SLOT_FURNACE+FURNACE_OUT);
		inventoryDragOver(SLOT_FURNACE+FURNACE_FUEL);
		inventoryRelease(SLOT_FURNACE+FURNACE_FUEL);
		CHECK(is(&f.slots[FURNACE_IN],ITEM_COBBLESTONE,1) && is(&f.slots[FURNACE_FUEL],ITEM_COBBLESTONE,1));   /* L: one each */
		CHECK(emptyStack(&f.slots[FURNACE_OUT]) && is(inventoryHeld(),ITEM_COBBLESTONE,17));
		inventoryClose();
		CHECK(!inventoryIsFurnace() && !inventorySlot(SLOT_FURNACE) && inventoryCount(ITEM_COBBLESTONE)==17);
		inventoryUpdateUI(false);
		CHECK(oamSub.oamMemory[3].attribute[0]==ATTR0_DISABLED && oamSub.oamMemory[0].attribute[0]==ATTR0_DISABLED);

		/* the pack's furnace.png gives the flame and the arrow */
		memset(png,0,sizeof(png));
		rgba[(176+3+5*256)*4+0]=255; rgba[(176+3+5*256)*4+3]=255;                  /* flame pixel 3,5: red */
		rgba[(176+20+(14+7)*256)*4+2]=255; rgba[(176+20+(14+7)*256)*4+3]=255;      /* arrow pixel 20,7: blue */
		inventoryLoadFurnaceGui(rgba,256,32);
		newFurnace(&f,ITEM_COBBLESTONE,2,ITEM_COAL,1);
		ticks(&f,190);
		inventoryOpenFurnace(&f);
		inventoryUpdateUI(true);
		CHECK(*survivalIconPixel(75,3,5)==(RGB15(31,0,0)|BIT(15)));
		CHECK(*survivalIconPixel(77,4,7)==(RGB15(0,0,31)|BIT(15)));
		CHECK(*survivalIconPixel(75,0,0)==0);
		inventoryClose();
		survivalKill();
	}

	/* a broken furnace's contents fall out: tested with the chest's dropsSpawnStack */

	/* creative: the furnace is in the catalogue */
	setup(false,false);
	creativeInit();
	{
		int i, s, found=0;
		for(s=0;s<4;s++)
		{
			creativeSetScroll(s);
			creativeUpdateUI(true);
			for(i=0;i<27;i++) if((oamSub.oamMemory[items[9+i].id].attribute[2]&1023)==survivalIconTile(ITEM_FURNACE)) found=1;
		}
		CHECK(found);
	}
	survivalKill();
}

/* a small world for the plant rules: 32x32x32, k up */
static u8 pw[32][32][32];
static int pwGet(void* ctx, int i, int j, int k){ (void)ctx; if(i<0 || j<0 || k<0 || i>=32 || j>=32 || k>=32) return -1; return pw[i][j][k]; }
static plantWorld_struct pwView(void){ plantWorld_struct w={pwGet,NULL,32}; return w; }
static void pwFlat(int ground){ int i, j, k; memset(pw,0,sizeof(pw)); for(i=0;i<32;i++)for(j=0;j<32;j++){ for(k=0;k<ground;k++) pw[i][j][k]=3; pw[i][j][ground]=1; } }
static int boxCount(treeBox_struct* b, u8 t){ int n, c=0; for(n=0;n<TREE_BOX_SIZE;n++) if(b->cells[n]==t) c++; return c; }
static u8 boxAt(treeBox_struct* b, int i, int j, int k){ u8* c=treeBoxCell(b,i,j,k); return c?*c:0; }

static treeBox_struct tb, tb2;

static void testPlants(void)
{
	plantWorld_struct w=pwView();
	int n, k, s, l;

	CHECK(isLeaves(LEAVES_BLOCK) && isLeaves(LEAVES_DECAY) && isLeaves(LEAVES_PLACED) && !isLeaves(ITEM_SAPLING));
	CHECK(isSapling(ITEM_SAPLING) && isSapling(SAPLING_GROWN) && !isSapling(ITEM_APPLE));
	CHECK(!solid(ITEM_SAPLING) && !solid(SAPLING_GROWN) && solid(LEAVES_DECAY) && solid(LEAVES_PLACED));

	/* leaves stay up to 4 steps from a log through leaves (any kind, also placed ones) */
	memset(pw,0,sizeof(pw));
	pw[10][10][10]=ITEM_LOG;
	for(n=1;n<=5;n++) pw[10+n][10][10]=(n==2)?LEAVES_PLACED:LEAVES_BLOCK;
	CHECK(plantsLeafSupported(&w,11,10,10) && plantsLeafSupported(&w,14,10,10));
	CHECK(!plantsLeafSupported(&w,15,10,10));                           /* 5 steps */
	pw[12][10][10]=0;                                                  /* the chain is cut */
	CHECK(plantsLeafSupported(&w,11,10,10) && !plantsLeafSupported(&w,13,10,10));
	pw[12][10][10]=LEAVES_BLOCK;
	pw[10][10][10]=29;                                                 /* any log */
	CHECK(plantsLeafSupported(&w,14,10,10));
	memset(pw,0,sizeof(pw));
	pw[10][10][10]=ITEM_LOG; pw[11][11][10]=LEAVES_BLOCK;              /* only a corner touches */
	CHECK(!plantsLeafSupported(&w,11,11,10));
	pw[10][10][11]=LEAVES_BLOCK; pw[10][10][12]=LEAVES_BLOCK;          /* up and around */
	pw[10][11][12]=LEAVES_BLOCK; pw[10][11][11]=LEAVES_BLOCK;
	CHECK(plantsLeafSupported(&w,10,11,11));

	/* time of day: Minecraft's sky darkness */
	CHECK(plantsSkyDarkness(8192)==0 && plantsSkyDarkness(24576)==11);
	CHECK(plantsSkyDarkness(0)==0 && plantsSkyDarkness(31000)>0 && plantsSkyDarkness(31000)<11);   /* dawn */
	CHECK(plantsSkyDarkness(4096)==0 && plantsSkyDarkness(20480)==11);

	/* light: the sky through leaves, nothing under stone, torches and lit furnaces */
	pwFlat(5);
	CHECK(plantsLight(&w,10,10,6,0)==15 && plantsLight(&w,10,10,6,11)==4 && plantsSeesSky(&w,10,10,6));
	pw[10][10][20]=LEAVES_BLOCK; pw[10][10][21]=LEAVES_BLOCK;
	CHECK(plantsLight(&w,10,10,6,0)==13 && !plantsSeesSky(&w,10,10,6));
	pw[10][10][25]=3;
	CHECK(plantsLight(&w,10,10,6,0)==0);
	pw[12][11][6]=13;                                                  /* a torch 3 away */
	CHECK(plantsLight(&w,10,10,6,0)==11);
	pw[12][11][6]=furnaceState(CHEST_FACE_PX,true);
	CHECK(plantsLight(&w,10,10,6,0)==10);
	pw[12][11][6]=furnaceState(CHEST_FACE_PX,false);
	CHECK(plantsLight(&w,10,10,6,0)==0);

	/* a sapling stays on grass or dirt, with the sky or light 8 */
	pwFlat(5);
	CHECK(plantsSaplingStays(&w,10,10,6));
	pw[10][10][5]=2; CHECK(plantsSaplingStays(&w,10,10,6));
	pw[10][10][5]=3; CHECK(!plantsSaplingStays(&w,10,10,6));         /* on stone */
	pw[10][10][5]=1;
	for(n=0;n<32;n++) for(s=0;s<32;s++) pw[n][s][20]=3;                /* a cave roof */
	CHECK(!plantsSaplingStays(&w,10,10,6));
	pw[10][14][6]=13;                                                  /* torch 4 away: light 10 */
	CHECK(plantsSaplingStays(&w,10,10,6));
	pw[10][14][6]=0; pw[10][17][6]=13;                                 /* 7 away: 7 */
	CHECK(!plantsSaplingStays(&w,10,10,6));

	/* the small oak (WorldGenTrees) */
	pwFlat(5);
	pw[10][10][6]=ITEM_SAPLING;
	for(s=1;s<200;s++)
	{
		CHECK(plantsGrowSmallTree(&w,s,10,10,6,&tb));
		l=0; while(boxAt(&tb,10,10,6+l)==ITEM_LOG) l++;
		if(l<4 || l>6){ CHECK(l>=4 && l<=6); break; }
		if(boxCount(&tb,ITEM_LOG)!=l || boxAt(&tb,10,10,5)!=2){ CHECK(0); break; }
		/* top layer: a plus; the trunk's top has leaves; two-wide layers below */
		if(boxAt(&tb,10,10,6+l)!=LEAVES_BLOCK || boxAt(&tb,11,11,6+l) || boxAt(&tb,11,10,6+l)!=LEAVES_BLOCK){ CHECK(0); break; }
		if(boxAt(&tb,12,10,6+l-3)!=LEAVES_BLOCK || boxAt(&tb,13,10,6+l-3) || boxAt(&tb,10,10,6+l+1)){ CHECK(0); break; }
		k=boxCount(&tb,LEAVES_BLOCK);
		if(k<20+20+4+5 || k>24+24+8+5){ CHECK(k>=49 && k<=61); break; }   /* corners: half of them */
	}
	CHECK(plantsGrowSmallTree(&w,7,10,10,6,&tb) && plantsGrowSmallTree(&w,7,10,10,6,&tb2) && !memcmp(tb.cells,tb2.cells,sizeof(tb.cells)));
	pw[11][10][9]=3;                                                   /* no room */
	CHECK(!plantsGrowSmallTree(&w,7,10,10,6,&tb));
	pw[11][10][9]=LEAVES_BLOCK;                                        /* leaves are room */
	CHECK(plantsGrowSmallTree(&w,7,10,10,6,&tb));
	pw[11][10][9]=0;
	pw[10][10][5]=3;                                                   /* not on stone */
	CHECK(!plantsGrowSmallTree(&w,7,10,10,6,&tb));
	pwFlat(27);                                                        /* too close to the top */
	CHECK(!plantsGrowSmallTree(&w,7,10,10,28,&tb));

	/* the big oak (WorldGenBigTree): a trunk, branches and clusters of leaves */
	pwFlat(3);
	for(s=1;s<60;s++)
	{
		if(!plantsGrowBigTree(&w,s,16,16,4,&tb)){ CHECK(0); break; }
		l=0; while(boxAt(&tb,16,16,4+l)==ITEM_LOG) l++;
		if(l<3 || boxCount(&tb,LEAVES_BLOCK)<20 || boxAt(&tb,16,16,3)){ CHECK(l>=3 && boxCount(&tb,LEAVES_BLOCK)>=20); break; }
		for(n=0;n<TREE_BOX_SIZE;n++)
		{
			int z=n/((2*TREE_BOX_R+1)*(2*TREE_BOX_R+1));
			if(tb.cells[n] && (z==0 || z>=TREE_BOX_H-1)){ CHECK(0); break; }       /* nothing below the base or at the box edge */
		}
	}
	pw[16][16][9]=3;                                                   /* a low ceiling: no room for a big oak */
	CHECK(!plantsGrowBigTree(&w,3,16,16,4,&tb));
	pw[16][16][9]=0;
	pw[16][16][3]=3;
	CHECK(!plantsGrowBigTree(&w,3,16,16,4,&tb));

	/* broken leaves: a sapling one time in 20, an apple one time in 200 */
	{
		int sap=0, app=0;
		u8 it[2];
		srand(12345);
		for(n=0;n<40000;n++)
		{
			int c=plantsLeafDrops(it);
			for(s=0;s<c;s++){ if(it[s]==ITEM_SAPLING) sap++; else if(it[s]==ITEM_APPLE) app++; }
		}
		CHECK(sap>1700 && sap<2300 && app>130 && app<270);
	}

	/* in survival: leaves never drop themselves, saplings and apples sometimes */
	setup(true,false);
	srand(99);
	for(n=0;n<600;n++){ survivalBlockBroken(n%3?LEAVES_BLOCK:LEAVES_DECAY,100,100,30); collect(); }
	CHECK(inventoryCount(LEAVES_BLOCK)==0 && inventoryCount(LEAVES_DECAY)==0 && inventoryCount(ITEM_SAPLING)>10);
	CHECK(survivalBreakTicks(LEAVES_PLACED,0)==survivalBreakTicks(LEAVES_BLOCK,0) && survivalBreakTicks(ITEM_SAPLING,0)==1);
	brk(SAPLING_GROWN);
	CHECK(itemMaxStack(ITEM_SAPLING)==64);
	inventorySelect(0);
	CHECK(is(slot(0),ITEM_SAPLING,slot(0)->count) && survivalCanPlace(ITEM_SAPLING));

	/* apples: Beta food, 2 hearts, eaten even at full health, one per slot */
	CHECK(itemMaxStack(ITEM_APPLE)==1 && !survivalCanPlace(ITEM_APPLE));
	inventoryAdd(ITEM_APPLE,1,0); inventoryAdd(ITEM_APPLE,1,0);
	for(n=0;n<INV_HOTBAR && slot(n)->item!=ITEM_APPLE;n++);
	CHECK(n<INV_HOTBAR && is(slot(n),ITEM_APPLE,1));
	inventorySelect(n);
	survivalDamage(9);
	CHECK(health()==11 && survivalEat() && health()==15 && emptyStack(slot(n)));
	CHECK(!survivalEat());                                             /* nothing to eat in that slot */
	for(n=0;n<INV_HOTBAR && slot(n)->item!=ITEM_APPLE;n++);
	inventorySelect(n);
	survivalDamage(1);
	CHECK(survivalEat() && health()==18);
	survivalKill();
	setup(false,false);
	CHECK(!survivalEat());
	survivalKill();
}

static void testFarm(void)
{
	plantWorld_struct w=pwView();
	int n, s, c;
	u8 it[8];
	u16 art[256];

	/* ids */
	CHECK(isFarmland(FARMLAND_FIRST) && isFarmland(FARMLAND_FIRST+7) && !isFarmland(CARROT_FIRST));
	CHECK(isCarrotCrop(CARROT_FIRST+7) && isStem(STEM_FIRST) && isStem(STEM_ATTACHED+3) && isAttachedStem(STEM_ATTACHED) && !isAttachedStem(STEM_FIRST+7));
	CHECK(isPumpkin(PUMPKIN_FIRST+3) && !isPumpkin(PUMPKIN_FIRST+4) && isHoe(ITEM_WOOD_HOE) && isHoe(ITEM_STONE_HOE));
	CHECK(isPlant(CARROT_FIRST) && isPlant(STEM_ATTACHED) && !isPlant(FARMLAND_FIRST) && !isPlant(PUMPKIN_FIRST));
	CHECK(!solid(CARROT_FIRST+3) && !solid(STEM_FIRST) && solid(FARMLAND_FIRST) && solid(PUMPKIN_FIRST));
	CHECK(ITEM_PUMPKIN<112 && ITEM_CARROT<112 && ITEM_STONE_HOE<112);       /* items with icons */

	/* faces */
	CHECK(farmTexture(FARMLAND_FIRST,0)==FARMLAND_DRY_TILE && farmTexture(FARMLAND_FIRST+1,0)==FARMLAND_WET_TILE);
	CHECK(farmTexture(FARMLAND_FIRST+7,0)==FARMLAND_WET_TILE && farmTexture(FARMLAND_FIRST+7,1)==0 && farmTexture(FARMLAND_FIRST+7,3)==0);
	CHECK(farmTexture(PUMPKIN_FIRST+3,5)==PUMPKIN_FACE_TILE && farmTexture(PUMPKIN_FIRST+3,4)==PUMPKIN_SIDE_TILE && farmTexture(PUMPKIN_FIRST,2)==PUMPKIN_FACE_TILE);
	CHECK(farmTexture(PUMPKIN_FIRST,0)==PUMPKIN_TOP_TILE && farmTexture(PUMPKIN_FIRST,1)==PUMPKIN_TOP_TILE);
	{
		static const u8 look[8]={0,0,1,1,2,2,2,3};                     /* BlockCarrot.getIcon */
		for(s=0;s<8;s++) CHECK(farmTexture(CARROT_FIRST+s,6)==CARROT_TILE+look[s]);
		for(s=0;s<8;s++) CHECK(farmTexture(STEM_FIRST+s,8)==STEM_TILE+s);
	}
	/* a bent stem's plane bends towards its pumpkin from both sides */
	CHECK(farmTexture(STEM_ATTACHED+0,16)==STEM_BENT_TILE && farmTexture(STEM_ATTACHED+0,17)==STEM_BENT_TILE+1);
	CHECK(farmTexture(STEM_ATTACHED+1,16)==STEM_BENT_TILE+1 && farmTexture(STEM_ATTACHED+1,17)==STEM_BENT_TILE);
	CHECK(farmTexture(STEM_ATTACHED+2,14)==STEM_BENT_TILE && farmTexture(STEM_ATTACHED+3,15)==STEM_BENT_TILE);
	CHECK(cropStage(STEM_ATTACHED+2)==7 && cropStage(CARROT_FIRST+5)==5);

	/* water within 4 blocks, at the same level or one up */
	pwFlat(5);
	pw[14][10][5]=WATERTYPE;
	CHECK(farmWaterNear(&w,10,10,5));
	pw[14][10][5]=1; pw[15][10][5]=WATERTYPE;
	CHECK(!farmWaterNear(&w,10,10,5));
	pw[15][10][5]=1; pw[12][13][6]=WATERTYPE;
	CHECK(farmWaterNear(&w,10,10,5));
	pw[12][13][6]=0; pw[12][13][4]=WATERTYPE;
	CHECK(!farmWaterNear(&w,10,10,5));

	/* growth chance (BlockCrops.getGrowthChance) */
	pwFlat(5);
	pw[10][10][5]=FARMLAND_FIRST; pw[10][10][6]=CARROT_FIRST;
	CHECK(farmGrowthChance(&w,10,10,6)==2.0f);                        /* one dry farmland */
	pw[10][10][5]=FARMLAND_FIRST+7;
	CHECK(farmGrowthChance(&w,10,10,6)==4.0f);                        /* wet */
	for(n=9;n<=11;n++)for(s=9;s<=11;s++) pw[n][s][5]=FARMLAND_FIRST+7;
	CHECK(farmGrowthChance(&w,10,10,6)==10.0f);                       /* a wet field: 1 + 3 + 8 * 3/4 */
	pw[9][10][6]=CARROT_FIRST+3; pw[11][10][6]=CARROT_FIRST;
	CHECK(farmGrowthChance(&w,10,10,6)==10.0f);                       /* a row is fine */
	pw[10][11][6]=CARROT_FIRST;
	CHECK(farmGrowthChance(&w,10,10,6)==5.0f);                        /* crops on both sides: halved */
	pw[9][10][6]=pw[11][10][6]=pw[10][11][6]=0;
	pw[11][11][6]=CARROT_FIRST;
	CHECK(farmGrowthChance(&w,10,10,6)==5.0f);                        /* diagonal: halved */
	pw[11][11][6]=STEM_FIRST;
	CHECK(farmGrowthChance(&w,10,10,6)==10.0f);                       /* another crop does not count */

	/* crops and stems stay on farmland with light 8 or the sky */
	CHECK(farmPlantStays(&w,10,10,6));
	pw[10][10][5]=2; CHECK(!farmPlantStays(&w,10,10,6));
	pw[10][10][5]=FARMLAND_FIRST;
	for(n=0;n<32;n++)for(s=0;s<32;s++) pw[n][s][20]=3;
	CHECK(!farmPlantStays(&w,10,10,6));
	pw[13][10][6]=13; CHECK(farmPlantStays(&w,10,10,6));

	/* a grown stem finds its pumpkin: -x, +x, -y, +y */
	pwFlat(5);
	CHECK(farmStemFruitSide(&w,10,10,6)==-1);
	pw[10][11][6]=PUMPKIN_FIRST; CHECK(farmStemFruitSide(&w,10,10,6)==2);
	pw[10][9][6]=PUMPKIN_FIRST+1; CHECK(farmStemFruitSide(&w,10,10,6)==3);
	pw[11][10][6]=PUMPKIN_FIRST+2; CHECK(farmStemFruitSide(&w,10,10,6)==0);
	pw[9][10][6]=PUMPKIN_FIRST+3; CHECK(farmStemFruitSide(&w,10,10,6)==1);

	/* drops: a carrot, grown ones up to 4; seeds from stems by stage */
	srand(4321);
	for(n=0;n<200;n++){ c=farmCropDrops(CARROT_FIRST+3,it,8); if(c!=1 || it[0]!=ITEM_CARROT){ CHECK(0); break; } }
	{
		int total=0, most=0, seedsYoung=0, seedsGrown=0;
		for(n=0;n<3000;n++){ c=farmCropDrops(CARROT_FIRST+7,it,8); total+=c; if(c>most) most=c; for(s=0;s<c;s++) if(it[s]!=ITEM_CARROT) CHECK(0); }
		CHECK(most==4 && total>3000*2.45 && total<3000*2.75);              /* 1 + 3 * 8/15 */
		for(n=0;n<3000;n++){ seedsYoung+=farmCropDrops(STEM_FIRST,it,8); c=farmCropDrops(STEM_ATTACHED+1,it,8); seedsGrown+=c; if(c && it[0]!=ITEM_PUMPKIN_SEEDS) CHECK(0); }
		CHECK(seedsYoung>3000*0.13 && seedsYoung<3000*0.27);              /* 3 * 1/15 */
		CHECK(seedsGrown>3000*1.45 && seedsGrown<3000*1.75);              /* 3 * 8/15 */
	}

	/* pictures: the stem shows its lower (stage*2+2)/16, tinted green to yellow */
	farmArtTile(STEM_TILE,art);
	for(n=0;n<14*16;n++) if(art[n]){ CHECK(0); break; }
	for(c=0,n=14*16;n<256;n++) if(art[n]) c++;
	CHECK(c>0);
	for(n=0;n<256;n++) if(art[n] && (art[n]&31)){ CHECK(0); break; }   /* stage 0: no red */
	farmArtTile(STEM_TILE+7,art);
	for(c=0,n=0;n<256;n++) if(art[n]) c++;
	CHECK(c>20);
	{
		u16 a2[256];
		farmArtTile(STEM_BENT_TILE,art); farmArtTile(STEM_BENT_TILE+1,a2);
		for(n=0;n<256;n++) if(art[n]!=a2[(15-n%16)+(n/16)*16]){ CHECK(0); break; }
		for(s=CARROT_TILE;s<=SEEDS_TILE;s++){ farmArtTile(s,art); for(c=0,n=0;n<256;n++) if(art[n]) c++; if(!c) CHECK(0); }
	}

	/* survival: mining, drops, food, recipes, the hoe */
	CHECK(survivalBreakTicks(PUMPKIN_FIRST+1,0)==45 && survivalBreakTicks(PUMPKIN_FIRST,ITEM_WOOD_AXE)==23);
	CHECK(survivalBreakTicks(FARMLAND_FIRST+3,0)==27 && survivalBreakTicks(CARROT_FIRST+7,0)==1 && survivalBreakTicks(STEM_ATTACHED,0)==1);
	CHECK(!survivalCanHarvest(37,0) && survivalCanHarvest(37,ITEM_WOOD_PICKAXE));   /* netherrack */
	CHECK(survivalCanHarvest(38,0) && survivalBreakTicks(38,ITEM_WOOD_SHOVEL)<survivalBreakTicks(38,0));                                          /* soul sand */
	setup(true,false);
	brk(FARMLAND_FIRST+5);
	CHECK(inventoryCount(2)==1);
	brk(PUMPKIN_FIRST+2);
	CHECK(inventoryCount(ITEM_PUMPKIN)==1 && inventoryCount(PUMPKIN_FIRST+2)==0);
	srand(1);
	for(n=0;n<20;n++) brk(CARROT_FIRST+7);
	CHECK(inventoryCount(ITEM_CARROT)>=20*2 && inventoryCount(CARROT_FIRST+7)==0);
	CHECK(itemMaxStack(ITEM_CARROT)==64 && itemMaxStack(ITEM_PUMPKIN_SEEDS)==64 && itemMaxStack(ITEM_PUMPKIN)==64 && itemMaxStack(ITEM_WOOD_HOE)==1);
	/* a carrot is eaten for 1.5 hearts; it cannot be placed as a block */
	for(n=0;n<INV_HOTBAR && slot(n)->item!=ITEM_CARROT;n++);
	inventorySelect(n);
	CHECK(!survivalCanPlace(ITEM_CARROT));
	survivalDamage(6);
	c=slot(n)->count;
	CHECK(survivalEat() && health()==17 && slot(n)->count==c-1);
	for(n=0;n<INV_HOTBAR && slot(n)->item!=ITEM_PUMPKIN;n++);
	inventorySelect(n);
	CHECK(survivalCanPlace(ITEM_PUMPKIN));
	/* recipes: hoes (either hand), a pumpkin gives 4 seeds */
	inventoryOpen(true);
	clearGrid(); put(0,ITEM_PLANKS,1); put(1,ITEM_PLANKS,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_WOOD_HOE,1));
	clearGrid(); put(1,ITEM_COBBLESTONE,1); put(2,ITEM_COBBLESTONE,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_STONE_HOE,1));                                   /* mirrored */
	clearGrid(); put(4,ITEM_PUMPKIN,1);
	CHECK(result(ITEM_PUMPKIN_SEEDS,4));
	clearGrid();
	inventoryClose();
	/* the hoe wears out after 60 (wood) or 132 (stone) uses, shown by a bar */
	inventoryAdd(ITEM_WOOD_HOE,1,0);
	for(n=0;n<INV_HOTBAR && slot(n)->item!=ITEM_WOOD_HOE;n++);
	inventorySelect(n);
	CHECK(survivalToolDurability(ITEM_WOOD_HOE)==60 && survivalToolDurability(ITEM_STONE_HOE)==132);
	for(s=0;s<59;s++) survivalWearSelected();
	CHECK(is(slot(n),ITEM_WOOD_HOE,1) && slot(n)->wear==59);
	survivalWearSelected();
	CHECK(emptyStack(slot(n)));
	/* a fall tells the farm (trampling) how far the player fell */
	lastLanding=-1;
	fall(2);
	CHECK(lastLanding>=2*4096 && lastLanding<3*4096);
	survivalKill();

	/* creative: the farm items are in the catalogue */
	setup(false,false);
	creativeInit();
	{
		int i, found=0;
		for(s=0;s<4;s++)
		{
			creativeSetScroll(s);
			creativeUpdateUI(true);
			for(i=0;i<27;i++)
			{
				int a2=oamSub.oamMemory[items[9+i].id].attribute[2]&1023;
				if(a2==survivalIconTile(ITEM_PUMPKIN) || a2==survivalIconTile(ITEM_CARROT) || a2==survivalIconTile(ITEM_PUMPKIN_SEEDS) || a2==survivalIconTile(ITEM_WOOD_HOE)) found++;
			}
		}
		CHECK(found>=4);
	}
	survivalKill();
}

static void testIron(void)
{
	int n;
	/* iron tools: same kinds as the wooden and stone ones, tier 3 */
	CHECK(isTool(ITEM_IRON_PICKAXE) && isTool(ITEM_IRON_AXE) && !isTool(ITEM_IRON_HOE) && !isTool(ITEM_IRON_INGOT) && !isTool(ITEM_STICK));
	CHECK(toolKind(ITEM_IRON_PICKAXE)==TOOLKIND_PICKAXE && toolKind(ITEM_IRON_SHOVEL)==TOOLKIND_SHOVEL && toolKind(ITEM_IRON_AXE)==TOOLKIND_AXE);
	CHECK(toolTier(ITEM_IRON_PICKAXE)==3 && toolTier(ITEM_STONE_AXE)==2 && toolTier(ITEM_WOOD_SHOVEL)==1);
	CHECK(survivalToolDurability(ITEM_IRON_PICKAXE)==251 && survivalToolDurability(ITEM_IRON_HOE)==251 && isHoe(ITEM_IRON_HOE));
	/* efficiency 6 (Beta's EnumToolMaterial) */
	CHECK(survivalBreakTicks(3,ITEM_WOOD_PICKAXE)==34 && survivalBreakTicks(3,ITEM_STONE_PICKAXE)==17 && survivalBreakTicks(3,ITEM_IRON_PICKAXE)==12);
	CHECK(survivalBreakTicks(2,ITEM_IRON_SHOVEL)<survivalBreakTicks(2,ITEM_STONE_SHOVEL) && survivalBreakTicks(7,ITEM_IRON_AXE)<survivalBreakTicks(7,ITEM_STONE_AXE));
	/* iron ore: hardness 3, a stone pickaxe or better */
	CHECK(!survivalCanHarvest(ITEM_IRON_ORE,0) && !survivalCanHarvest(ITEM_IRON_ORE,ITEM_WOOD_PICKAXE) && !survivalCanHarvest(ITEM_IRON_ORE,ITEM_STONE_AXE));
	CHECK(survivalCanHarvest(ITEM_IRON_ORE,ITEM_STONE_PICKAXE) && survivalCanHarvest(ITEM_IRON_ORE,ITEM_IRON_PICKAXE));
	CHECK(survivalBreakTicks(ITEM_IRON_ORE,ITEM_STONE_PICKAXE)==survivalBreakTicks(ITEM_COAL_ORE,ITEM_STONE_PICKAXE));
	/* what an iron pickaxe opens up: gold and diamond blocks, not obsidian */
	CHECK(!survivalCanHarvest(32,ITEM_STONE_PICKAXE) && survivalCanHarvest(32,ITEM_IRON_PICKAXE) && survivalCanHarvest(33,ITEM_IRON_PICKAXE));
	CHECK(!survivalCanHarvest(36,ITEM_IRON_PICKAXE));
	/* the furnace smelts it into an ingot */
	CHECK(furnaceSmeltResult(ITEM_IRON_ORE)==ITEM_IRON_INGOT && !furnaceFuelTime(ITEM_IRON_INGOT) && !furnaceSmeltResult(ITEM_IRON_INGOT));
	CHECK(itemMaxStack(ITEM_IRON_INGOT)==64 && itemMaxStack(ITEM_IRON_ORE)==64 && itemMaxStack(ITEM_IRON_PICKAXE)==1 && itemMaxStack(ITEM_IRON_HOE)==1);
	CHECK(isCubeItem(ITEM_IRON_ORE) && !isCubeItem(ITEM_IRON_INGOT));

	setup(true,false);
	inventoryAdd(ITEM_STONE_PICKAXE,1,0);
	inventorySelect(0);
	brk(ITEM_IRON_ORE);
	CHECK(inventoryCount(ITEM_IRON_ORE)==1);
	for(n=0;n<INV_HOTBAR && slot(n)->item!=ITEM_IRON_ORE;n++);
	inventorySelect(n);
	CHECK(survivalCanPlace(ITEM_IRON_ORE));
	/* recipes */
	inventoryOpen(true);
	clearGrid(); put(0,ITEM_IRON_INGOT,1); put(1,ITEM_IRON_INGOT,1); put(2,ITEM_IRON_INGOT,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_IRON_PICKAXE,1));
	clearGrid(); put(1,ITEM_IRON_INGOT,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_IRON_SHOVEL,1));
	clearGrid(); put(0,ITEM_IRON_INGOT,1); put(1,ITEM_IRON_INGOT,1); put(3,ITEM_IRON_INGOT,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_IRON_AXE,1));
	clearGrid(); put(0,ITEM_IRON_INGOT,1); put(1,ITEM_IRON_INGOT,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_IRON_HOE,1));
	clearGrid(); put(1,ITEM_IRON_INGOT,1); put(2,ITEM_IRON_INGOT,1); put(4,ITEM_STICK,1); put(5,ITEM_IRON_INGOT,1); put(7,ITEM_STICK,1);
	CHECK(result(ITEM_IRON_AXE,1));                                   /* mirrored */
	clearGrid(); put(1,ITEM_IRON_ORE,1); put(4,ITEM_STICK,1); put(7,ITEM_STICK,1);
	CHECK(noResult());                                                 /* ore is not an ingot */
	clearGrid();
	inventoryClose();
	/* an iron tool wears out after 251 uses */
	inventoryAdd(ITEM_IRON_PICKAXE,1,0);
	for(n=0;n<INV_HOTBAR && slot(n)->item!=ITEM_IRON_PICKAXE;n++);
	inventorySelect(n);
	for(n=0;n<250;n++) survivalBlockBroken(3,100,100,30);
	CHECK(inventoryCount(ITEM_IRON_PICKAXE)==1);
	survivalBlockBroken(3,100,100,30);
	CHECK(inventoryCount(ITEM_IRON_PICKAXE)==0);
	survivalKill();
}

int main(void)
{
	testHealth();
	testStacks();
	testClicks();
	testCrafting();
	testDragSpread();
	testMiningAndTools();
	testMigration();
	testDrops();
	testDisplay();
	testCreative();
	testVoid();
	testCoal();
	testChest();
	testFurnace();
	testPlants();
	testFarm();
	testIron();
	printf(failures?"%d FAILURES\n":"all survival tests passed\n",failures);
	return failures!=0;
}

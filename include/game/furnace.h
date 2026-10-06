#ifndef FURNACE_9
#define FURNACE_9

// Furnaces, as in Minecraft Beta 1.7: an input, a fuel and an output slot.
// Fuel burns for a time that depends on the item; while it burns, an item that
// can be smelted cooks in 200 ticks (10 s). A burning furnace shows its lit
// front and gives light. Furnaces keep working while their area is loaded,
// also with their screen closed.
//
// Each placed furnace is a block id that encodes its facing and whether it is
// lit; its contents and timers are kept per world in "<world>.furnaces".

#define ITEM_FURNACE 63
#define FURNACE_STATE_FIRST 84     // placed furnaces: 84 + facing (unlit), 88 + facing (lit)
#define FURNACE_STATES 8
#define ITEM_CHARCOAL 92           // Minecraft's coal with damage 1, from smelting logs

#define FURNACE_IN 0
#define FURNACE_FUEL 1
#define FURNACE_OUT 2
#define FURNACE_SLOTS 3

#define FURNACE_COOK_TIME 200      // Minecraft ticks (20 per second)
#define FURNACE_LIGHT 13           // light level of a lit furnace (a torch gives 14)

// atlas tiles (terrain.png: front 12,2  side 13,2  top 14,3  lit front 13,3)
#define FURNACE_TILE_FRONT 63
#define FURNACE_TILE_SIDE 64
#define FURNACE_TILE_TOP 65
#define FURNACE_TILE_FRONT_LIT 66
#define CHARCOAL_TILE 67           // items.png coal, tinted (see charcoalTint)

typedef struct furnace_s
{
	u16 i, j;
	u8 k;
	u8 used;
	stack_struct slots[FURNACE_SLOTS];
	u16 burnTime;                  // ticks of fuel left
	u16 burnMax;                   // burn time of the last fuel item (for the flame)
	u16 cookTime;                  // progress of the item being smelted
}furnace_struct;

static inline bool isFurnaceBlock(u8 t){ return t>=FURNACE_STATE_FIRST && t<FURNACE_STATE_FIRST+FURNACE_STATES; }
static inline bool isLitFurnace(u8 t){ return t>=FURNACE_STATE_FIRST+4 && t<FURNACE_STATE_FIRST+FURNACE_STATES; }
static inline u8 furnaceState(int facing, bool lit){ return FURNACE_STATE_FIRST+(lit?4:0)+facing; }
static inline int furnaceFacing(u8 t){ return (t-FURNACE_STATE_FIRST)%4; }

// Charcoal shares coal's picture in Beta (told apart by its name); without
// item names on the DS it is drawn browner, as in later versions.
static inline u16 charcoalTint(u16 c)
{
	int r=c&31, g=(c>>5)&31, b=(c>>10)&31;
	r=r+4>31?31:r+4; g=g+2>31?31:g+2; b=b>1?b-1:0;
	return (c&BIT(15))|r|(g<<5)|(b<<10);
}

// pure rules (tested on the host)
u8 furnaceTexture(u8 state, u8 direction);
int furnaceFuelTime(u8 item);              // Minecraft ticks of burning, 0: not fuel
u8 furnaceSmeltResult(u8 item);            // 0: cannot be smelted
bool furnaceTick(furnace_struct* f);       // one Minecraft tick; true when it lights up or goes out
bool furnaceBurning(const furnace_struct* f);
int furnaceCookScaled(const furnace_struct* f, int n);   // arrow length (24 in the screen)
int furnaceBurnScaled(const furnace_struct* f, int n);   // flame height (12 in the screen)

// contents
void furnacesClear(void);
bool furnacesLoad(const char* mapPath);
bool furnacesSave(const char* mapPath);
void furnaceSidecarPath(const char* mapPath, char* out, int size);
furnace_struct* furnaceAt(int i, int j, int k, bool create);
void furnaceRemove(int i, int j, int k);
int furnaceCount(void);
furnace_struct* furnaceGet(int n);         // record n, or NULL (for the update loop)
int furnaceRecords(void);

// game hooks (furnaceworld.c)
bool furnacePlace(map_struct* m, int i, int j, int k);     // place an ITEM_FURNACE at i,j,k
bool furnaceOpen(map_struct* m, int i, int j, int k);      // survival: open the furnace screen
void furnaceBroken(map_struct* m, int i, int j, int k);
void furnacesUpdate(map_struct* m);                       // every 30 Hz game tick
void interfaceOpenFurnace(furnace_struct* f);             // interface.c: the furnace screen

#endif

#include "game/game_main.h"

#include <maxmod9.h>
#include "soundbank.h"

// State lives in the survival save block so it is written with the world.
static survivalSave_struct* data(void){ return survivalData(); }

static bool windowOpen;
static bool tableOpen;
static chest_struct* chestA;       // open chest: first 27 slots
static chest_struct* chestB;       // second half of a double chest
static int chestPage;
static furnace_struct* furnace;     // open furnace
static void forgetContainerIcons(void);
static stack_struct emptyStack;

/* ---------------------------------------------------------------------------
 * Stacks
 * ------------------------------------------------------------------------- */

u8 itemMaxStack(u8 item)
{
	if(isTool(item))return 1;
	if(item==DOORTYPE || item==11)return 1;   // wooden door, water bucket
	if(item==ITEM_APPLE)return 1;            // Beta food does not stack
	if(isHoe(item) || isArmor(item))return 1;
	return MAX_STACK;
}

static inline bool isEmpty(const stack_struct* s){ return !s->count || !s->item; }
static inline void clear(stack_struct* s){ s->item=0; s->count=0; s->wear=0; }
static inline bool canMerge(const stack_struct* a, const stack_struct* b)
{
	return a->item==b->item && itemMaxStack(a->item)>1;
}

stack_struct* inventorySlot(int slot)
{
	survivalSave_struct* d=data();
	if(!d)return NULL;
	if(slot>=0 && slot<INV_SLOTS)return &d->slots[slot];
	if(slot>=SLOT_GRID && slot<SLOT_GRID+CRAFT_CELLS)return &d->grid[slot-SLOT_GRID];
	if(slot==SLOT_HELD)return &d->held;
	if(slot>=SLOT_ARMOR && slot<SLOT_ARMOR+4)return &d->armor[slot-SLOT_ARMOR];
	if(slot>=SLOT_CHEST && slot<SLOT_CHEST+2*CHEST_SLOTS)
	{
		int n=slot-SLOT_CHEST;
		if(n<CHEST_SLOTS)return chestA?&chestA->slots[n]:NULL;
		return chestB?&chestB->slots[n-CHEST_SLOTS]:NULL;
	}
	if(slot>=SLOT_FURNACE && slot<SLOT_FURNACE+FURNACE_SLOTS)return furnace?&furnace->slots[slot-SLOT_FURNACE]:NULL;
	return NULL;
}

static inline bool isArmorSlot(int slot)
{
	return slot>=SLOT_ARMOR && slot<SLOT_ARMOR+4;
}

// an armour slot takes only its own piece (Minecraft's SlotArmor)
static bool fitsSlot(int slot, const stack_struct* s)
{
	if(isArmorSlot(slot))return isArmor(s->item) && armorType(s->item)==slot-SLOT_ARMOR;
	return true;
}

static inline bool isFurnaceSlot(int slot)
{
	return furnace && slot>=SLOT_FURNACE && slot<SLOT_FURNACE+FURNACE_SLOTS;
}

static inline bool isChestSlot(int slot)
{
	return slot>=SLOT_CHEST && slot<SLOT_CHEST+2*CHEST_SLOTS && inventorySlot(slot);
}

stack_struct* inventoryHeld(void)
{
	return data()?&data()->held:&emptyStack;
}

int inventoryCount(u8 item)
{
	int i, n=0;
	if(!data())return 0;
	for(i=0;i<INV_SLOTS;i++)if(data()->slots[i].item==item)n+=data()->slots[i].count;
	return n;
}

// Merge into existing stacks first, then fill empty slots; [first,last) in order.
static int addRange(u8 item, int count, u16 wear, int first, int last)
{
	int i, max=itemMaxStack(item);
	stack_struct* s=data()->slots;
	if(max>1)for(i=first;i<last && count;i++)
	{
		if(s[i].item==item && s[i].count && s[i].count<max)
		{
			int n=max-s[i].count;
			if(n>count)n=count;
			s[i].count+=n;
			count-=n;
		}
	}
	for(i=first;i<last && count;i++)
	{
		if(isEmpty(&s[i]))
		{
			int n=(count<max)?count:max;
			s[i].item=item; s[i].count=n; s[i].wear=wear;
			count-=n;
		}
	}
	return count;
}

// Crafted results taken with shift go to the hotbar from the right, then to the
// main inventory from the bottom (Beta 1.7 merges them in reverse slot order).
static int addReverse(u8 item, int count, u16 wear)
{
	int pass, n, max=itemMaxStack(item);
	stack_struct* s=data()->slots;
	for(pass=0;pass<2 && count;pass++)
	{
		for(n=0;n<INV_SLOTS && count;n++)
		{
			int i=(n<INV_HOTBAR)?INV_HOTBAR-1-n:INV_SLOTS-1-(n-INV_HOTBAR);
			if(!pass && max>1 && s[i].item==item && s[i].count && s[i].count<max)
			{
				int k=max-s[i].count;
				if(k>count)k=count;
				s[i].count+=k;
				count-=k;
			}else if(pass && isEmpty(&s[i]))
			{
				int k=(count<max)?count:max;
				s[i].item=item; s[i].count=k; s[i].wear=wear;
				count-=k;
			}
		}
	}
	return count;
}

int inventoryAdd(u8 item, int count, u16 wear)
{
	if(!data() || !item || count<=0)return count;
	return addRange(item,count,wear,0,INV_SLOTS);
}

stack_struct* inventorySelected(void)
{
	return data()?&data()->slots[data()->selected%INV_HOTBAR]:&emptyStack;
}

int inventorySelectedIndex(void)
{
	return data()?data()->selected%INV_HOTBAR:0;
}

void inventorySelect(int hotbar)
{
	if(data() && hotbar>=0 && hotbar<INV_HOTBAR)data()->selected=hotbar;
}

/* ---------------------------------------------------------------------------
 * Crafting (shaped recipes, Minecraft Beta 1.7)
 * ------------------------------------------------------------------------- */

typedef struct
{
	const char* rows[3];           // P planks, S stick, C cobblestone, L any log, K coal, U pumpkin, I iron ingot, '.' empty
	u8 result, count;
}recipe_struct;

static const recipe_struct recipes[]={
	{{"L"},                 ITEM_PLANKS,4},
	{{"P","P"},             ITEM_STICK,4},
	{{"PP","PP"},           ITEM_CRAFTING_TABLE,1},
	{{"PPP",".S.",".S."},   ITEM_WOOD_PICKAXE,1},
	{{"CCC",".S.",".S."},   ITEM_STONE_PICKAXE,1},
	{{"P","S","S"},         ITEM_WOOD_SHOVEL,1},
	{{"C","S","S"},         ITEM_STONE_SHOVEL,1},
	{{"PP","PS",".S"},      ITEM_WOOD_AXE,1},
	{{"CC","CS",".S"},      ITEM_STONE_AXE,1},
	{{"S.S","SSS","S.S"},   LADDERTYPE,2},
	{{"PP","PP","PP"},      DOORTYPE,1},
	{{"K","S"},             13,4},                    // torch: coal on a stick
	{{"PPP","P.P","PPP"},   ITEM_CHEST,1},
	{{"CCC","C.C","CCC"},   ITEM_FURNACE,1},
	{{"PP",".S",".S"},      ITEM_WOOD_HOE,1},
	{{"CC",".S",".S"},      ITEM_STONE_HOE,1},
	{{"III",".S.",".S."},   ITEM_IRON_PICKAXE,1},
	{{"I","S","S"},         ITEM_IRON_SHOVEL,1},
	{{"II","IS",".S"},      ITEM_IRON_AXE,1},
	{{"II",".S",".S"},      ITEM_IRON_HOE,1},
	{{"III","I.I"},         ITEM_IRON_HELMET,1},
	{{"I.I","III","III"},   ITEM_IRON_HELMET+1,1},   // chestplate
	{{"III","I.I","I.I"},   ITEM_IRON_HELMET+2,1},   // leggings
	{{"I.I","I.I"},         ITEM_IRON_BOOTS,1},
	{{"U"},                 ITEM_PUMPKIN_SEEDS,4},     // Minecraft 1.4
};
#define RECIPES ((int)(sizeof(recipes)/sizeof(recipes[0])))

int craftingRecipes(void)
{
	return RECIPES;
}

static bool ingredientMatches(char c, const stack_struct* s)
{
	if(c=='.')return isEmpty(s);
	if(isEmpty(s))return false;
	switch(c)
	{
		case 'P': return s->item==ITEM_PLANKS;
		case 'S': return s->item==ITEM_STICK;
		case 'C': return s->item==ITEM_COBBLESTONE;
		case 'L': return itemIsLog(s->item);
		case 'K': return s->item==ITEM_COAL || s->item==ITEM_CHARCOAL;   // torches take either
		case 'U': return s->item==ITEM_PUMPKIN;
		case 'I': return s->item==ITEM_IRON_INGOT;
	}
	return false;
}

bool inventoryCellActive(int cell)
{
	if(cell<0 || cell>=CRAFT_CELLS)return false;
	return tableOpen || (cell%3<2 && cell/3<2);
}

int inventoryGridSize(void)
{
	return tableOpen?3:2;
}

// Bounding box of the occupied cells, then compare with each recipe (and its mirror).
static int matchRecipe(void)
{
	stack_struct* g=data()->grid;
	int x, y, r, minX=3, minY=3, maxX=-1, maxY=-1;
	for(y=0;y<3;y++)for(x=0;x<3;x++)
	{
		if(!inventoryCellActive(x+y*3) || isEmpty(&g[x+y*3]))continue;
		if(x<minX)minX=x;
		if(y<minY)minY=y;
		if(x>maxX)maxX=x;
		if(y>maxY)maxY=y;
	}
	if(maxX<0)return -1;
	int w=maxX-minX+1, h=maxY-minY+1;
	for(r=0;r<RECIPES;r++)
	{
		int rh=0, rw=strlen(recipes[r].rows[0]), mirror;
		while(rh<3 && recipes[r].rows[rh])rh++;
		if(rw!=w || rh!=h)continue;
		for(mirror=0;mirror<2;mirror++)
		{
			bool ok=true;
			for(y=0;y<h && ok;y++)for(x=0;x<w && ok;x++)
			{
				char c=recipes[r].rows[y][mirror?w-1-x:x];
				ok=ingredientMatches(c,&g[(minX+x)+(minY+y)*3]);
			}
			if(ok)return r;
		}
	}
	return -1;
}

bool craftingResult(stack_struct* out)
{
	int r;
	clear(out);
	if(!data() || (r=matchRecipe())<0)return false;
	out->item=recipes[r].result;
	out->count=recipes[r].count;
	return true;
}

static void consumeGrid(void)
{
	int i;
	for(i=0;i<CRAFT_CELLS;i++)
	{
		stack_struct* s=&data()->grid[i];
		if(!inventoryCellActive(i) || isEmpty(s))continue;
		if(!--s->count)clear(s);
	}
}

// would `count` items fit in the inventory?
static bool fits(u8 item, int count)
{
	int i, max=itemMaxStack(item), room=0;
	for(i=0;i<INV_SLOTS && room<count;i++)
	{
		stack_struct* s=&data()->slots[i];
		if(isEmpty(s))room+=max;
		else if(s->item==item && max>1)room+=max-s->count;
	}
	return room>=count;
}

static void clickResult(bool shift)
{
	stack_struct r;
	stack_struct* held=&data()->held;
	if(!craftingResult(&r))return;
	if(shift)
	{
		// craft as many as possible straight into the inventory
		while(craftingResult(&r) && fits(r.item,r.count))
		{
			consumeGrid();
			addReverse(r.item,r.count,0);
		}
		return;
	}
	if(isEmpty(held))*held=r;
	else if(canMerge(held,&r) && held->count+r.count<=itemMaxStack(r.item))held->count+=r.count;
	else return;
	consumeGrid();
}

/* ---------------------------------------------------------------------------
 * Clicks
 * ------------------------------------------------------------------------- */

// shift click: hotbar <-> main inventory, grid -> inventory
// into the open chest, in slot order: merge first, then empty slots
static int addToChest(u8 item, int count, u16 wear)
{
	int pass, n, max=itemMaxStack(item), total=chestB?2*CHEST_SLOTS:CHEST_SLOTS;
	for(pass=0;pass<2 && count;pass++)
	{
		for(n=0;n<total && count;n++)
		{
			stack_struct* s=inventorySlot(SLOT_CHEST+n);
			if(!pass && max>1 && s->item==item && s->count && s->count<max)
			{
				int k=max-s->count;
				if(k>count)k=count;
				s->count+=k;
				count-=k;
			}else if(pass && isEmpty(s))
			{
				int k=(count<max)?count:max;
				s->item=item; s->count=k; s->wear=wear;
				count-=k;
			}
		}
	}
	return count;
}

static void quickMove(int slot)
{
	stack_struct* s=inventorySlot(slot);
	int left;
	if(!s || isEmpty(s))return;
	// with a chest open (Beta 1.7): chest -> inventory (item bar from the right
	// first), inventory -> chest
	if(chestA)
	{
		if(isChestSlot(slot))left=addReverse(s->item,s->count,s->wear);
		else left=addToChest(s->item,s->count,s->wear);
	}
	// furnace output: to the item bar from the right, like a crafted result;
	// input and fuel go back to the inventory (Beta 1.7 never shift-fills a furnace)
	else if(isFurnaceSlot(slot) && slot==SLOT_FURNACE+FURNACE_OUT)left=addReverse(s->item,s->count,s->wear);
	else if(slot<INV_HOTBAR)left=addRange(s->item,s->count,s->wear,INV_HOTBAR,INV_SLOTS);
	else if(slot<INV_SLOTS)left=addRange(s->item,s->count,s->wear,0,INV_HOTBAR);
	else
	{
		left=addRange(s->item,s->count,s->wear,INV_HOTBAR,INV_SLOTS);
		if(left)left=addRange(s->item,left,s->wear,0,INV_HOTBAR);
	}
	if(left)s->count=left;
	else clear(s);
}

static void throwHeld(bool one)
{
	stack_struct* held=&data()->held;
	if(isEmpty(held))return;
	int n=one?1:held->count;
	dropsThrow(held->item,n,held->wear);
	held->count-=n;
	if(!held->count)clear(held);
}

void inventoryClick(int slot, bool right, bool shift)
{
	if(!data() || !windowOpen)return;
	if(slot==SLOT_RESULT){clickResult(shift);return;}
	if(slot==SLOT_OUTSIDE){throwHeld(right);return;}
	if(slot>=SLOT_GRID && slot<SLOT_GRID+CRAFT_CELLS && !inventoryCellActive(slot-SLOT_GRID))return;
	stack_struct* s=inventorySlot(slot);
	stack_struct* held=&data()->held;
	if(!s || slot==SLOT_HELD)return;

	if(shift){quickMove(slot);return;}

	// an armour slot refuses anything but its piece
	if(isArmorSlot(slot) && !isEmpty(held) && !fitsSlot(slot,held))return;

	// the furnace output only gives: take it, or add all of it to the held stack
	if(isFurnaceSlot(slot) && slot==SLOT_FURNACE+FURNACE_OUT && !isEmpty(held))
	{
		if(!isEmpty(s) && canMerge(s,held) && held->count+s->count<=itemMaxStack(held->item))
		{
			held->count+=s->count;
			clear(s);
		}
		return;
	}

	if(isEmpty(held))
	{
		if(isEmpty(s))return;
		if(right)
		{
			// take half, rounded up
			int n=(s->count+1)/2;
			held->item=s->item; held->count=n; held->wear=s->wear;
			s->count-=n;
			if(!s->count)clear(s);
		}else{
			*held=*s;
			clear(s);
		}
		return;
	}

	int max=itemMaxStack(held->item);
	if(isEmpty(s))
	{
		int n=right?1:held->count;
		s->item=held->item; s->count=n; s->wear=held->wear;
		held->count-=n;
		if(!held->count)clear(held);
	}else if(s->item==held->item)
	{
		// same item: add what fits (nothing for a full stack or a tool, as in Beta 1.7)
		int n=right?1:held->count;
		if(n>max-s->count)n=max-s->count;
		s->count+=n;
		held->count-=n;
		if(!held->count)clear(held);
	}else{
		stack_struct t=*s;
		*s=*held;
		*held=t;
	}
}

/* Stylus gestures (Minecraft 1.5+ drag behaviour, adapted to a touch screen):
   - press on a slot with an empty hand picks the stack up (L: half)
   - moving the stylus over slots collects them; on release
       no other slot  -> the stack stays on the stylus (tap, then tap again)
       one slot       -> a normal click there (put down, merge or swap)
       several slots  -> the stack is spread: evenly, or one each with L
   - press while holding a stack starts the same gesture from that slot
   - shift (R), the result slot and outside the window act at once on press */

#define DRAG_MAX (INV_SLOTS+2*CHEST_SLOTS)

static int pressSlot=SLOT_NONE;
static bool dragging;              // a gesture that acts on release
static bool dragRight;
static int lastSlot;
static int dragSlots[DRAG_MAX];
static int dragCount;

static bool dragTarget(int slot)
{
	if(chestA)return (slot>=0 && slot<INV_SLOTS) || isChestSlot(slot);
	if(furnace)return (slot>=0 && slot<INV_SLOTS) || slot==SLOT_FURNACE+FURNACE_IN || slot==SLOT_FURNACE+FURNACE_FUEL;
	if(isArmorSlot(slot))return !tableOpen;
	return (slot>=0 && slot<INV_SLOTS) || (slot>=SLOT_GRID && slot<SLOT_GRID+CRAFT_CELLS && inventoryCellActive(slot-SLOT_GRID));
}

void inventoryDragOver(int slot)
{
	int i;
	if(!dragging || slot==lastSlot)return;
	lastSlot=slot;
	if(!dragTarget(slot))return;
	for(i=0;i<dragCount;i++)if(dragSlots[i]==slot)return;
	if(dragCount<DRAG_MAX)dragSlots[dragCount++]=slot;
}

// spread the held stack over the dragged slots that can take it
static void spread(void)
{
	stack_struct* held=&data()->held;
	int i, n=0, targets[DRAG_MAX];
	int max=itemMaxStack(held->item);
	for(i=0;i<dragCount;i++)
	{
		stack_struct* s=inventorySlot(dragSlots[i]);
		if(!fitsSlot(dragSlots[i],held))continue;
		if(isEmpty(s) || (s->item==held->item && s->count<max))targets[n++]=dragSlots[i];
	}
	if(n>held->count)n=held->count;           // never fewer than one item per slot
	if(!n)return;
	int each=dragRight?1:held->count/n;
	for(i=0;i<n && held->count;i++)
	{
		stack_struct* s=inventorySlot(targets[i]);
		int k=each;
		if(isEmpty(s)){s->item=held->item; s->count=0; s->wear=held->wear;}
		if(k>max-s->count)k=max-s->count;
		if(k>held->count)k=held->count;
		s->count+=k;
		held->count-=k;
	}
	if(!held->count)clear(held);
}

void inventoryPress(int slot, bool right, bool shift)
{
	bool wasEmpty=isEmpty(inventoryHeld());
	pressSlot=slot;
	dragging=false;
	dragCount=0;
	lastSlot=slot;
	if(slot==SLOT_NONE)return;
	if(slot==SLOT_PAGE_UP || slot==SLOT_PAGE_DOWN)
	{
		if(chestB)chestPage=(slot==SLOT_PAGE_DOWN)?1:0;
		return;
	}
	if(shift || slot==SLOT_RESULT || slot==SLOT_OUTSIDE || !dragTarget(slot)){inventoryClick(slot,right,shift);return;}
	dragRight=right;
	if(wasEmpty)
	{
		// pick up now; the origin only counts if the stylus comes back to it
		inventoryClick(slot,right,false);
		dragging=!isEmpty(inventoryHeld());
	}else{
		dragging=true;
		dragSlots[dragCount++]=slot;
	}
}

void inventoryRelease(int slot)
{
	if(dragging)
	{
		inventoryDragOver(slot);
		if(dragCount==1)inventoryClick(dragSlots[0],dragRight,false);
		else if(dragCount>1)spread();
	}
	dragging=false;
	dragCount=0;
	pressSlot=SLOT_NONE;
}

/* ---------------------------------------------------------------------------
 * Window
 * ------------------------------------------------------------------------- */

void inventoryOpen(bool table)
{
	windowOpen=true;
	tableOpen=table;
	chestA=chestB=NULL;
	furnace=NULL;
	dragging=false;
	pressSlot=SLOT_NONE;
}

// Items left in the grid or on the stylus go back to the inventory; what does
// not fit is thrown in front of the player.
void inventoryClose(void)
{
	int i;
	if(data())
	{
		for(i=0;i<CRAFT_CELLS;i++)
		{
			stack_struct* s=&data()->grid[i];
			if(isEmpty(s))continue;
			int left=inventoryAdd(s->item,s->count,s->wear);
			if(left)dropsThrow(s->item,left,s->wear);
			clear(s);
		}
		stack_struct* h=&data()->held;
		if(!isEmpty(h))
		{
			int left=inventoryAdd(h->item,h->count,h->wear);
			if(left)dropsThrow(h->item,left,h->wear);
			clear(h);
		}
	}
	windowOpen=false;
	tableOpen=false;
	chestA=chestB=NULL;
	furnace=NULL;
	dragging=false;
}

void inventoryOpenChest(struct chest_s* a, struct chest_s* b)
{
	inventoryOpen(false);
	chestA=a;
	chestB=b;
	chestPage=0;
	forgetContainerIcons();
}

bool inventoryIsChest(void)
{
	return windowOpen && chestA;
}

int inventoryChestPage(void)
{
	return chestPage;
}

void inventoryOpenFurnace(struct furnace_s* f)
{
	inventoryOpen(false);
	furnace=f;
	forgetContainerIcons();
}

bool inventoryIsFurnace(void)
{
	return windowOpen && furnace;
}

void inventoryReset(void)
{
	furnace=NULL;
	chestA=chestB=NULL;
	windowOpen=false;
	tableOpen=false;
	dragging=false;
	dragCount=0;
	pressSlot=SLOT_NONE;
}

bool inventoryIsOpen(void)
{
	return windowOpen;
}

bool inventoryIsTable(void)
{
	return windowOpen && tableOpen;
}

/* ---------------------------------------------------------------------------
 * Display: one 16x16 bitmap sprite per slot, drawn into its own icon
 * ------------------------------------------------------------------------- */

#define HOTBARX 59               // item bar with the window closed
#define HOTBARY 170
#define HOTBARD 20
#define INVX 55                  // inventory and crafting table screens
#define INVY 110
#define INVBARY 168
#define INVD 18
#define OUTSIDE_LEFT 47          // the window spans 47..232 horizontally
#define OUTSIDE_RIGHT 233
#define TOPBAR 26                // Create / Inventory / Save buttons

static const u8 grid2X[4]={135,153,135,153}, grid2Y[4]={52,52,70,70};
#define RESULT2X 191
#define RESULT2Y 62
#define GRID3X 86
#define GRID3Y 44
#define RESULT3X 170
#define RESULT3Y 62
#define CHESTX 55                // chest screen: three rows above the inventory
#define CHESTY 41
#define PAGEX 236                // page arrows of a double chest
#define PAGEUPY 41
#define PAGEDOWNY 77
#define ARMORX 55                // the inventory screen's armour slots (Beta's 8, 8+18n, +47, +26)
#define ARMORY 34
#define FURNACEINX 103           // furnace screen: Beta's layout (+47, +26)
#define FURNACEINY 43
#define FURNACEFUELY 79
#define FURNACEOUTX 163
#define FURNACEOUTY 61
#define FLAMEX 103               // flame: 14x14, arrow: 24x16
#define FLAMEY 62
#define ARROWX 126
#define ARROWY 60

extern u8 cursorSprite;

static stack_struct shown[UI_SLOTS];
static bool shownValid[UI_SLOTS];
static int heldX, heldY;

// position of a slot on screen; false when it is not displayed
static bool slotPos(int slot, bool open, int* x, int* y)
{
	if(slot<INV_HOTBAR)
	{
		if(open){*x=INVX+slot*INVD; *y=INVBARY;}
		else{*x=HOTBARX+slot*HOTBARD; *y=HOTBARY;}
		return true;
	}
	if(!open)return false;
	if(slot<INV_SLOTS)
	{
		*x=INVX+((slot-INV_HOTBAR)%9)*INVD;
		*y=INVY+((slot-INV_HOTBAR)/9)*INVD;
		return true;
	}
	if(isArmorSlot(slot))
	{
		// only on the inventory screen, as in Minecraft
		if(furnace || chestA || tableOpen)return false;
		*x=ARMORX;
		*y=ARMORY+(slot-SLOT_ARMOR)*INVD;
		return true;
	}
	if(furnace)
	{
		if(slot==SLOT_FURNACE+FURNACE_IN){*x=FURNACEINX; *y=FURNACEINY; return true;}
		if(slot==SLOT_FURNACE+FURNACE_FUEL){*x=FURNACEINX; *y=FURNACEFUELY; return true;}
		if(slot==SLOT_FURNACE+FURNACE_OUT){*x=FURNACEOUTX; *y=FURNACEOUTY; return true;}
		return false;
	}
	if(chestA)
	{
		int n=slot-SLOT_CHEST-chestPage*CHEST_VIEW;
		if(slot<SLOT_CHEST || n<0 || n>=CHEST_VIEW)return false;
		*x=CHESTX+(n%9)*INVD;
		*y=CHESTY+(n/9)*INVD;
		return true;
	}
	if(slot<SLOT_GRID+CRAFT_CELLS)
	{
		int c=slot-SLOT_GRID;
		if(!inventoryCellActive(c))return false;
		if(tableOpen){*x=GRID3X+(c%3)*INVD; *y=GRID3Y+(c/3)*INVD;}
		else{int k=(c/3)*2+(c%3); *x=grid2X[k]; *y=grid2Y[k];}
		return true;
	}
	if(slot==SLOT_RESULT)
	{
		*x=tableOpen?RESULT3X:RESULT2X;
		*y=tableOpen?RESULT3Y:RESULT2Y;
		return true;
	}
	return false;
}

int inventorySlotAt(int px, int py)
{
	int s, x, y;
	if(py<TOPBAR)return SLOT_NONE;
	for(s=0;s<SLOT_HELD;s++)
	{
		if(!slotPos(s,windowOpen,&x,&y))continue;
		if(px>=x-1 && px<x+17 && py>=y-1 && py<y+17)return s;
	}
	for(s=SLOT_ARMOR;s<SLOT_ARMOR+4;s++)
	{
		if(!slotPos(s,windowOpen,&x,&y))continue;
		if(px>=x-1 && px<x+17 && py>=y-1 && py<y+17)return s;
	}
	if(furnace && windowOpen)
	{
		for(s=SLOT_FURNACE;s<SLOT_FURNACE+FURNACE_SLOTS;s++)
		{
			if(!slotPos(s,true,&x,&y))continue;
			if(px>=x-1 && px<x+17 && py>=y-1 && py<y+17)return s;
		}
	}
	if(chestA && windowOpen)
	{
		for(s=SLOT_CHEST+chestPage*CHEST_VIEW;s<SLOT_CHEST+(chestPage+1)*CHEST_VIEW;s++)
		{
			if(!slotPos(s,true,&x,&y))continue;
			if(px>=x-1 && px<x+17 && py>=y-1 && py<y+17)return s;
		}
		if(chestB && px>=PAGEX-2 && px<PAGEX+18)
		{
			if(py>=PAGEUPY-2 && py<PAGEUPY+18)return SLOT_PAGE_UP;
			if(py>=PAGEDOWNY-2 && py<PAGEDOWNY+18)return SLOT_PAGE_DOWN;
		}
	}
	if(windowOpen && (px<OUTSIDE_LEFT || px>=OUTSIDE_RIGHT))return SLOT_OUTSIDE;
	return SLOT_NONE;
}

static void showSlot(int slot, const stack_struct* s, int x, int y)
{
	u8 sprite=items[slot].id;
	if(!shownValid[slot] || memcmp(&shown[slot],s,sizeof(stack_struct)))
	{
		survivalDrawItemIcon(slot,s);
		shown[slot]=*s;
		shownValid[slot]=true;
	}
	if(isEmpty(s))
	{
		oamSub.oamMemory[sprite].attribute[0]=ATTR0_DISABLED;
		return;
	}
	oamSub.oamMemory[sprite].attribute[0]=ATTR0_BMP | ATTR0_SQUARE | (y&255);
	oamSub.oamMemory[sprite].attribute[1]=ATTR1_SIZE_16 | (x&511);
	oamSub.oamMemory[sprite].attribute[2]=ATTR2_ALPHA(1) | ATTR2_PRIORITY(0) | survivalIconTile(slot);
}

static void hideSlot(int slot)
{
	oamSub.oamMemory[items[slot].id].attribute[0]=ATTR0_DISABLED;
}

// chest slots on screen: their own sprites and icons
static u8 chestSpriteFirst;
static const u8 chestIcons[CHEST_VIEW]={72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,99,100,101};
static stack_struct shownChest[CHEST_VIEW];
static bool shownChestValid[CHEST_VIEW];

// furnace screen: its three slots, the flame and the arrow use the first
// chest sprites and icons (only one container is open at a time)
static stack_struct furnaceShown[FURNACE_SLOTS];
static int furnaceShownFlame, furnaceShownArrow;
static bool furnaceShownValid;
static u16 flameGfx[14*14], arrowGfx[24*16];      // the filled flame and arrow
static bool furnaceGfxLoaded;

void inventoryLoadFurnaceGui(const unsigned char* rgba, int width, int height)
{
	int x, y;
	furnaceGfxLoaded=false;
	if(!rgba || width!=256 || height<31)return;
	// Beta's furnace.png: the flame at 176,0 and the arrow at 176,14
	for(y=0;y<14;y++)for(x=0;x<14;x++)
	{
		const unsigned char* p=&rgba[((176+x)+y*width)*4];
		flameGfx[x+y*14]=p[3]?(RGB15(p[0]>>3,p[1]>>3,p[2]>>3)|BIT(15)):0;
	}
	for(y=0;y<16;y++)for(x=0;x<24;x++)
	{
		const unsigned char* p=&rgba[((176+x)+(14+y)*width)*4];
		arrowGfx[x+y*24]=p[3]?(RGB15(p[0]>>3,p[1]>>3,p[2]>>3)|BIT(15)):0;
	}
	furnaceGfxLoaded=true;
}

// drawn stand-ins for a pack without gui/furnace.png
static void drawFurnaceGfxFallback(void)
{
	int x, y;
	for(y=0;y<14;y++)for(x=0;x<14;x++)
	{
		int w=(y<4)?y:(y<10?4+(y-4)/2:7);         // a flame widening towards the bottom
		bool in=abs(x*2-13)<=w*2-1;
		flameGfx[x+y*14]=in?((y<6)?(RGB15(31,30,8)|BIT(15)):(RGB15(31,16+((x+y)&3)*3,0)|BIT(15))):0;
	}
	for(y=0;y<16;y++)for(x=0;x<24;x++)
	{
		bool shaft=x<16 && y>=5 && y<=10;
		bool head=x>=16 && x<23 && abs(y*2-15)<=(23-x)*2;
		arrowGfx[x+y*24]=(shaft || head)?(RGB15(31,31,31)|BIT(15)):0;
	}
	furnaceGfxLoaded=true;
}

static void drawFurnaceFlame(int icon, int l)
{
	int x, y;
	for(y=0;y<16;y++)for(x=0;x<16;x++)
	{
		// Beta draws rows 12-l .. 13 of the flame
		bool on=l>=0 && x<14 && y<14 && y>=12-l;
		*survivalIconPixel(icon,x,y)=on?flameGfx[x+y*14]:0;
	}
}

static void drawFurnaceArrow(int icon, int part, int w)
{
	int x, y;
	for(y=0;y<16;y++)for(x=0;x<16;x++)
	{
		int ax=part*16+x;
		bool on=ax<24 && ax<w;
		*survivalIconPixel(icon,x,y)=on?arrowGfx[ax+y*24]:0;
	}
}

void inventoryInitChestSprites(u8 first)
{
	int i;
	chestSpriteFirst=first;
	for(i=0;i<CHEST_SPRITES;i++)oamSub.oamMemory[first+i].attribute[0]=ATTR0_DISABLED;
	for(i=0;i<CHEST_VIEW;i++)shownChestValid[i]=false;
}

static void showSprite(u8 sprite, int icon, int x, int y)
{
	oamSub.oamMemory[sprite].attribute[0]=ATTR0_BMP | ATTR0_SQUARE | (y&255);
	oamSub.oamMemory[sprite].attribute[1]=ATTR1_SIZE_16 | (x&511);
	oamSub.oamMemory[sprite].attribute[2]=ATTR2_ALPHA(1) | ATTR2_PRIORITY(0) | survivalIconTile(icon);
}

static void forgetContainerIcons(void)
{
	int i;
	for(i=0;i<CHEST_VIEW;i++)shownChestValid[i]=false;
	furnaceShownValid=false;
}

static void updateChestView(bool open)
{
	int c, x, y;
	for(c=0;c<CHEST_VIEW;c++)
	{
		u8 sprite=chestSpriteFirst+c;
		int slot=SLOT_CHEST+chestPage*CHEST_VIEW+c;
		const stack_struct* s=(open && chestA)?inventorySlot(slot):NULL;
		if(!s || isEmpty(s)){oamSub.oamMemory[sprite].attribute[0]=ATTR0_DISABLED;continue;}
		if(!shownChestValid[c] || memcmp(&shownChest[c],s,sizeof(stack_struct)))
		{
			survivalDrawItemIcon(chestIcons[c],s);
			shownChest[c]=*s;
			shownChestValid[c]=true;
		}
		slotPos(slot,true,&x,&y);
		showSprite(sprite,chestIcons[c],x,y);
	}
	// page arrows: only a double chest has a second page
	if(open && chestB && chestPage==1)showSprite(chestSpriteFirst+CHEST_VIEW,ICON_ARROW_UP,PAGEX,PAGEUPY);
	else oamSub.oamMemory[chestSpriteFirst+CHEST_VIEW].attribute[0]=ATTR0_DISABLED;
	if(open && chestB && chestPage==0)showSprite(chestSpriteFirst+CHEST_VIEW+1,ICON_ARROW_DOWN,PAGEX,PAGEDOWNY);
	else oamSub.oamMemory[chestSpriteFirst+CHEST_VIEW+1].attribute[0]=ATTR0_DISABLED;
}

static void updateFurnaceView(bool open)
{
	int n, x, y;
	if(!open || !furnace)return;                 // the chest view already hid the sprites
	for(n=0;n<FURNACE_SLOTS;n++)
	{
		u8 sprite=chestSpriteFirst+n;
		const stack_struct* s=&furnace->slots[n];
		if(isEmpty(s)){oamSub.oamMemory[sprite].attribute[0]=ATTR0_DISABLED;continue;}
		if(!furnaceShownValid || memcmp(&furnaceShown[n],s,sizeof(stack_struct)))
		{
			survivalDrawItemIcon(chestIcons[n],s);
			furnaceShown[n]=*s;
		}
		slotPos(SLOT_FURNACE+n,true,&x,&y);
		showSprite(sprite,chestIcons[n],x,y);
	}
	if(!furnaceGfxLoaded)drawFurnaceGfxFallback();
	// flame (only while burning) and arrow (always at least one column), as Beta draws them
	{
		int l=furnaceBurning(furnace)?furnaceBurnScaled(furnace,12):-1;
		int w=furnaceCookScaled(furnace,24)+1;
		if(!furnaceShownValid || l!=furnaceShownFlame)drawFurnaceFlame(chestIcons[3],l);
		if(!furnaceShownValid || w!=furnaceShownArrow)
		{
			drawFurnaceArrow(chestIcons[4],0,w);
			drawFurnaceArrow(chestIcons[5],1,w);
		}
		furnaceShownFlame=l;
		furnaceShownArrow=w;
	}
	furnaceShownValid=true;
	showSprite(chestSpriteFirst+3,chestIcons[3],FLAMEX,FLAMEY);
	showSprite(chestSpriteFirst+4,chestIcons[4],ARROWX,ARROWY);
	showSprite(chestSpriteFirst+5,chestIcons[5],ARROWX+16,ARROWY);
}

void inventoryUpdateUI(bool open)
{
	int s, x, y;
	if(!data())return;

	// a window opened or closed by the interface buttons
	if(open && !windowOpen)inventoryOpen(false);
	else if(!open && windowOpen)inventoryClose();

	if(open)
	{
		bool right=keysHeld() & KEY_L, shift=keysHeld() & KEY_R;
		if(keysDown() & KEY_TOUCH)
		{
			inventoryPress(inventorySlotAt(thisXY.px,thisXY.py),right,shift);
			heldX=thisXY.px; heldY=thisXY.py;
		}else if(keysHeld() & KEY_TOUCH)
		{
			heldX=thisXY.px; heldY=thisXY.py;
			inventoryDragOver(inventorySlotAt(thisXY.px,thisXY.py));
		}else if(keysUp() & KEY_TOUCH)
		{
			inventoryRelease(inventorySlotAt(lastXY.px,lastXY.py));
		}
	}else if(keysDown() & KEY_TOUCH)
	{
		s=inventorySlotAt(thisXY.px,thisXY.py);
		if(s>=0 && s<INV_HOTBAR)inventorySelect(s);
	}

	for(s=0;s<SLOT_HELD;s++)
	{
		stack_struct r;
		const stack_struct* st;
		if(!slotPos(s,open,&x,&y)){hideSlot(s);continue;}
		if(s==SLOT_RESULT){craftingResult(&r);st=&r;}
		else st=inventorySlot(s);
		showSlot(s,st,x,y);
	}
	for(s=SLOT_ARMOR;s<SLOT_ARMOR+4;s++)
	{
		if(!slotPos(s,open,&x,&y)){hideSlot(s);continue;}
		showSlot(s,inventorySlot(s),x,y);
	}
	updateChestView(open);
	updateFurnaceView(open);
	if(open && !isEmpty(inventoryHeld()))showSlot(SLOT_HELD,inventoryHeld(),heldX-8,heldY-8);
	else hideSlot(SLOT_HELD);

	// selected hotbar slot frame
	slotPos(inventorySelectedIndex(),open,&x,&y);
	oamSub.oamMemory[cursorSprite].attribute[0]=ATTR0_COLOR_256 | ATTR0_SQUARE | ((y-2)&255);
	oamSub.oamMemory[cursorSprite].attribute[1]=ATTR1_SIZE_32 | ((x-2)&511);

	// the selected item is what the player holds and places
	cursorBlock=inventorySelected()->count?inventorySelected()->item:0;
}

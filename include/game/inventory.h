#ifndef INVENTORY_9
#define INVENTORY_9

// Survival inventory and crafting, following Minecraft Beta 1.7 (the version
// this game and its texture packs come from):
// - 36 slots (9 hotbar + 27 main), stacks of up to 64 (tools and doors: 1)
// - a 2x2 crafting grid in the inventory, 3x3 at a crafting table
// - shaped recipes, mirrored shapes accepted
// - clicks: tap = left click, tap with L held = right click, tap with R held =
//   shift click; dragging over several slots spreads the stack (evenly, or one
//   each with L), as in Minecraft 1.5+; dragging to one slot moves it there
// - tapping outside the window throws the held stack (L: one item)

#define INV_HOTBAR 9
#define INV_SLOTS 36               // 0..8 hotbar, 9..35 main inventory
#define CRAFT_CELLS 9              // 3x3 grid, row-major; the 2x2 grid uses cells 0,1,3,4

// slot ids used by the window
#define SLOT_GRID 36               // 36..44 crafting grid cells
#define SLOT_RESULT 45
#define SLOT_HELD 46               // the stack carried by the stylus (display only)
#define UI_SLOTS 47
#define SLOT_CHEST 47              // 47..100: an open chest (27 slots, 54 for a double chest)
#define CHEST_VIEW 27              // chest slots on screen at a time (a double chest has two pages)
#define SLOT_FURNACE 101           // 101..103: an open furnace (input, fuel, output)
#define SLOT_PAGE_UP -3
#define SLOT_PAGE_DOWN -4
#define CHEST_SPRITES (CHEST_VIEW+2)
#define SLOT_NONE -1
#define SLOT_OUTSIDE -2

#define ITEM_CRAFTING_TABLE 60
#define ITEM_STICK 70
#define ITEM_COAL_ORE 61
#define ITEM_COAL 71
#define MAX_STACK 64

typedef struct
{
	u8 item, count;
	u16 wear;                      // tools only
}stack_struct;

u8 itemMaxStack(u8 item);

// inventory contents
int inventoryAdd(u8 item, int count, u16 wear);   // returns how many did not fit
int inventoryCount(u8 item);
stack_struct* inventorySlot(int slot);            // any slot id, NULL if invalid
stack_struct* inventorySelected(void);
int inventorySelectedIndex(void);
void inventorySelect(int hotbar);

// window
struct chest_s;
struct furnace_s;
void inventoryOpen(bool table);
void inventoryOpenChest(struct chest_s* a, struct chest_s* b);   // b: the second half of a double chest, or NULL
bool inventoryIsChest(void);
void inventoryOpenFurnace(struct furnace_s* f);
bool inventoryIsFurnace(void);
void inventoryLoadFurnaceGui(const unsigned char* rgba, int width, int height);   // the pack's gui/furnace.png
int inventoryChestPage(void);
void inventoryInitChestSprites(u8 firstSprite);
void inventoryClose(void);                         // grid and held stack go back (overflow is thrown)
void inventoryReset(void);                         // forget the window state, touch nothing
bool inventoryIsOpen(void);
bool inventoryIsTable(void);
int inventoryGridSize(void);
bool inventoryCellActive(int cell);
void inventoryClick(int slot, bool rightClick, bool shift);
void inventoryPress(int slot, bool rightClick, bool shift);   // stylus down
void inventoryDragOver(int slot);                             // stylus moved over a slot
void inventoryRelease(int slot);                              // stylus up
stack_struct* inventoryHeld(void);

// crafting
int craftingRecipes(void);
bool craftingResult(stack_struct* out);

// sub-screen display
void inventoryUpdateUI(bool windowOpen);
int inventorySlotAt(int x, int y);

#endif

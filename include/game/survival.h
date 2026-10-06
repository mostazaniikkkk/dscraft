#ifndef SURVIVAL_9
#define SURVIVAL_9

// Survival mode: health, fall damage, drowning, respawn, timed mining and tools.
// The inventory and crafting live in inventory.c. All timers tick at the player
// update rate (30 Hz).

#define SURVIVAL_MAXHEALTH 20      // half-hearts
#define SURVIVAL_HEARTS (SURVIVAL_MAXHEALTH/2)
#define SURVIVAL_MAXAIR 300        // 10 s under water before drowning

// Item ids below 64 are block ids. Tools and the stick use the ids after them.
#define ITEM_TOOL_FIRST 64
#define ITEM_WOOD_PICKAXE 64
#define ITEM_WOOD_SHOVEL 65
#define ITEM_WOOD_AXE 66
#define ITEM_STONE_PICKAXE 67
#define ITEM_STONE_SHOVEL 68
#define ITEM_STONE_AXE 69
#define SURVIVAL_TOOLS 6           // the wooden and stone ones (old saves keep their wear)
#define SURVIVAL_ITEMTYPES 154     // item ids 0..153 (72..91: placed chests and furnaces, 93, 94, 96: leaves and sapling states, 112..143: farm blocks)
#define ARMOR_TILE 99              // atlas tiles 99..102: the armour in hand (items.png column 2, rows 0..3)
#define SURVIVAL_HUD_SPRITES (SURVIVAL_HEARTS*2)   // hearts, then the armour bar
#define IRON_PICKAXE_TILE 93       // items.png 2,6 / 2,5 / 2,7 / 2,8 (pickaxe, shovel, axe, hoe)
#define IRON_INGOT_TILE 97         // items.png 7,1
#define IRON_ORE_TILE 98           // terrain.png 1,2

#define ITEM_PLANKS 7
#define ITEM_LOG 8
static inline bool itemIsLog(u8 item){ return item==ITEM_LOG || item==9 || item==29 || item==30; }
#define ITEM_COBBLESTONE 4

// tool kinds, as used by the block rules
#define TOOLKIND_NONE 0
#define TOOLKIND_PICKAXE 1
#define TOOLKIND_SHOVEL 2
#define TOOLKIND_AXE 3
#define TOOLKIND_HOE 4                // only for the drawn fallback icon

// tools: kind (pickaxe, shovel, axe) and tier (1 wood, 2 stone, 3 iron)
static inline bool isTool(u8 item){ return (item>=ITEM_TOOL_FIRST && item<ITEM_TOOL_FIRST+SURVIVAL_TOOLS) || (item>=ITEM_IRON_PICKAXE && item<=ITEM_IRON_AXE); }
static inline u8 toolKind(u8 tool)
{
	if(tool>=ITEM_IRON_PICKAXE && tool<=ITEM_IRON_AXE)return tool-ITEM_IRON_PICKAXE+1;
	return isTool(tool)?(tool-ITEM_TOOL_FIRST)%3+1:TOOLKIND_NONE;
}
static inline u8 toolTier(u8 tool)
{
	if(tool>=ITEM_IRON_PICKAXE && tool<=ITEM_IRON_AXE)return 3;
	return isTool(tool)?(tool-ITEM_TOOL_FIRST)/3+1:0;
}

#define SURVIVAL_UNBREAKABLE 0xFFFF

// The void: below the bedrock there is empty space. 64 blocks under the bottom
// of the world is the death zone (Minecraft Beta: y < -64): survival players
// take 4 damage every tick there, creative players are brought back up, and
// dropped items are destroyed.
#define VOID_DEATH_DEPTH 64
#define VOID_DAMAGE 4

#include "game/inventory.h"

// Persistent survival state, stored inside the 2048-byte map header.
// Older maps have random bytes there, so the block is validated by magic + checksum.
#define SURVIVAL_HEADER_OFFSET 512
#define SURVIVAL_MAGIC 0x56525553  // "SURV"
#define SURVIVAL_VERSION 4         // 4: worn armour

typedef struct
{
	u32 magic;
	u8 version;
	u8 health;
	u8 selected;                   // hotbar slot in hand
	u8 reserved;
	u16 worldSpawnX, worldSpawnY;
	int32 worldSpawnZ;
	stack_struct slots[INV_SLOTS];
	stack_struct grid[CRAFT_CELLS];
	stack_struct held;
	stack_struct armor[4];         // worn: helmet, chestplate, leggings, boots
	u32 checksum;
}survivalSave_struct;

extern bool cursorValid;           // the crosshair points at a block this tick
extern bool packHasItems;          // the texture pack has gui/items.png

void survivalInitSprites(u8 firstSprite);
void survivalInit(map_struct* m, player_struct* p);
u16* survivalIconPixel(int id, int x, int y);   // pixel of a sub-screen icon
bool survivalEat(void);                         // eat the selected food (apple, carrot); false: none
int survivalArmorValue(void);                   // 0..20, Minecraft Beta's armour bar
void survivalLoadArmorIcons(const unsigned char* rgba, int width, int height);   // the pack's gui/icons.png
void survivalWearSelected(void);                // the selected hoe was used once
void survivalKill(void);
void survivalUpdate(player_struct* p);
void survivalUpdateHUD(bool inventoryOpen);
void survivalWriteHeader(map_struct* m);
survivalSave_struct* survivalData(void);   // NULL outside survival
void survivalFormatHeader(u8* header);     // make a new world's header a survival world

bool survivalEnabled(void);
bool survivalInputLocked(void);
bool survivalCanPlace(u8 item);
void survivalConsume(u8 item);
bool survivalCanBreak(u8 block);
void survivalDamage(u8 amount);

// mining and tools
u16 survivalBreakTicks(u8 block, u8 tool);
bool survivalCanHarvest(u8 block, u8 tool);
u8 survivalHeldTool(void);
bool survivalMine(void);           // call every tick while digging; true when the block breaks
void survivalStopMining(void);
u16 survivalMiningTicks(void);
u8 survivalMiningProgress(void);   // 0..255
void survivalBlockBroken(u8 block, int i, int j, int k);
u16 survivalToolDurability(u8 tool);

// icons (sub-screen sprite VRAM)
u16 survivalIconTile(int id);
void survivalDrawItemIcon(int dst, const stack_struct* s);

#endif

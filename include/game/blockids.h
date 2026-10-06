#ifndef BLOCKIDS_9
#define BLOCKIDS_9

// Block and item ids of the plants and the farm, shared by the game and the
// host-side generator. Minecraft keeps a plant's stage or a farmland's
// moisture in block metadata; here they are part of the id.
//
// Items that show an icon must stay below 112 (the icons' room in sub-screen
// sprite memory); block states that are never held use the ids from 112.

#define LEAVES_BLOCK 10            // natural leaves
#define LEAVES_DECAY 93            // natural leaves marked for a decay check (metadata bit 8)
#define LEAVES_PLACED 94           // placed by the player: never decay
#define ITEM_SAPLING 95            // also the planted sapling
#define SAPLING_GROWN 96           // second growth stage (metadata bit 8)
#define ITEM_APPLE 97

#define ITEM_CARROT 98             // planted on farmland, or eaten
#define ITEM_PUMPKIN_SEEDS 99
#define ITEM_WOOD_HOE 100
#define ITEM_STONE_HOE 101
#define ITEM_PUMPKIN 102           // placed as a PUMPKIN_FIRST state facing the player

#define ITEM_IRON_ORE 103          // a block, held and placed as itself
#define ITEM_IRON_INGOT 106        // (104, 105: icons of the scroll arrows)
#define ITEM_IRON_PICKAXE 107      // iron tools: pickaxe, shovel, axe as the wooden and stone ones
#define ITEM_IRON_SHOVEL 108
#define ITEM_IRON_AXE 109
#define ITEM_IRON_HOE 110

#define ITEM_IRON_HELMET 150       // iron armour: helmet, chestplate, leggings, boots
#define ITEM_IRON_BOOTS 153
#define ARMOR_ICON_SLOT 52         // their pictures are drawn in icons 52..55 (free when items are loaded)

#define FARMLAND_FIRST 112         // + moisture 0..7 (0: dry)
#define CARROT_FIRST 120           // + growth stage 0..7
#define STEM_FIRST 128             // pumpkin stem, + growth stage 0..7
#define STEM_ATTACHED 136          // grown stem bent towards its pumpkin: + side (+x, -x, +y, -y)
#define PUMPKIN_FIRST 140          // + facing (+x, -x, +y, -y), as chests and furnaces

static inline bool isLeaves(u8 t){ return t==LEAVES_BLOCK || t==LEAVES_DECAY || t==LEAVES_PLACED; }
static inline bool isSapling(u8 t){ return t==ITEM_SAPLING || t==SAPLING_GROWN; }
static inline bool isFarmland(u8 t){ return t>=FARMLAND_FIRST && t<FARMLAND_FIRST+8; }
static inline bool isCarrotCrop(u8 t){ return t>=CARROT_FIRST && t<CARROT_FIRST+8; }
static inline bool isStem(u8 t){ return t>=STEM_FIRST && t<STEM_ATTACHED+4; }
static inline bool isAttachedStem(u8 t){ return t>=STEM_ATTACHED && t<STEM_ATTACHED+4; }
static inline bool isPumpkin(u8 t){ return t>=PUMPKIN_FIRST && t<PUMPKIN_FIRST+4; }
static inline bool isHoe(u8 t){ return t==ITEM_WOOD_HOE || t==ITEM_STONE_HOE || t==ITEM_IRON_HOE; }
// held items above 63 that are blocks (drawn as cubes, placed)
static inline bool isArmor(u8 t){ return t>=ITEM_IRON_HELMET && t<=ITEM_IRON_BOOTS; }
static inline int armorType(u8 t){ return t-ITEM_IRON_HELMET; }     // 0 helmet .. 3 boots
static inline bool isCubeItem(u8 t){ return t==ITEM_PUMPKIN || t==ITEM_IRON_ORE; }
// drawn as crossed planes, walked through, see-through
static inline bool isPlant(u8 t){ return isSapling(t) || isCarrotCrop(t) || isStem(t); }

#endif

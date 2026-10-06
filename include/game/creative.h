#ifndef CREATIVE_9
#define CREATIVE_9

// Creative inventory, as in Minecraft: a scrolling catalogue with every block,
// and the item bar. Dragging a block from the catalogue to the bar copies it
// (the catalogue never runs out), dragging between bar slots swaps them and
// dropping a bar slot on the catalogue empties it.

#define CREATIVE_ROWS 3            // catalogue rows on screen (9 blocks each)

#define ICON_ARROW_UP 104          // scroll / page arrows (also used by the chest screen)
#define ICON_ARROW_DOWN 105

void creativeInit(void);
void creativeDrawArrowIcons(void);
void creativeUpdateUI(bool inventoryOpen);
void creativeSelect(int hotbar);
int creativeCatalogueSize(void);
int creativeScroll(void);
void creativeSetScroll(int row);
u8 creativeHotbar(int slot);

#endif

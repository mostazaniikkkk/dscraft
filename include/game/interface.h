#ifndef INTERFACE_9
#define INTERFACE_9

#define MAXITEMS 56                // the inventory's 51 slots (and the creative catalogue) fit
#define MAXSLOTS 64

u8 usedSprites;

typedef struct
{
	vect3D position;
	u8 id, type;
	s8 slot;
	bool used;
}item_struct;

typedef struct
{
	vect3D position;
	s8 id;
	bool used;
}slot_struct;

item_struct items[MAXITEMS];
slot_struct slots[MAXSLOTS];

bool invOpen, overButtons;
bool gamePaused;                  // the game menu is open (START)
void gamePause(bool on);
void pauseUpdate(void);           // the game menu's input, instead of the controls
void takeScreenshot(void);        // game_main.c: at the start of the next frame

void initItems(void);
void initItemBar(void);
void initInterface(void);
bool updateInterface(void);
void interfaceOpenCraftingTable(void);
void loadInterface(char* filename, u8 prio);
void startSave(void);
void endSave(void);

#endif
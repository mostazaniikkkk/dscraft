#include "game/game_main.h"
#include <maxmod9.h>
#include "soundbank.h"

#define IBARY 170
#define IBARD 20
#define IBARX (39+IBARD)

#define COMPX 8
#define COMPY 34

#define CLOCKX 8
#define CLOCKY 78

#define TOGGLEX 0
#define TOGGLEY 1
#define TOGGLESX 96
#define TOGGLESY 24

#define SAVEX 187
#define SAVEY 1
#define SAVESX (256-SAVEX)
#define SAVESY 24

#define INVX 97
#define INVY 1
#define INVSX 90
#define INVSY 24

#define INVENTORYX 55
#define INVENTORYY 110
#define INVENTORYOX 18
#define INVENTORYOY 18

#define INVENTORYBX 55
#define INVENTORYBY 168
#define INVENTORYBOX 18

u8 itemBar[]={1,3,4,13,6,7,8,11,12}; //water test
u8 inventoryItem[]={14,15,16,17,18,19,20,21,22,
					23,24,25,26,27,28,29,30,31,
					32,33,34,35,36,40,DOORTYPE,39,2};
u8 inventoryItems=27;

u8 cursorSprite, buttonSprite;

//from libnds (heavily modified)
void loadImageDirect(char* filename)
{
	FILE* f=DS_OpenFile(filename, "", false, true);
	vramSetBankI(VRAM_I_LCD);
	fread(VRAM_I_EXT_SPR_PALETTE[0],2,256,f);
	vramSetBankI(VRAM_I_SUB_SPRITE_EXT_PALETTE);
	fread(SPRITE_GFX_SUB,256*256,1,f);
	fclose(f);
}

void initItems(void)
{
	int i;
	for(i=0;i<MAXITEMS;i++)
	{
		items[i].id=usedSprites;
		items[i].used=false;
		oamSub.oamMemory[items[i].id].attribute[0] = ATTR0_DISABLED;
		usedSprites++;
	}
}

void initSlots(void)
{
	int i;
	for(i=0;i<MAXSLOTS;i++)
	{
		slots[i].id=-1;
		slots[i].used=false;
	}
}

void setItemPosition(u8 id, u8 x, u8 y)
{
	items[id].position.x=x;
	items[id].position.y=y;
	oamSub.oamMemory[items[id].id].attribute[0] = ATTR0_BMP | ATTR0_SQUARE | (items[id].position.y);
	oamSub.oamMemory[items[id].id].attribute[1] = ATTR1_SIZE_16 | (items[id].position.x);
	oamSub.oamMemory[items[id].id].attribute[2] = ATTR2_ALPHA(1) | ATTR2_PRIORITY(0) | (8*2*(36+((items[id].type-(items[id].type%8))/8)*2)+2*(items[id].type%8));
}

void initItemBar(void)
{
	int i;
	for(i=0;i<9;i++)
	{
		items[i].type=itemBar[i];
		items[i].slot=i;
		items[i].used=true;
		
		slots[i].id=i;
		slots[i].position.x=IBARX+i*IBARD;
		slots[i].position.y=IBARY;
		slots[i].used=true;
		
		setItemPosition(i, slots[items[i].slot].position.x, slots[items[i].slot].position.y);
	}
}

void initInventory(void)
{
	int i, j;
	int id=9;
	for(j=0;j<3;j++)
	{
		for(i=0;i<9;i++)
		{
			slots[id].id=id;
			slots[id].position.x=INVENTORYX+i*INVENTORYOX;
			slots[id].position.y=INVENTORYY+j*INVENTORYOY;
			slots[id].used=false;
			id++;
		}
	}
	for(i=0;i<inventoryItems;i++)
	{
		items[9+i].type=inventoryItem[i];
		items[9+i].slot=9+i;
		items[9+i].used=true;
		
		setItemPosition(i, slots[items[i].slot].position.x, slots[items[i].slot].position.y);
	}
}

void loadInterface(char* filename, u8 prio)
{
	int x,y;
	
	loadImageDirect(filename);
	
	int id=0;
	for(y = 0; y < 3; y++)
	{
		for(x = 0; x < 4; x++)
		{
			oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_SQUARE | (64 * y);
			oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (64 * x);
			oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(prio) | ATTR2_PALETTE(0) | (8 * 2 * (x+4*y));
			id++;
		}
	}
}

void initInterface(void)
{
	oamInit(&oamSub, SpriteMapping_1D_256, true);
 
	int x, y, i;
 
	usedSprites=0;
	int id = 0;

	for(y = 0; y < 3; y++)
	{
		for(x = 0; x < 4; x++)
		{
			oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_SQUARE | (64 * y);
			oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (64 * x);
			oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(1) | ATTR2_PALETTE(0) | (8 * 2 * (x+4*y));
			id++;
		}
	}
	{
		buttonSprite=id;
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		oamSub.oamMemory[id].attribute[0] = ATTR0_DISABLED;
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (0);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (8 * 2 * (0+4*3));
		id++;
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		oamSub.oamMemory[id].attribute[0] = ATTR0_DISABLED;
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (64);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (8 * 2 * (1+4*3));
		id++;
		
		
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		oamSub.oamMemory[id].attribute[0] = ATTR0_DISABLED;
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (96);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (8 * 2 * (0+4*3)+8);
		id++;
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		oamSub.oamMemory[id].attribute[0] = ATTR0_DISABLED;
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (96+64);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (8 * 2 * (1+4*3)+8);
		id++;
		
		
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		oamSub.oamMemory[id].attribute[0] = ATTR0_DISABLED;
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (128);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (8 * 2 * (2+4*3));
		id++;
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		oamSub.oamMemory[id].attribute[0] = ATTR0_DISABLED;
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (128+64);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (8 * 2 * (3+4*3));
		id++;
		
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (80);
		oamSub.oamMemory[id].attribute[0] = ATTR0_DISABLED;
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (128-64);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (8 * 2 * (2+4*3)+8);
		id++;
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (80);
		oamSub.oamMemory[id].attribute[0] = ATTR0_DISABLED;
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (128-64+64);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (8 * 2 * (3+4*3)+8);
		id++;
	}
	{
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_SQUARE | ATTR0_ROTSCALE_DOUBLE | (COMPY);
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_16 | ATTR1_ROTDATA(0) | (COMPX);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(3) | (8 * 2 * (0+4*4));
		id++;
	}
	{
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_SQUARE | ATTR0_ROTSCALE | (CLOCKY);
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_32 | ATTR1_ROTDATA(1) | (CLOCKX);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(2) | ATTR2_PALETTE(1) | (8 * 2 * (0+4*4)+1);
		id++;
	}
	{
		oamSub.oamMemory[id].attribute[0] = ATTR0_COLOR_256 | ATTR0_SQUARE | (0);
		oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_32 | (0);
		oamSub.oamMemory[id].attribute[2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(2) | (8 * 2 * (0+4*4)+5);
		cursorSprite=id;
		id++;
	}
	survivalInitSprites(id);
	id+=SURVIVAL_HUD_SPRITES;
	inventoryInitChestSprites(id);
	id+=CHEST_SPRITES;
	usedSprites=id;
	initItems();
	initSlots();
	initItemBar();
	initInventory();
	
	loadInterface("loading.bin",0);

	sImage pcx2;
	u8* buffer2=DS_OpenFile("compass.pcx", "", true, true);
	loadPCX((u8*)buffer2, &pcx2);
	free(buffer2);
	vramSetBankI(VRAM_I_LCD);
	for(i=0;i<256;i++)VRAM_I_EXT_SPR_PALETTE[3][i]  = pcx2.palette[i];
	vramSetBankI(VRAM_I_SUB_SPRITE_EXT_PALETTE);
    imageTileData(&pcx2);
	for(i=0;i<64*2;i++)SPRITE_GFX_SUB[i+256*128]=pcx2.image.data16[i];
	imageDestroy(&pcx2);
	buffer2=DS_OpenFile("daynight.pcx", "", true, true);
	loadPCX((u8*)buffer2, &pcx2);
	free(buffer2);
	vramSetBankI(VRAM_I_LCD);
	for(i=0;i<256;i++)VRAM_I_EXT_SPR_PALETTE[1][i]  = pcx2.palette[i];
	vramSetBankI(VRAM_I_SUB_SPRITE_EXT_PALETTE);
    imageTileData(&pcx2);
	for(i=0;i<64*8;i++)SPRITE_GFX_SUB[i+256*128+64*2]=pcx2.image.data16[i];
	imageDestroy(&pcx2);
	buffer2=DS_OpenFile("cursor.pcx", "", true, true);
	loadPCX((u8*)buffer2, &pcx2);
	free(buffer2);
	vramSetBankI(VRAM_I_LCD);
	for(i=0;i<256;i++)VRAM_I_EXT_SPR_PALETTE[2][i]  = pcx2.palette[i];
	vramSetBankI(VRAM_I_SUB_SPRITE_EXT_PALETTE);
    imageTileData(&pcx2);
	for(i=0;i<64*8;i++)SPRITE_GFX_SUB[i+256*128+64*10]=pcx2.image.data16[i];
	imageDestroy(&pcx2);

	swiWaitForVBlank();
 
	oamUpdate(&oamSub);

	invOpen=false;
}

bool updateInterface(void)
{
	int i;

	oamRotateScale(&oamSub, 0, -Player.angleZ, intToFixed(1, 8), intToFixed(1, 8));
	oamRotateScale(&oamSub, 1, sunX+8192, intToFixed(1, 8), intToFixed(1, 8));
	oamUpdate(&oamSub);

	// item bar and inventory: survival.c/inventory.c or creative.c draw them
	if(!survivalEnabled())creativeUpdateUI(invOpen);
	if(!invOpen && (keysDown() & KEY_TOUCH))overButtons=thisXY.py<TOGGLEY+TOGGLESY;
	else if(invOpen)overButtons=true;
	if((keysHeld() & KEY_TOUCH) && thisXY.px>=CLOCKX && thisXY.py>=CLOCKY && thisXY.px<CLOCKX+32 && thisXY.py<CLOCKY+16)
	{
		sunX+=600;
		cloudcnt+=200;
	}
	if(overButtons)
	{
		if((keysUp() & KEY_TOUCH) && lastXY.px>=TOGGLEX && lastXY.py>=TOGGLEY && lastXY.px<TOGGLEX+TOGGLESX && lastXY.py<TOGGLEY+TOGGLESY)
		{
			action^=1;
		}else if(!action){oamSub.oamMemory[buttonSprite+2*0].attribute[0] = ATTR0_DISABLED;oamSub.oamMemory[buttonSprite+2*0+1].attribute[0] = ATTR0_DISABLED;}
		if(action || ((keysHeld() & KEY_TOUCH) && thisXY.px>=TOGGLEX && thisXY.py>=TOGGLEY && thisXY.px<TOGGLEX+TOGGLESX && thisXY.py<TOGGLEY+TOGGLESY))
		{
			oamSub.oamMemory[buttonSprite+2*0].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
			oamSub.oamMemory[buttonSprite+2*0+1].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		}
		
		if((keysHeld() & KEY_TOUCH) && thisXY.px>=INVX && thisXY.py>=INVY && thisXY.px<INVX+INVSX && thisXY.py<INVY+INVSY)
		{
			oamSub.oamMemory[buttonSprite+2*1].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
			oamSub.oamMemory[buttonSprite+2*1+1].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		}else if((keysUp() & KEY_TOUCH) && lastXY.px>=INVX && lastXY.py>=INVY && lastXY.px<INVX+INVSX && lastXY.py<INVY+INVSY)
		{
			if(invOpen)
			{
				loadInterface("interface.bin",1);
				for(i=0;i<9;i++)
				{
					slots[i].position.x=IBARX+i*IBARD;
					slots[i].position.y=IBARY;
				}
				for(i=9;i<9+9*3;i++)
				{
					slots[i].used=false;
				}
			}
			else
			{
				loadInterface("inventory.bin",1);
				for(i=0;i<9;i++)
				{
					slots[i].position.x=INVENTORYBX+i*INVENTORYBOX;
					slots[i].position.y=INVENTORYBY;
				}
				for(i=9;i<9+9*3;i++)
				{
					slots[i].used=true;
				}
			}
			invOpen^=1;
		}else{oamSub.oamMemory[buttonSprite+2*1].attribute[0] = ATTR0_DISABLED;oamSub.oamMemory[buttonSprite+2*1+1].attribute[0] = ATTR0_DISABLED;}
		
		if((keysHeld() & KEY_TOUCH) && thisXY.px>=SAVEX && thisXY.py>=SAVEY && thisXY.px<SAVEX+SAVESX && thisXY.py<SAVEY+SAVESY)
		{
			oamSub.oamMemory[buttonSprite+2*2].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
			oamSub.oamMemory[buttonSprite+2*2+1].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (0);
		}else if((keysUp() & KEY_TOUCH) && lastXY.px>=SAVEX && lastXY.py>=SAVEY && lastXY.px<SAVEX+SAVESX && lastXY.py<SAVEY+SAVESY)
		{
			globalSaveMap(&map);
		}else{oamSub.oamMemory[buttonSprite+2*2].attribute[0] = ATTR0_DISABLED;oamSub.oamMemory[buttonSprite+2*2+1].attribute[0] = ATTR0_DISABLED;}
	}
	survivalUpdateHUD(invOpen);
	return !invOpen;
}

// Opened by using a crafting table: the inventory screen with a 3x3 grid.
void interfaceOpenCraftingTable(void)
{
	int i;
	if(invOpen)return;
	loadInterface("crafting.bin",1);
	for(i=0;i<9;i++)
	{
		slots[i].position.x=INVENTORYBX+i*INVENTORYBOX;
		slots[i].position.y=INVENTORYBY;
	}
	for(i=9;i<9+9*3;i++)slots[i].used=true;
	invOpen=true;
	overButtons=true;
	inventoryOpen(true);
}

// Opened by using a chest: its slots (two pages for a double chest) above the inventory.
void interfaceOpenChest(chest_struct* a, chest_struct* b)
{
	int i;
	if(invOpen)return;
	loadInterface("chest.bin",1);
	for(i=0;i<9;i++)
	{
		slots[i].position.x=INVENTORYBX+i*INVENTORYBOX;
		slots[i].position.y=INVENTORYBY;
	}
	for(i=9;i<9+9*3;i++)slots[i].used=true;
	invOpen=true;
	overButtons=true;
	inventoryOpenChest(a,b);
}

// Opened by using a furnace: input, fuel and output above the inventory.
void interfaceOpenFurnace(furnace_struct* f)
{
	int i;
	if(invOpen)return;
	loadInterface("furnace.bin",1);
	for(i=0;i<9;i++)
	{
		slots[i].position.x=INVENTORYBX+i*INVENTORYBOX;
		slots[i].position.y=INVENTORYBY;
	}
	for(i=9;i<9+9*3;i++)slots[i].used=true;
	invOpen=true;
	overButtons=true;
	inventoryOpenFurnace(f);
}

/* ---------------------------------------------------------------------------
 * Game menu (START), as Minecraft's: back to the game, a screenshot, or save
 * and quit to the title screen. The game stands still while it is open.
 * ------------------------------------------------------------------------- */

#define PAUSE_BX 59                // buttons drawn in pause.bin (tools/make_pause_bin.py)
#define PAUSE_BW 152
#define PAUSE_BH 20
static const u8 pauseButtonY[3]={112,136,160};
static u16 pauseOam[128][3];
static touchPosition pauseTouch;

// an open inventory, crafting table, chest or furnace closes first, as in Minecraft
static void closeWindow(void)
{
	int i;
	loadInterface("interface.bin",1);
	for(i=0;i<9;i++)
	{
		slots[i].position.x=IBARX+i*IBARD;
		slots[i].position.y=IBARY;
	}
	for(i=9;i<9+9*3;i++)slots[i].used=false;
	invOpen=false;
	if(survivalEnabled())inventoryClose();
}

void gamePause(bool on)
{
	int i;
	if(on==gamePaused)return;
	if(on)
	{
		if(invOpen)closeWindow();
		for(i=0;i<128;i++)
		{
			pauseOam[i][0]=oamSub.oamMemory[i].attribute[0];
			pauseOam[i][1]=oamSub.oamMemory[i].attribute[1];
			pauseOam[i][2]=oamSub.oamMemory[i].attribute[2];
		}
		// the screen's background, the compass and the clock stay
		for(i=buttonSprite;i<buttonSprite+8;i++)oamSub.oamMemory[i].attribute[0]=ATTR0_DISABLED;
		for(i=cursorSprite;i<128;i++)oamSub.oamMemory[i].attribute[0]=ATTR0_DISABLED;
		loadInterface("pause.bin",1);
	}else{
		loadInterface("interface.bin",1);
		for(i=12;i<128;i++)
		{
			oamSub.oamMemory[i].attribute[0]=pauseOam[i][0];
			oamSub.oamMemory[i].attribute[1]=pauseOam[i][1];
			oamSub.oamMemory[i].attribute[2]=pauseOam[i][2];
		}
	}
	gamePaused=on;
	oamUpdate(&oamSub);
}

void pauseUpdate(void)
{
	int b;
	scanKeys();
	if(keysDown() & KEY_START){gamePause(false);return;}
	if(keysHeld() & KEY_TOUCH){touchRead(&pauseTouch);return;}
	if(!(keysUp() & KEY_TOUCH))return;
	// a button acts when the stylus is lifted on it
	for(b=0;b<3;b++)
		if(pauseTouch.px>=PAUSE_BX && pauseTouch.px<PAUSE_BX+PAUSE_BW && pauseTouch.py>=pauseButtonY[b] && pauseTouch.py<pauseButtonY[b]+PAUSE_BH)break;
	switch(b)
	{
		case 0:
			gamePause(false);
			break;
		case 1:
			mmEffect(SFX_ADD);
			takeScreenshot();
			break;
		case 2:
			gamePause(false);
			globalSaveMap(&map);
			DS_ChangeState(&Menu_State);
			break;
	}
}

void startSave(void)
{
	oamSub.oamMemory[buttonSprite+2*3].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (80);
	oamSub.oamMemory[buttonSprite+2*3+1].attribute[0] = ATTR0_COLOR_256 | ATTR0_WIDE | (80);
	oamUpdate(&oamSub);
}

void endSave(void)
{
	oamSub.oamMemory[buttonSprite+2*3].attribute[0] = ATTR0_DISABLED;
	oamSub.oamMemory[buttonSprite+2*3+1].attribute[0] = ATTR0_DISABLED;
	oamUpdate(&oamSub);
}

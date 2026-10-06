#include "game/game_main.h"

// Catalogue: every block a creative player can place, in block id order,
// then the ladder, the door and the crafting table.
static u8 catalogue[64];
static int catalogueSize;

static u8 hotbar[INV_HOTBAR];
static int selected;
static int scroll;

static bool dragging;
static u8 dragType;
static int dragFrom;               // hotbar slot, or -1 from the catalogue
static int heldX, heldY;

#define HOTBARX 59                 // item bar with the window closed
#define HOTBARY 170
#define HOTBARD 20
#define INVX 55                    // inventory screen
#define INVY 110
#define INVBARY 168
#define INVD 18
#define ARROWX 236                 // scroll arrows, right of the catalogue
#define ARROWUPY 110
#define ARROWDOWNY 146

#define HIT_UP 100
#define HIT_DOWN 101

#define ICON_BASE (256*288/2)

extern u8 cursorSprite;

static const u8 defaultHotbar[INV_HOTBAR]={1,3,4,13,6,7,8,11,12};

static inline u16* iconPixel(int id, int x, int y)
{
	return &SPRITE_GFX_SUB[ICON_BASE+x+(id%8)*16+(y+(id/8)*16)*128];
}

// a white triangle with a black outline, pointing up or down
static void drawArrow(int id, bool up)
{
	int x, y;
	for(y=0;y<16;y++)for(x=0;x<16;x++)
	{
		int row=up?y:15-y;                 // 0 at the tip
		int half=row/2+1;
		bool inside=row>=2 && row<14 && abs(x*2-15)<half*2;
		bool edge=row>=1 && row<15 && abs(x*2-15)<half*2+3 && !inside;
		*iconPixel(id,x,y)=inside?(RGB15(31,31,31)|BIT(15)):(edge?(RGB15(0,0,0)|BIT(15)):0);
	}
}

void creativeDrawArrowIcons(void)
{
	drawArrow(ICON_ARROW_UP,true);
	drawArrow(ICON_ARROW_DOWN,false);
}

int creativeCatalogueSize(void){ return catalogueSize; }
int creativeScroll(void){ return scroll; }
u8 creativeHotbar(int slot){ return (slot>=0 && slot<INV_HOTBAR)?hotbar[slot]:0; }

static int maxScroll(void)
{
	int rows=(catalogueSize+8)/9;
	return rows>CREATIVE_ROWS?rows-CREATIVE_ROWS:0;
}

void creativeSetScroll(int row)
{
	if(row<0)row=0;
	if(row>maxScroll())row=maxScroll();
	scroll=row;
}

void creativeSelect(int slot)
{
	if(slot>=0 && slot<INV_HOTBAR)selected=slot;
	cursorBlock=hotbar[selected];
}

void creativeInit(void)
{
	int t;
	catalogueSize=0;
	for(t=1;t<LADDERTYPE;t++)catalogue[catalogueSize++]=t;
	catalogue[catalogueSize++]=LADDERTYPE;
	catalogue[catalogueSize++]=DOORTYPE;
	catalogue[catalogueSize++]=ITEM_CRAFTING_TABLE;
	catalogue[catalogueSize++]=ITEM_COAL_ORE;
	catalogue[catalogueSize++]=ITEM_IRON_ORE;
	catalogue[catalogueSize++]=ITEM_CHEST;
	catalogue[catalogueSize++]=ITEM_FURNACE;
	catalogue[catalogueSize++]=ITEM_SAPLING;
	catalogue[catalogueSize++]=ITEM_PUMPKIN;
	catalogue[catalogueSize++]=ITEM_CARROT;
	catalogue[catalogueSize++]=ITEM_PUMPKIN_SEEDS;
	catalogue[catalogueSize++]=ITEM_WOOD_HOE;
	memcpy(hotbar,defaultHotbar,sizeof(hotbar));
	scroll=0;
	dragging=false;
	creativeSelect(0);
	creativeDrawArrowIcons();
}

// slot under the stylus: hotbar 0..8, catalogue cells 9..35, arrows, or -1
static int hitTest(int px, int py, bool open)
{
	int s;
	for(s=0;s<INV_HOTBAR;s++)
	{
		int x=open?INVX+s*INVD:HOTBARX+s*HOTBARD, y=open?INVBARY:HOTBARY;
		if(px>=x-1 && px<x+17 && py>=y-1 && py<y+17)return s;
	}
	if(!open)return -1;
	for(s=0;s<CREATIVE_ROWS*9;s++)
	{
		int x=INVX+(s%9)*INVD, y=INVY+(s/9)*INVD;
		if(px>=x-1 && px<x+17 && py>=y-1 && py<y+17)return INV_HOTBAR+s;
	}
	if(px>=ARROWX-2 && px<ARROWX+18)
	{
		if(py>=ARROWUPY-2 && py<ARROWUPY+18)return HIT_UP;
		if(py>=ARROWDOWNY-2 && py<ARROWDOWNY+18)return HIT_DOWN;
	}
	return -1;
}

static u8 catalogueAt(int cell)
{
	int n=scroll*9+cell;
	return (n>=0 && n<catalogueSize)?catalogue[n]:0;
}

static void showSprite(int k, u8 icon, int x, int y)
{
	u8 sprite=items[k].id;
	if(!icon)
	{
		oamSub.oamMemory[sprite].attribute[0]=ATTR0_DISABLED;
		return;
	}
	oamSub.oamMemory[sprite].attribute[0]=ATTR0_BMP | ATTR0_SQUARE | (y&255);
	oamSub.oamMemory[sprite].attribute[1]=ATTR1_SIZE_16 | (x&511);
	oamSub.oamMemory[sprite].attribute[2]=ATTR2_ALPHA(1) | ATTR2_PRIORITY(0) | survivalIconTile(icon);
}

static void press(int hit)
{
	if(hit==HIT_UP){creativeSetScroll(scroll-1);return;}
	if(hit==HIT_DOWN){creativeSetScroll(scroll+1);return;}
	if(hit<0)return;
	if(hit<INV_HOTBAR)
	{
		creativeSelect(hit);
		if(!hotbar[hit])return;
		dragType=hotbar[hit];
		dragFrom=hit;
	}else{
		dragType=catalogueAt(hit-INV_HOTBAR);
		dragFrom=-1;
		if(!dragType)return;
	}
	dragging=true;
}

static void release(int hit)
{
	if(!dragging)return;
	dragging=false;
	if(hit>=0 && hit<INV_HOTBAR)
	{
		if(dragFrom>=0 && dragFrom!=hit)
		{
			// bar to bar: swap
			u8 t=hotbar[hit];
			hotbar[hit]=hotbar[dragFrom];
			hotbar[dragFrom]=t;
		}else if(dragFrom<0)hotbar[hit]=dragType;   // catalogue to bar: a copy
		creativeSelect(hit);
	}else if(hit>=INV_HOTBAR && hit<INV_HOTBAR+CREATIVE_ROWS*9 && dragFrom>=0)
	{
		hotbar[dragFrom]=0;                          // bar to catalogue: gone
		creativeSelect(selected);
	}
}

void creativeUpdateUI(bool open)
{
	int s;
	if(keysDown() & KEY_TOUCH)
	{
		int hit=hitTest(thisXY.px,thisXY.py,open);
		if(open)press(hit);
		else if(hit>=0 && hit<INV_HOTBAR)creativeSelect(hit);
		heldX=thisXY.px; heldY=thisXY.py;
	}else if(keysHeld() & KEY_TOUCH)
	{
		heldX=thisXY.px; heldY=thisXY.py;
	}else if(keysUp() & KEY_TOUCH)
	{
		release(hitTest(lastXY.px,lastXY.py,open));
	}
	if(!open)dragging=false;

	for(s=0;s<INV_HOTBAR;s++)
		showSprite(s,hotbar[s],open?INVX+s*INVD:HOTBARX+s*HOTBARD,open?INVBARY:HOTBARY);
	for(s=0;s<CREATIVE_ROWS*9;s++)
		showSprite(INV_HOTBAR+s,open?catalogueAt(s):0,INVX+(s%9)*INVD,INVY+(s/9)*INVD);
	showSprite(36,(open && dragging)?dragType:0,heldX-8,heldY-8);
	showSprite(37,(open && scroll>0)?ICON_ARROW_UP:0,ARROWX,ARROWUPY);
	showSprite(38,(open && scroll<maxScroll())?ICON_ARROW_DOWN:0,ARROWX,ARROWDOWNY);

	// selected bar slot frame
	{
		int x=open?INVX+selected*INVD:HOTBARX+selected*HOTBARD, y=open?INVBARY:HOTBARY;
		oamSub.oamMemory[cursorSprite].attribute[0]=ATTR0_COLOR_256 | ATTR0_SQUARE | ((y-2)&255);
		oamSub.oamMemory[cursorSprite].attribute[1]=ATTR1_SIZE_32 | ((x-2)&511);
	}
	cursorBlock=hotbar[selected];
}

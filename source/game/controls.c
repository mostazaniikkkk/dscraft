#include "game/game_main.h"

#include <maxmod9.h>

#include "soundbank.h"
#include "soundbank_bin.h"

// Jump speed: with GRAVITY 70 per tick this reaches 1.26 blocks, Minecraft's jump
// height (1.25). The original 850 only reached 1.16, too little to jump and
// place a block under yourself.
#define JUMP_SPEED 885

vect3D tempAngle;
u8 oldwAngle;
u8 walkSfx;

void initControls(void)
{
	lastXY=(touchPosition){0,0,0,0};
	action=0;
}

// Returns true when a block was actually placed (opening a door is not placing).
bool placeBlock(void)
{
	map_struct* m=&map;
	int i=testCursorI, j=testCursorJ, k=testCursorK;
	// the cursor keeps the last block it found: act only while it points at one
	if(!cursorValid)return false;
	u8 ot=*getBlockP(m,i,j,k);
	// sneaking with something in hand places against the block instead of using it
	bool activate=!Player.sneaking || !cursorBlock;
	if(activate && survivalEnabled() && ot==ITEM_CRAFTING_TABLE)
	{
		interfaceOpenCraftingTable();
		return false;
	}
	if(activate && chestOpen(m,i,j,k))return false;
	if(activate && furnaceOpen(m,i,j,k))return false;
	if(activate && ot>=DOORTYPE && ot<DOORTYPE+8)
	{
		mmEffect(SFX_ADD);
		if((ot-DOORTYPE)%2)k++;
		ot=(ot-DOORTYPE-((ot-DOORTYPE)%2))/2;
		removeBlock(m, i, j, k, false);
		removeBlock(m, i, j, k-1, false);
		*getBlockP(m,i,j,k)+=8;
		*getBlockP(m,i,j,k-1)+=8;
		u8 dir;
		switch(ot)
		{
			case 0:
				dir=4;
				break;
			case 1:
				dir=5;
				break;
			case 2:
				dir=3;
				break;
			default:
				dir=2;
				break;
		}
		vect3D clusterCoord=getCluster(m,i,j,k);
		quadList_struct* ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		vect3D clusterCoord2=getCluster(m,i,j,k-1);
		quadList_struct* ql2=&m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->cluster[clusterCoord2.z-m->offset.z].quadList;
		u8 light;
		int bid;
		surface(m, i, j, k, &light);
		getLight(m, i, j, k, &light, dir);
		addQuad(ql, m, dir, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
		surface(m, i, j, k, &light);
		getLight(m, i, j, k, &light, dir+8);
		addQuad(ql, m, dir+8, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
		surface(m, i, j, k-1, &light);
		getLight(m, i, j, k-1, &light, dir);
		addQuad(ql2, m, dir, light, bid, m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->data, i, j, k-1);
		surface(m, i, j, k-1, &light);
		getLight(m, i, j, k-1, &light, dir+8);
		addQuad(ql2, m, dir+8, light, bid, m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->data, i, j, k-1);
		return false;
	}else if(ot>=DOORTYPE+8 && ot<DOORTYPE+16)
	{
		mmEffect(SFX_ADD);
		if((ot-DOORTYPE)%2)k++;
		ot=(ot-DOORTYPE-((ot-DOORTYPE)%2)-8)/2;
		removeBlock(m, i, j, k, false);
		removeBlock(m, i, j, k-1, false);
		*getBlockP(m,i,j,k)-=8;
		*getBlockP(m,i,j,k-1)-=8;
		u8 dir;
		switch(ot)
		{
			case 2:
				dir=4;
				break;
			case 3:
				dir=5;
				break;
			case 1:
				dir=3;
				break;
			default:
				dir=2;
				break;
		}
		vect3D clusterCoord=getCluster(m,i,j,k);
		quadList_struct* ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		vect3D clusterCoord2=getCluster(m,i,j,k-1);
		quadList_struct* ql2=&m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->cluster[clusterCoord2.z-m->offset.z].quadList;
		u8 light;
		int bid;
		surface(m, i, j, k, &light);
		getLight(m, i, j, k, &light, dir);
		addQuad(ql, m, dir, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
		surface(m, i, j, k, &light);
		getLight(m, i, j, k, &light, dir+8);
		addQuad(ql, m, dir+8, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
		surface(m, i, j, k-1, &light);
		getLight(m, i, j, k-1, &light, dir);
		addQuad(ql2, m, dir, light, bid, m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->data, i, j, k-1);
		surface(m, i, j, k-1, &light);
		getLight(m, i, j, k-1, &light, dir+8);
		addQuad(ql2, m, dir+8, light, bid, m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->data, i, j, k-1);
		return false;
	}
	// the hoe and planting act on the block pointed at; a carrot not planted is eaten
	{
		int r=farmUseItem(m,cursorBlock,i,j,k,cursorDir);
		if(r)return r==2;
	}
	if(survivalEat())return false;                                    // food is eaten, not placed
	if(!cursorBlock || !survivalCanPlace(cursorBlock))return false;   // empty hand places nothing
	mmEffect(SFX_ADD);
	switch(cursorDir)
	{
		case 0:
			testCursorK++;
			break;
		case 1:
			testCursorK--;
			break;
		case 2:
			testCursorI++;
			break;
		case 3:
			testCursorI--;
			break;
		case 4:
			testCursorJ++;
			break;
		case 5:
			testCursorJ--;
			break;
	}
	testCursor=(testCursorI)+(testCursorJ)*(map).size.x+(testCursorK)*(map).size.y*(map).size.x;
	if(cursorBlock==ITEM_CHEST)
	{
		if(!chestPlace(m,testCursorI,testCursorJ,testCursorK))return false;
		farmBlockChanged(m,testCursorI,testCursorJ,testCursorK);
		return true;
	}else if(cursorBlock==ITEM_FURNACE)
	{
		if(!furnacePlace(m,testCursorI,testCursorJ,testCursorK))return false;
		farmBlockChanged(m,testCursorI,testCursorJ,testCursorK);
		return true;
	}else if(cursorBlock==ITEM_PUMPKIN)
	{
		// the face looks at the player, as in Minecraft
		u8 st=PUMPKIN_FIRST+chestFacingFromLook(Player.angleZ);
		if(*getBlockP(m,testCursorI,testCursorJ,testCursorK))return false;
		changeBlock(&map, testCursorI, testCursorJ, testCursorK, st);
		if(*getBlockP(m,testCursorI,testCursorJ,testCursorK)!=st)return false;
		farmBlockChanged(m,testCursorI,testCursorJ,testCursorK);
		return true;
	}else if(cursorBlock==11)
	{
		changeBlock(&map, testCursorI, testCursorJ, testCursorK, WATERTYPE);
		if(*getBlockP(m,testCursorI,testCursorJ,testCursorK)<WATERTYPE)return false;
		addWater(&map, testCursorI, testCursorJ, testCursorK, WATERTYPE);
	}else if(cursorBlock==LADDERTYPE)
	{
		if(cursorDir<2)return false;
		if(isLadder(*getBlockP(m,i,j,k)) || isDoor(*getBlockP(m,i,j,k)) || *getBlockP(m,i,j,k)==13)return false;
		i=testCursorI;j=testCursorJ;k=testCursorK;
		u8 *d=(getBlockP(m,i,j,k));
		if(*d)return false;
		*d=LADDERTYPE+cursorDir-2;
		vect3D clusterCoord=getCluster(m,i,j,k);
		quadList_struct* ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		
		u32 bid=(i)+(j)*(m)->size.x+(k)*(m)->size.y*(m)->size.x;
		u8 light=0;
		surface(m, i, j, k, &light);
		u8 dir;
		switch(cursorDir)
		{
			case 2:
				dir=3;
				break;
			case 3:
				dir=2;
				break;
			case 4:
				dir=5;
				break;
			case 5:
				dir=4;
				break;
		}
		getLight(m, i, j, k, &light, 10+dir-2);
		addQuad(ql, m, 10+dir-2, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
	}else if(cursorBlock==DOORTYPE)
	{
		if(cursorDir)return false;
		i=testCursorI;j=testCursorJ;k=testCursorK;
		u8 *d=(getBlockP(m,i,j,k));
		if(*d)return false;
		i=testCursorI;j=testCursorJ;k=testCursorK+1;
		u8 *d2=(getBlockP(m,i,j,k));
		if(*d2)return false;
		u8 dir;
		if(Player.angleZ<4096 || Player.angleZ>=32768-4096)
		{
			dir=5;
		}else if(Player.angleZ>=4096 && Player.angleZ<4096+8192)
		{
			dir=3;
		}else if(Player.angleZ>=4096+8192 && Player.angleZ<4096+8192+8192)
		{
			dir=4;
		}else{
			dir=2;
		}
		*d=DOORTYPE+1+(dir-2)*2;
		*d2=DOORTYPE+(dir-2)*2;
		vect3D clusterCoord=getCluster(m,i,j,k);
		quadList_struct* ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		vect3D clusterCoord2=getCluster(m,i,j,k-1);
		quadList_struct* ql2=&m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->cluster[clusterCoord2.z-m->offset.z].quadList;
		u8 light;
		int bid;
		surface(m, i, j, k, &light);
		getLight(m, i, j, k, &light, dir);
			addQuad(ql, m, dir, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
		surface(m, i, j, k, &light);
		getLight(m, i, j, k, &light, dir+8);
			addQuad(ql, m, dir+8, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
		surface(m, i, j, k-1, &light);
		getLight(m, i, j, k-1, &light, dir);
			addQuad(ql2, m, dir, light, bid, m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->data, i, j, k-1);
		surface(m, i, j, k-1, &light);
		getLight(m, i, j, k-1, &light, dir+8);
			addQuad(ql2, m, dir+8, light, bid, m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->data, i, j, k-1);
			m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->changed=1;
			m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->changed=1;
	}else{
		// leaves placed by the player never decay; saplings need grass or dirt
		u8 placed=(cursorBlock==LEAVES_BLOCK)?LEAVES_PLACED:cursorBlock;
		if(isSapling(placed))
		{
			u8 below=(testCursorK>0)?*getBlockP(m,testCursorI,testCursorJ,testCursorK-1):0;
			if((below!=1 && below!=2) || *getBlockP(m,testCursorI,testCursorJ,testCursorK))return false;
		}
		// changeBlock refuses a block where the player stands (as in Minecraft):
		// it only counts as placed, and uses up an item, if it is really there
		changeBlock(&map, testCursorI, testCursorJ, testCursorK, placed);
		if(*getBlockP(m,testCursorI,testCursorJ,testCursorK)!=placed)return false;
		farmBlockChanged(m,testCursorI,testCursorJ,testCursorK);
		return true;
	}
	return true;
}

// Survival: holding the dig button mines the block over time.
// Creative: the block breaks when the button is released.
void destroyBlock(void);

// with the inventory open the buttons belong to it (as in Minecraft)
static inline bool worldInput(void)
{
	return !invOpen;
}

static void updateDigging(bool held)
{
	if(!survivalEnabled())return;
	if(!worldInput())held=false;
	if(!held){survivalStopMining();return;}
	if(survivalMine())
	{
		cubeAngleX=-1;
		destroyBlock();
	}
	else if(survivalMiningTicks()%8==1)mmEffect(SFX_STEP);
}

static void usePlace(void)
{
	if(!worldInput())return;
	if(!cursorValid){survivalEat();return;}                           // eating needs no block in sight
	if(placeBlock())survivalConsume(cursorBlock);
}

void destroyBlock(void)
{
	if(!cursorValid)return;
	map_struct* m=&map;
	int i=testCursorI, j=testCursorJ, k=testCursorK;
	u8 ot=*getBlockP(m,i,j,k);
	if(!survivalCanBreak(ot))return;
	mmEffect(SFX_REMOVE);
	survivalBlockBroken(ot,i,j,k);
	if(ot>=DOORTYPE && ot<DOORTYPE+16)
	{
		if((ot-DOORTYPE)%2)k++;
		ot=(ot-DOORTYPE-((ot-DOORTYPE)%2))/2;
		removeBlock(m, i, j, k, false);
		removeBlock(m, i, j, k-1, false);
		*getBlockP(m,i,j,k)=0;
		*getBlockP(m,i,j,k-1)=0;
		return;
	}
	if(isChestBlock(ot))chestBroken(m,i,j,k,ot);
	if(isFurnaceBlock(ot))furnaceBroken(m,i,j,k);
	changeBlock(&map, testCursorI, testCursorJ, testCursorK, 0);
	if(!*getBlockP(m,i,j,k))
	{
		plantsBlockRemoved(m,i,j,k,ot);
		farmBlockChanged(m,i,j,k);
	}
}

u8 holdABXY;
u8 doubletap;

// Double presses, within 7 Minecraft ticks (10 updates here): forward twice
// sprints, jump twice starts or stops flying in creative.
#define DOUBLE_PRESS 10
static int forwardTimer, jumpTimer;
static bool touchJump;             // scheme 1: the stylus stays down after a jump

static void jumpInput(void)
{
	if(noclip)return;
	if(!survivalEnabled())
	{
		if(jumpTimer)
		{
			jumpTimer=0;
			Player.flying=!Player.flying;
			Player.flyVX=Player.flyVY=0;
			if(Player.flying && Player.vector.z<0)Player.vector.z=0;
			return;
		}
		jumpTimer=1;
	}
	if(!Player.flying && (Player.inWater || !Player.vector.z))Player.vector.z+=JUMP_SPEED;
}

void controlScheme1(void)
{
	u16 keys = keysHeld();
	if(!noclip)
	{
		if((keys & KEY_LEFT) || (keys & KEY_Y)) {walkSfx++;Player.vector.y += cosLerp(Player.angleZ-8192)>>3;Player.vector.x += sinLerp(Player.angleZ-8192)>>3;if(!Player.vector.z)walkAngle+=2500;}
		if((keys & KEY_RIGHT) || (keys & KEY_A)) {walkSfx++;Player.vector.y += cosLerp(Player.angleZ+8192)>>3;Player.vector.x += sinLerp(Player.angleZ+8192)>>3;if(!Player.vector.z)walkAngle+=2500;}
	}else{
		if((keys & KEY_LEFT) || (keys & KEY_Y)) tempAngle.z -= 500;
		if((keys & KEY_RIGHT) || (keys & KEY_A)) tempAngle.z += 500;
	}
	
		
	touchRead(&thisXY);
	
	if(doubletap)doubletap++;
	if(doubletap>15)doubletap=0;
	if((keysDown() & KEY_TOUCH) && !doubletap)doubletap=1;
	else if((keysDown() & KEY_TOUCH) && doubletap && !invOpen && !overButtons){jumpInput();doubletap=0;touchJump=true;}
	if(!(keysHeld() & KEY_TOUCH))touchJump=false;
	
	if(updateInterface() && !overButtons && (keysHeld() & KEY_TOUCH))
	{
		if(!(keysDown() & KEY_TOUCH))
		{
			int16 dx = thisXY.px - lastXY.px;
			int16 dy = thisXY.py - lastXY.py;

			// filtering measurement errors
			if (dx<20 && dx>-20 && dy<20 && dy>-20)
			{
				if(dx>-2&&dx<2)
					dx=0;

				if(dy>-2&&dy<2)
					dy=0;

					tempAngle.x += degreesToAngle(dy);
					tempAngle.z += degreesToAngle(dx);
			}

		}
	}
	if(keysHeld() & KEY_TOUCH)lastXY = thisXY;
	
	
	testCursorI=testCursor%map.size.x;
	testCursorJ=((testCursor-testCursorI)%(map.size.x*map.size.y))/(map.size.x);
	testCursorK=(testCursor-testCursorI-testCursorJ*(map.size.x))/(map.size.x*map.size.y);
	#ifdef DEBUGMODE
	iprintf("\ncursor1 : %d %d %d %d   ",testCursorI,testCursorJ,testCursorK, testCursor);
	#endif
	if((keysUp() & KEY_R) || (keysUp() & KEY_L)){
		cubeAngleX=-1;
		if(action)
		{
			if(!survivalEnabled() && worldInput())destroyBlock();
		}else{
			usePlace();
		}
	}
	updateDigging(action && (keysHeld() & (KEY_R|KEY_L)));
}

void controlScheme2(void)
{
	u16 keys = keysHeld();

	if((keys & KEY_LEFT)) tempAngle.z -= 500;
	if((keys & KEY_RIGHT)) tempAngle.z += 500;
	if((keys & KEY_X)) tempAngle.x -= 500;
	if((keys & KEY_B)) tempAngle.x += 500;
	if((keysDown() & KEY_Y))
	{
		tempCursor++;
		tempCursor%=9;
		if(survivalEnabled())inventorySelect(tempCursor);
		else creativeSelect(tempCursor);
	}
	if(keysDown() & KEY_A)jumpInput();
	updateDigging(keysHeld() & KEY_R);
	if(keysUp() & KEY_R)
	{
		cubeAngleX=-1;
		if(!survivalEnabled() && worldInput())destroyBlock();
	}else if(keysUp() & KEY_L){
		cubeAngleX=-1;
		usePlace();
	}
	touchRead(&thisXY);
	updateInterface();
	if(keysHeld() & KEY_TOUCH)lastXY = thisXY;
}

void controlScheme3(void)
{
	u16 keys = keysHeld();

	if((keys & KEY_LEFT)) tempAngle.z -= 500;
	if((keys & KEY_RIGHT)) tempAngle.z += 500;
	if((keys & KEY_X)) tempAngle.x -= 500;
	if((keys & KEY_B)) tempAngle.x += 500;
	if(keysDown() & KEY_A)jumpInput();
	if(keysUp() & KEY_Y)
	{
		cubeAngleX=-1;
		if(action)
		{
			if(!survivalEnabled() && worldInput())destroyBlock();
		}else{
			usePlace();
		}
	}
	updateDigging(action && (keysHeld() & KEY_Y));
	touchRead(&thisXY);
	updateInterface();
	if(keysHeld() & KEY_TOUCH)lastXY = thisXY;
}

void updateControls(void)
{
	if(survivalInputLocked())
	{
		tempAngle=(vect3D){0,0,0};
		return;
	}
	if(testBuffer)
	{
		scanKeys();
		tempAngle=(vect3D){0,0,0};
		u16 keys = keysHeld();
		{
			bool forward=(keys & KEY_UP) || (!gameSettings.controls && (keys & KEY_X));
			bool forwardDown=(keysDown() & KEY_UP) || (!gameSettings.controls && (keysDown() & KEY_X));
			Player.sneaking=(keys & KEY_SELECT) && !noclip;
			if(forwardTimer && ++forwardTimer>DOUBLE_PRESS)forwardTimer=0;
			if(jumpTimer && ++jumpTimer>DOUBLE_PRESS)jumpTimer=0;
			if(forwardDown)
			{
				if(forwardTimer && !Player.sneaking && !Player.inWater)Player.sprinting=true;
				forwardTimer=1;
			}
			if(!forward || Player.sneaking || Player.inWater || noclip)Player.sprinting=false;
		}
		if(!noclip && (Player.flying || (!Player.inWater && !Player.onLadder)))
		{
			if((keys & KEY_UP) || (!gameSettings.controls && (keys & KEY_X))) {walkSfx++;Player.vector.y = cosLerp(Player.angleZ)>>3;Player.vector.x = sinLerp(Player.angleZ)>>3;if(!Player.vector.z)walkAngle+=2500;}
			if((keys & KEY_DOWN) || (!gameSettings.controls && (keys & KEY_B))) {walkSfx++;Player.vector.y = -cosLerp(Player.angleZ)>>3;Player.vector.x = -sinLerp(Player.angleZ)>>3;if(!Player.vector.z)walkAngle-=2500;}
		}else if(Player.inWater){
			if((keys & KEY_UP) || (!gameSettings.controls && (keys & KEY_X))) {Player.vector.y = mulf32(cosLerp(Player.angleZ),cosLerp(Player.angleX))>>3;Player.vector.x = mulf32(sinLerp(Player.angleZ),cosLerp(Player.angleX))>>3;Player.vector.z += -sinLerp(Player.angleX)>>6;}
			if((keys & KEY_DOWN) || (!gameSettings.controls && (keys & KEY_B))) {Player.vector.y = -mulf32(cosLerp(Player.angleZ),cosLerp(Player.angleX))>>3;Player.vector.x = -mulf32(sinLerp(Player.angleZ),cosLerp(Player.angleX))>>3;Player.vector.z += sinLerp(Player.angleX)>>6;}
		}else if(Player.onLadder){
			if((keys & KEY_UP) || (!gameSettings.controls && (keys & KEY_X))) {Player.vector.y = mulf32(cosLerp(Player.angleZ),cosLerp(Player.angleX))>>2;Player.vector.x = mulf32(sinLerp(Player.angleZ),cosLerp(Player.angleX))>>2;Player.vector.z = 1024;}
			if((keys & KEY_DOWN) || (!gameSettings.controls && (keys & KEY_B))) {Player.vector.y = -mulf32(cosLerp(Player.angleZ),cosLerp(Player.angleX))>>2;Player.vector.x = -mulf32(sinLerp(Player.angleZ),cosLerp(Player.angleX))>>2;Player.vector.z = -1024;}
		}else{
			if((keys & KEY_UP) || (!gameSettings.controls && (keys & KEY_X))) {Player.vector.y = mulf32(cosLerp(Player.angleZ),cosLerp(Player.angleX))>>3;Player.vector.x = mulf32(sinLerp(Player.angleZ),cosLerp(Player.angleX))>>3;Player.vector.z = -sinLerp(Player.angleX)>>3;}
			if((keys & KEY_DOWN) || (!gameSettings.controls && (keys & KEY_B))) {Player.vector.y = -mulf32(cosLerp(Player.angleZ),cosLerp(Player.angleX))>>3;Player.vector.x = -mulf32(sinLerp(Player.angleZ),cosLerp(Player.angleX))>>3;Player.vector.z = sinLerp(Player.angleX)>>3;}
		}
		if(!(keysHeld() & KEY_A) && !(keysHeld() & KEY_B) && !(keysHeld() & KEY_X) && !(keysHeld() & KEY_Y))holdABXY=false;
		if(!holdABXY && !survivalEnabled() && (keysHeld() & KEY_A) && (keysHeld() & KEY_B) && (keysHeld() & KEY_X) && (keysHeld() & KEY_Y))
		{
			holdABXY=true;
			noclip^=1;
			Player.flying=false;
		}
		if((keysDown() & KEY_START)) {gamePause(true);return;}
		if(Player.sneaking || Player.flying)walkSfx=0;            // sneaking and flying make no steps
		if(oldwAngle!=walkSfx && ((walkSfx)>=10) && !Player.vector.z){mmEffect(SFX_STEP);walkSfx=0;}
		oldwAngle=walkSfx;
		switch(gameSettings.controls)
		{
			case 1:
				controlScheme2();
				break;
			case 2:
				controlScheme3();
				break;
			default:
				controlScheme1();
				break;
		}
		{
			// held jump: A, or in scheme 1 the stylus kept down after the jump's double tap
			Player.jumpHeld=gameSettings.controls?((keysHeld() & KEY_A)!=0):(touchJump && (keysHeld() & KEY_TOUCH));
		}		
	}
	
	Player.angleX+=tempAngle.x/2;
	Player.angleZ+=tempAngle.z/2;
	if(Player.angleX>8192)Player.angleX=8192;
	else if(Player.angleX<-8192)Player.angleX=-8192;
	tempCursor%=9;
}

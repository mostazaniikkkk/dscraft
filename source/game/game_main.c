#include "game/game_main.h"

#define minus(a) (((a)<0)?((a)+1):((a)-1))
	
void Game_Init(void)
{
	int i;
	lcdMainOnTop();

	initFilesystem();

	//set mode 0, enable BG0 and set it to 3D
	videoSetMode(MODE_3_3D | DISPLAY_BG3_ACTIVE);
	#ifdef DEBUGMODE
		videoSetModeSub(MODE_0_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_2D_BMP_256 | DISPLAY_SPR_EXT_PALETTE | DISPLAY_BG0_ACTIVE);
	#else
		videoSetModeSub(MODE_0_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_2D_BMP_256 | DISPLAY_SPR_EXT_PALETTE);
	#endif
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankB(VRAM_B_LCD);
	vramSetBankC(VRAM_C_MAIN_BG_0x06000000);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	vramSetBankE(VRAM_E_TEX_PALETTE);
	vramSetBankF(VRAM_F_LCD);
	vramSetBankG(VRAM_G_LCD);
	vramSetBankH(VRAM_H_SUB_BG);
	vramSetBankI(VRAM_I_SUB_SPRITE_EXT_PALETTE);
	// the top screen shows the VRAM C bitmap while the world loads: clear it now
	// instead of after loading, or the menu's leftovers show as noise
	dmaFillWords(0, BG_GFX, 128*1024);
	BG_PALETTE[0]=RGB15(0,0,0);
	
	#ifdef DEBUGMODE
		consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);
	#endif
	#ifdef DEBUGMODE2
		videoSetModeSub(MODE_0_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_2D_BMP_256 | DISPLAY_SPR_EXT_PALETTE | DISPLAY_BG0_ACTIVE);
		consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);
	#endif
	
	initInterface();
	
	REG_BG3CNT = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(1);
        REG_BG3PA = 1 << 8;
        REG_BG3PB = 0;
        REG_BG3PC = 0;
        REG_BG3PD = 1 << 8;
		
        REG_BG3X = 0;
        REG_BG3Y = 0;
	REG_BG0CNT = BG_PRIORITY(0);

	// initialize gl
	glReInit();
	
	// enable antialiasing
	glDisable(GL_ANTIALIAS);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	
	setFog(0);

	Game_InitVramBanks(1);
	Game_InitTextures();

	// setup the rear plane
	glClearColor(0,0,0,0); // BG must be opaque for AA to work
	glClearPolyID(63); // BG must have a unique polygon ID for AA to work
	glClearDepth(0x7FFF);
	
	//this should work the same as the normal gl call
	glViewport(0,0,255,191);

	//any floating point gl call is being converted to fixed prior to being implemented
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, 256.0 / 192.0, 0.1, 10000);
	
	gluLookAt(	0.0, 0.0, 0.0,		//camera possition
				0.0, 1.0, 0.0,		//look at
				0.0, 0.0, 1.0);		//up
	
	initControls();
	
	initEnvironment(false);
	loadBlockTextures(true, true);
	
	plantsClear();                   // the map loader finds the saplings and marked leaves
	loadTestMap(&map);
	chestsLoad(mapPath);
	furnacesLoad(mapPath);
	
		testCursorI=map.size.x/2;
		testCursorJ=map.size.y/2;
		testCursorK=map.size.z/2 + 2;

	NOGBA("RAM : %dko used, %dko free    \n",DS_UsedMem()/1024,DS_FreeMem()/1024);
	Game_GetVramStatus();
	
		vramSetBankB(VRAM_B_LCD);
		vramSetBankC(VRAM_C_LCD);
	for(i=0;i<256*192;i++){VRAM_B[i]=0;VRAM_C[i]=0;}
	loadInterface("interface.bin", 1);
	initPlayer(&Player);
	survivalInit(&map, &Player);
	if(!survivalEnabled())creativeInit();
	testBuffer=false;
}

void SetRegCapture(bool enable, uint8 srcBlend, uint8 destBlend, uint8 bank, uint8 offset, uint8 size, uint8 source, uint8 srcOffset)
{
	uint32 value=0;

	if(enable)value|=1 << 31; // 31 is enable
	value|=0 << 29; // 29-30 seems to have something to do with the blending   //3
	value|=(srcOffset & 0x3) << 26; // capture source offset is 26-27
	value|=(source & 0x3) << 24; // capture source is 24-25
	value|=(size & 0x3) << 20; // capture data write size is 20-21
	value|=(offset & 0x3) << 18; // write offset is 18-19
	value|=(bank & 0x3) << 16; // vram bank select is 16-17
	value|=(srcBlend & 0x1F) << 8; // graphics blend evb is 8..12
	value|=(destBlend & 0x1F) << 0; // ram blend EVA is bits 0..4
	REG_DISPCAPCNT=value;
}

// asked for by the game menu; taken at the start of a frame, before the
// capture banks are set up for it
static bool screenshotRequested;

void takeScreenshot(void)
{
	screenshotRequested=true;
}

static void screenshotNow(void)
{
		vramSetBankB(VRAM_B_LCD);
		vramSetBankC(VRAM_C_LCD);
	Debug_TakeScreenshotBMP(VRAM_B, VRAM_C);
}

void Game_Frame(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(playerFov(), 256.0 / 192.0, 0.1, 10000);   // wider when sprinting or flying
				
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	if(testBuffer)
	{
		Game_FrameCount++;
		if(screenshotRequested){screenshotRequested=false;screenshotNow();}
		vramSetBankB(VRAM_B_MAIN_BG_0x06000000);
		vramSetBankC(VRAM_C_LCD);
		SetRegCapture(true, 0, 16, 2, 0, 3, 1, 0);
		drawSky();
		REG_BG0CNT = BG_PRIORITY(0);
		REG_BG3CNT = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(1);
		testBuffer=false;
	}else{
		vramSetBankB(VRAM_B_LCD);
		vramSetBankC(VRAM_C_MAIN_BG_0x06000000);
		SetRegCapture(true, 0, 16, 1, 0, 3, 1, 0);
		glClearColor(0,0,0,0);
		REG_BG0CNT = BG_PRIORITY(1);
		REG_BG3CNT = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(0);
		testBuffer=true;
	}
	
	playerCamera(&Player, false);

	glPolyFmt(POLY_ALPHA(31)  | POLY_CULL_BACK);
	
	if(!gamePaused)updateControls();
	else if(testBuffer)pauseUpdate();
	
	drawTestMap(&map);
	
	glPopMatrix(1);
	
	if(testBuffer)
	{
		//HUD
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT);
		if(cursorBlock)drawTestCube();   // nothing in hand: no cube
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-128, 127, 95, -96, -1000, 1000);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef32(0, inttof32(-800), 0);
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
		glColor(RGB15(31,31,31));
		Game_ApplyMTL(crossHair);
		glBegin(GL_QUADS);
		GFX_TEX_COORD = TEXTURE_PACK(16*(0), 16*(16));
		GFX_VERTEX10 = NORMAL_PACK((255),(0),(-256));
		GFX_TEX_COORD = TEXTURE_PACK(16*(16), 16*(16));
		GFX_VERTEX10 = NORMAL_PACK((255),(0),(255));
		GFX_TEX_COORD = TEXTURE_PACK(16*(16), 16*0);
		GFX_VERTEX10 = NORMAL_PACK((-256),(0),(255));
		GFX_TEX_COORD = TEXTURE_PACK(16*(0), 16*0);
		GFX_VERTEX10 = NORMAL_PACK((-256),(0),(-256));
		glPopMatrix(1);
	}

	glFlush(0);

	int time;
	PROF2_END(time);
	addValue(&frameTime,time);
	swiWaitForVBlank();
	PROF2_START();
	
	if(keysDown() & KEY_X){DS_UsedMem();addValue(&freeRam,DS_FreeMem()/1024);}
	
	#ifdef DEBUGMODE
	if(testBuffer)iprintf("\x1b[0;0HMCDS_test %d fps   \n\n%dK;%dK %d, %d, %d, %d ",Game_FPS,latestFree/1024,latestUsed/1024,TESTVALUE, cacheNumber, cacheCursor, lightProcess.count);
	#endif
	#ifdef DEBUGMODE2
	if(testBuffer)iprintf("\x1b[0;0HMCDS_test %d fps   \n\n%dK;%dK %d, %d, %d, %d ",Game_FPS,latestFree/1024,latestUsed/1024,TESTVALUE, cacheNumber, cacheCursor, lightProcess.count);
	#endif
}

void Game_Kill(void)
{
	survivalKill();
	chestsClear();
	furnacesClear();
	plantsClear();
	freeMap(&map);
	freeQuadCache();
	freeLightCache();
	freeEnvironment();
	NOGBA("RAM after free :\n%dko used, %dko free    \n",DS_UsedMem()/1024,DS_FreeMem()/1024);
}

void Game_VBlank(void)
{
	Game_VBLcount++;
	if(Game_VBLcount>=60){Game_FPS=Game_FrameCount;Game_FrameCount=0;Game_VBLcount=0;}
}

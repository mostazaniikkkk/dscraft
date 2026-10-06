#include "menu/menu_main.h"
#include <dirent.h>
#include "API/API.h"
#include "game/map.h"
#include "game/player.h"
#include "game/environment.h"
#include "game/survival.h"
#include "game/worldgen.h"

void chestSidecarPath(const char* mapPath, char* out, int size);   // game/chest.c
void furnaceSidecarPath(const char* mapPath, char* out, int size); // game/furnace.c

#define minus(a) (((a)<0)?((a)+1):((a)-1))

typedef struct
{
	API_Entity* button;
	char* filename;
	bool isDir, nitro;
}listFile_struct;

typedef struct
{
	listFile_struct* list;
	u16 count;
}filelist_struct;

filelist_struct worldList, packList;

void setupButtons(filelist_struct* l, u16 index);
void Menu_SelectScheme(API_Entity* e);
void Menu_LeaveDelete(void);
void Menu_HideCredits(void);
static bool menuRendered;          // the menu has drawn at least one frame
static void enterSingleScreen(void);

API_Entity* bottomScreen;
API_Entity *singleWindow, *optionWindow, *textureWindow;
API_Entity* topScreen;
API_Entity* schemes[3];
API_Entity* schemeWindows[3*2];
u8 selectedScheme;

MTL_img* logo;

u32* sceneList1;
u32* subSceneList1;

s16 testAngleZ;
s16 testAngleX;
s16 subtestAngleZ;
s16 subtestAngleX;

vect3D logoPos=(vect3D){0,0,0};


void RenderScene()
{
		if(!D3D_Screen)sunX+=degreesToAngle(180);
		else sunX-=degreesToAngle(180);
		sunZ%=32768;
		sunX%=32768;
		if(sunX<0)sunX+=degreesToAngle(360);
		dayTime=((sunX<16384)?(8192-abs(sunX-8192)):0);
		dayTime=(dayTime<4096)?(dayTime):4096;
		nightTime=((sunX<16384)?0:(8192-abs((sunX-16384)-8192)));
		nightTime=(nightTime<4096)?(nightTime):4096;
		
		drawSky();
		glPushMatrix();
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(70, 256.0 / 192.0, 0.1, 10000);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		if(!D3D_Screen)
		{
			glRotateXi(testAngleX);
			glRotateZi(testAngleZ);
		}else{
			glRotateXi(subtestAngleX);
			glRotateZi(subtestAngleZ);
		}

		glMaterialf(GL_AMBIENT, RGB15(14,14,14));
		glMaterialf(GL_DIFFUSE, RGB15(31,31,31));
		glMaterialf(GL_SPECULAR, BIT(15) | RGB15(21,21,21));
		glMaterialf(GL_EMISSION, RGB15(0,0,0));

		//ds uses a table for shinyness..this generates a half-ass one
		glMaterialShinyness();
		glLight(0, RGB15(31,31,31), minus(mulf32(sinLerp(sunZ),cosLerp(sunX))/8), minus(mulf32(cosLerp(sunZ),cosLerp(sunX))/8), minus(-(sinLerp(sunX)/8)));

		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(63));
		
		drawSun();
		drawMoon();
		drawDawn();
		drawStars();
		drawCloud();
		
		if(!D3D_Screen)
		{
			glPushMatrix();
				Game_ApplyMTL(logo);
				glTranslatef32(logoPos.x,logoPos.y,0);
				glScalef32(-inttof32(1),-inttof32(1),inttof32(1));
				drawLogo();
			glPopMatrix(1);
		}
		
		glPushMatrix();
			glPolyFmt(POLY_ALPHA(31) | POLY_FORMAT_LIGHT0 | POLY_CULL_BACK);
			glScalef32(inttof32(SCALEFACTOR),inttof32(SCALEFACTOR),inttof32(SCALEFACTOR));
			glTranslatef32(-(SUPERCLUSTERSIZE*CLUSTERSIZE*(tilesize2<<6))/2, -(SUPERCLUSTERSIZE*CLUSTERSIZE*(tilesize2<<6))/2,-(64*(tilesize2<<6))/2);
			Game_ApplyMTL(blockSuperTexture);
			if(!D3D_Screen)
			{
				glCallList(sceneList1);
			}else{
				glCallList(subSceneList1);
			}
		glPopMatrix(1);
		
		sunX+=20;
		
		glPopMatrix(1);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		
		if(D3D_Screen)glOrtho(-128, 127, 191, 0, -1000, 1000);
		else glOrtho(-128, 127, 0, -191, -1000, 1000);
		
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		
		scanKeys();
		
		API_UpdateScene(&API_List);
		
		glPopMatrix(1);
			
		glFlush(0);
}

u8* loadFile(char* filename)
{
	FILE* file;
	
	NOGBA("opening...");
	file = fopen(filename, "rb+");
	
	u8* buffer;
	long lsize;
	fseek (file, 0 , SEEK_END);
	lsize = ftell (file);
	lastSize=lsize;
	rewind (file);
	buffer = (u8*) malloc (lsize);
		
	fread (buffer, 1, lsize, file);
	fclose (file);
	return buffer;
}

void Menu_Singleplayer(API_Entity* e)
{
	API_SetAlpha(bottomScreen,0);
	API_SetAlphaSons(bottomScreen,0);
	API_SetAlpha(singleWindow,31);
	API_SetAlphaSons(singleWindow,31);
	setupButtons(&worldList, 0);
}

void Menu_Texture(API_Entity* e)
{
	API_SetAlpha(bottomScreen,0);
	API_SetAlphaSons(bottomScreen,0);
	API_SetAlpha(textureWindow,31);
	API_SetAlphaSons(textureWindow,31);
	setupButtons(&packList, 0);
}

void Menu_Option(API_Entity* e)
{
	API_SetAlpha(bottomScreen,0);
	API_SetAlphaSons(bottomScreen,0);
	API_SetAlpha(optionWindow,31);
	API_SetAlphaSons(optionWindow,31);
	Menu_SelectScheme(schemes[selectedScheme]);
}

void Menu_Back(API_Entity* e)
{
	Menu_LeaveDelete();
	Menu_HideCredits();
	API_SetAlpha(bottomScreen,31);
	API_SetAlphaSons(bottomScreen,31);
	API_SetAlpha(singleWindow,0);
	API_SetAlpha(optionWindow,0);
	API_SetAlpha(textureWindow,0);
	API_SetAlphaSons(singleWindow,0);
	API_SetAlphaSons(optionWindow,0);
	API_SetAlphaSons(textureWindow,0);
	API_SetAlpha(schemeWindows[0],0);
	API_SetAlpha(schemeWindows[1],0);
	API_SetAlpha(schemeWindows[2],0);
	API_SetAlpha(schemeWindows[3],0);
	API_SetAlpha(schemeWindows[4],0);
	API_SetAlpha(schemeWindows[5],0);
}

void Menu_BackOptions(API_Entity* e)
{
	Menu_Back(e);
	
	if(gameSettings.controls!=selectedScheme || strcmp(gameSettings.texturePack,packPath))
	{
		sprintf(gameSettings.texturePack,packPath);
		gameSettings.controls=selectedScheme;
		saveSettings();
	}
}

void Menu_ChangePack(API_Entity* e)
{
	int i;
	for(i=0;i<packList.count;i++)
	{
		if(e==packList.list[i].button)
		{
			#ifdef FATONLY
				sprintf(packPath,"packs/%s",packList.list[i].filename);
			#else
				if(packList.list[i].nitro)sprintf(packPath,"nitro:/dscraft/packs/%s",packList.list[i].filename);
				else{sprintf(packPath,"%s/%s/packs/%s",basePath,ROOT,packList.list[i].filename);}
			#endif
		}
	}
	loadBlockTextures(false, false);
}

// New world. The menu draws both screens with the single 3D engine, alternating
// every frame, so every frame must end before the next vertical blank or the
// screens get swapped. Generation runs in small units right after the vertical
// blank and stops at scanline GEN_LAST_LINE, which leaves time to draw the menu.
#define GENTEXT_LEN 30
#define GEN_LAST_LINE 60

static API_Entity* genWindow;
static API_Entity* genLabel;
static API_Entity* genModes[2];        // Survival, Creative: the world's game mode, fixed at creation
static API_Entity* genCreate;
static bool genSurvival=true;
static bool generating;
static bool showWorldsOnInit;
static char genPath[255];

// world options: name (also the file name) and superflat
#define NAME_MAX_LEN 16
static char worldName[NAME_MAX_LEN+1]="New World";
static API_Entity* genNameTitle;
static API_Entity* genNameButton;
static API_Entity* genFlat;
static bool genFlatWorld;

// on-screen keyboard for the world name
#define KB_KEYS 37
static const char kbChars[KB_KEYS+1]="1234567890qwertyuiopasdfghjklzxcvbnm-";
static API_Entity* kbWindow;
static API_Entity* kbNameLabel;
static API_Entity* kbKeys[KB_KEYS];
static char* kbText;               // the text being typed: the world's name or its seed
static const char* kbTitle;

// the seed (empty: random), as in Minecraft's world options
static char worldSeed[NAME_MAX_LEN+1];
static API_Entity* genSeedTitle;
static API_Entity* genSeedButton;
static u32 menuFrames;             // frames since the game started: randomness for seeds
static bool kbUpper;

// deleting worlds
static API_Entity* singleTitle;
static API_Entity* deleteButton;
static bool deleteMode;
static API_Entity* confirmWindow;
static API_Entity* confirmLine1;
static API_Entity* confirmLine2;
static API_Entity* confirmDelete;
static int deleteIndex=-1;

// write `text` centred in a label created with `width` spaces
static void setLabelText(API_Entity* label, const char* text, int width)
{
	char* s=((APIE_LabelData*)label->data)->string;
	int len=strlen(text), pad;
	if(len>width)len=width;
	pad=(width-len)/2;
	memset(s,' ',width);
	memcpy(s+pad,text,len);
	s[width]=0;
}

static void setButtonText(API_Entity* button, const char* text, int width)
{
	setLabelText(((APIE_ButtonData*)button->data)->label,text,width);
}

// a world's name is its file name without ".map"
static void displayName(const char* file, char* out, int size)
{
	int len=strlen(file);
	if(len>4 && !strcasecmp(file+len-4,".map"))len-=4;
	if(len>size-1)len=size-1;
	memcpy(out,file,len);
	out[len]=0;
}

static void setGenText(const char* text)
{
	setLabelText(genLabel,text,GENTEXT_LEN);
}

static void showModeChoice(bool show)
{
	int i;
	for(i=0;i<2;i++)
	{
		API_SetAlpha(genModes[i],show?31:0);
		API_SetAlphaSons(genModes[i],show?31:0);
		((APIE_CheckBoxData*)genModes[i]->data)->checked=(i==0)==genSurvival;
	}
	API_SetAlpha(genCreate,show?31:0);
	API_SetAlphaSons(genCreate,show?31:0);
	API_SetAlpha(genFlat,show?31:0);
	API_SetAlphaSons(genFlat,show?31:0);
	((APIE_CheckBoxData*)genFlat->data)->checked=genFlatWorld;
	API_SetAlpha(genNameTitle,show?31:0);
	API_SetAlpha(genNameButton,show?31:0);
	API_SetAlphaSons(genNameButton,show?31:0);
	setButtonText(genNameButton,worldName,NAME_MAX_LEN);
	API_SetAlpha(genSeedTitle,show?31:0);
	API_SetAlpha(genSeedButton,show?31:0);
	API_SetAlphaSons(genSeedButton,show?31:0);
	setButtonText(genSeedButton,worldSeed[0]?worldSeed:"(random)",NAME_MAX_LEN);
}

void Menu_ToggleFlat(API_Entity* e)
{
	genFlatWorld=((APIE_CheckBoxData*)e->data)->checked;
}

/* --- keyboard --- */

static void updateKeyboard(void)
{
	char text[GENTEXT_LEN+1];
	int i;
	snprintf(text,sizeof(text),"%s: %s_",kbTitle,kbText);
	setLabelText(kbNameLabel,text,GENTEXT_LEN);
	for(i=0;i<KB_KEYS;i++)
	{
		char c=kbChars[i];
		if(kbUpper && c>='a' && c<='z')c+='A'-'a';
		((APIE_LabelData*)((APIE_ButtonData*)kbKeys[i]->data)->label->data)->string[0]=c;
	}
}

static void openKeyboard(char* text, const char* title)
{
	kbText=text;
	kbTitle=title;
	API_SetAlpha(genWindow,0);
	API_SetAlphaSons(genWindow,0);
	API_SetAlpha(kbWindow,31);
	API_SetAlphaSons(kbWindow,31);
	updateKeyboard();
}

void Menu_EditName(API_Entity* e){ openKeyboard(worldName,"Name"); }
void Menu_EditSeed(API_Entity* e){ openKeyboard(worldSeed,"Seed"); }

static void typeChar(char c)
{
	int len=strlen(kbText);
	if(len<NAME_MAX_LEN){kbText[len]=c;kbText[len+1]=0;}
	updateKeyboard();
}

void Menu_Key(API_Entity* e)
{
	int i;
	for(i=0;i<KB_KEYS;i++)
	{
		if(e!=kbKeys[i])continue;
		char c=kbChars[i];
		if(kbUpper && c>='a' && c<='z')c+='A'-'a';
		typeChar(c);
	}
}

void Menu_KeySpace(API_Entity* e){ typeChar(' '); }

void Menu_KeyDelete(API_Entity* e)
{
	int len=strlen(kbText);
	if(len)kbText[len-1]=0;
	updateKeyboard();
}

void Menu_KeyShift(API_Entity* e)
{
	kbUpper=!kbUpper;
	updateKeyboard();
}

void Menu_KeyOK(API_Entity* e)
{
	API_SetAlpha(kbWindow,0);
	API_SetAlphaSons(kbWindow,0);
	API_SetAlpha(genWindow,31);
	API_SetAlphaSons(genWindow,31);
	setGenText("");
	showModeChoice(true);
}

// file name from the world name: characters FAT accepts, no surrounding spaces
static void fileBase(char* out, int size)
{
	int i, n=0;
	for(i=0;worldName[i] && n<size-1;i++)
	{
		char c=worldName[i];
		bool ok=(c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c==' ' || c=='-' || c=='_';
		if(n==0 && c==' ')continue;
		out[n++]=ok?c:'_';
	}
	while(n && out[n-1]==' ')n--;
	out[n]=0;
	if(!n)snprintf(out,size,"World");
}

void Menu_SelectMode(API_Entity* e)
{
	genSurvival=(e==genModes[0]);
	showModeChoice(true);
}

// "New world" on the world list: choose the game mode, then create
void Menu_NewWorld(API_Entity* e)
{
	API_SetAlpha(singleWindow,0);
	API_SetAlphaSons(singleWindow,0);
	API_SetAlpha(genWindow,31);
	API_SetAlphaSons(genWindow,31);
	setGenText("");
	showModeChoice(true);
}

// The typed seed, or a random one. The clock alone is not enough: without a
// working clock (some emulators) every world would come out the same, so the
// frames since start-up, the scanline and the stylus position are mixed in.
static u32 worldSeedValue(void)
{
	u32 seed;
	if(worldgenSeedFromText(worldSeed,&seed))return seed;
	seed=(u32)time(NULL);
	seed^=menuFrames*2654435761u;
	seed^=(u32)rand()<<11;
	seed^=(u32)REG_VCOUNT<<20;
	seed^=((u32)API_Touch.px<<8)|API_Touch.py;
	seed^=seed>>15;
	seed*=2246822519u;
	return seed^(seed>>13);
}

void Menu_CreateWorld(API_Entity* e)
{
	char dir[255];
	if(generating)return;
	showModeChoice(false);
	if(!saveAvailable)
	{
		setGenText("No SD card to save worlds");
		return;
	}
	#ifdef FATONLY
		sprintf(dir,"worlds");
	#else
		sprintf(dir,"%s/%s/worlds",basePath,ROOT);
	#endif
	char base[NAME_MAX_LEN+1];
	fileBase(base,sizeof(base));
	if(!worldgenNewPath(dir,base,genPath,sizeof(genPath))
	|| !worldgenStart(genPath,worldSeedValue(),genFlatWorld,genSurvival?&survivalFormatHeader:NULL))
	{
		setGenText("Could not start the world");
		return;
	}
	generating=true;
	enterSingleScreen();
	setGenText("Generating world   0%  B:Back");
}

// Generating is the heaviest work the menu does, so meanwhile only the top screen
// is drawn (the menu's own screen, with the progress) and the bottom one stays
// black: no alternating between screens, nothing to get out of step.
static bool singleScreen;

static void enterSingleScreen(void)
{
	singleScreen=true;
	REG_DISPCAPCNT=0;
	lcdMainOnTop();
	setBrightness(2,-16);
}

// back to drawing both screens, as when the menu starts
static void leaveSingleScreen(void)
{
	if(!singleScreen)return;
	singleScreen=false;
	lcdMainOnTop();
	D3D_Screen=true;
	menuRendered=false;            // the brightness comes back after two frames
}

void Menu_CancelWorld(API_Entity* e)
{
	if(generating)worldgenCancel();
	generating=false;
	leaveSingleScreen();
	API_SetAlpha(genWindow,0);
	API_SetAlphaSons(genWindow,0);
	Menu_Singleplayer(e);   // back to the world list
}

static void updateNewWorld(void)
{
	char text[GENTEXT_LEN+1];
	int p, line;
	if(!generating)return;
	// at least one unit per frame, then as many as fit before GEN_LAST_LINE
	do{
		p=worldgenStep();
		line=REG_VCOUNT;
	}while(p>=0 && p<100 && (line>=192 || line<GEN_LAST_LINE));
	if(p<0)
	{
		generating=false;
		leaveSingleScreen();
		setGenText("Could not write the world");
	}else if(p>=100)
	{
		// rebuild the menu so the world list includes the new file
		generating=false;
		showWorldsOnInit=true;
		DS_ChangeState(&Menu_State);
	}else{
		sprintf(text,"Generating world %3d%%  B:Back",p);
		setGenText(text);
	}
}

static void worldPath(int i, char* out)
{
	#ifdef FATONLY
		sprintf(out,"worlds/%s",worldList.list[i].filename);
	#else
		if(worldList.list[i].nitro)sprintf(out,"nitro:/dscraft/worlds/%s",worldList.list[i].filename);
		else sprintf(out,"%s/%s/worlds/%s",basePath,ROOT,worldList.list[i].filename);
	#endif
}

static void updateDeleteMode(void)
{
	setLabelText(singleTitle,deleteMode?"Tap a world to delete":"Select world",24);
	setButtonText(deleteButton,deleteMode?"Cancel":"Delete",6);
}

void Menu_LeaveDelete(void)
{
	if(!singleTitle)return;
	deleteMode=false;
	updateDeleteMode();
}

void Menu_ToggleDelete(API_Entity* e)
{
	deleteMode=!deleteMode;
	updateDeleteMode();
}

static void askDelete(int i)
{
	char name[GENTEXT_LEN+1];
	deleteIndex=i;
	displayName(worldList.list[i].filename,name,sizeof(name));
	API_SetAlpha(singleWindow,0);
	API_SetAlphaSons(singleWindow,0);
	API_SetAlpha(confirmWindow,31);
	API_SetAlphaSons(confirmWindow,31);
	if(worldList.list[i].nitro)
	{
		// built into the ROM: read-only
		setLabelText(confirmLine1,"Built-in worlds",GENTEXT_LEN);
		setLabelText(confirmLine2,"cannot be deleted",GENTEXT_LEN);
		API_SetAlpha(confirmDelete,0);
		API_SetAlphaSons(confirmDelete,0);
	}else{
		setLabelText(confirmLine1,"Delete this world forever?",GENTEXT_LEN);
		setLabelText(confirmLine2,name,GENTEXT_LEN);
	}
}

void Menu_ConfirmDelete(API_Entity* e)
{
	char path[255];
	if(deleteIndex<0 || worldList.list[deleteIndex].nitro)return;
	worldPath(deleteIndex,path);
	if(remove(path))
	{
		setLabelText(confirmLine1,"Could not delete",GENTEXT_LEN);
		setLabelText(confirmLine2,"the world",GENTEXT_LEN);
		API_SetAlpha(confirmDelete,0);
		API_SetAlphaSons(confirmDelete,0);
		return;
	}
	{
		char chests[300];
		chestSidecarPath(path,chests,sizeof(chests));
		remove(chests);            // the world's chests and furnaces go with it (there may be none)
		furnaceSidecarPath(path,chests,sizeof(chests));
		remove(chests);
	}
	// rebuild the menu so the list no longer shows it
	deleteMode=false;
	showWorldsOnInit=true;
	DS_ChangeState(&Menu_State);
}

void Menu_CancelDelete(API_Entity* e)
{
	API_SetAlpha(confirmWindow,0);
	API_SetAlphaSons(confirmWindow,0);
	deleteIndex=-1;
	Menu_Singleplayer(e);
	Menu_LeaveDelete();
}

/* --- credits --- */

#define CREDIT_LINES 9
#define CREDIT_PAGES 3

static const char* credits[CREDIT_PAGES][CREDIT_LINES]={
	{"DScraft",
	 "",
	 "Original game by smealum",
	 "summer 2011",
	 "www.smealum.net/dscraft",
	 "",
	 "The Survival Update",
	 "by mostazaniikkkk",
	 "Inspired by Minecraft (Mojang)"},
	{"Libraries",
	 "",
	 "devkitARM, libnds, libfat",
	 "and maxmod by devkitPro",
	 "LodePNG by Lode Vandevenne",
	 "iniparser by N. Devillard",
	 "PCX loader by David Henry",
	 "xmem by SunDEV",
	 ""},
	{"Texture packs",
	 "",
	 "Eldpack (eldpack.com)",
	 "Jolicraft",
	 "Painterly Pack by rhodox",
	 "Cel-Pak TRON Grid",
	 "by thetonestarr",
	 "PlanetPack, Visibility",
	 ""},
};

static API_Entity* creditsWindow;
static API_Entity* creditLabels[CREDIT_LINES];
static API_Entity* creditPageLabel;
static int creditPage;

static void showCreditPage(void)
{
	char page[GENTEXT_LEN+1];
	int i;
	for(i=0;i<CREDIT_LINES;i++)setLabelText(creditLabels[i],credits[creditPage][i],GENTEXT_LEN);
	snprintf(page,sizeof(page),"%d/%d",creditPage+1,CREDIT_PAGES);
	setLabelText(creditPageLabel,page,GENTEXT_LEN);
}

void Menu_Credits(API_Entity* e)
{
	API_SetAlpha(bottomScreen,0);
	API_SetAlphaSons(bottomScreen,0);
	API_SetAlpha(creditsWindow,31);
	API_SetAlphaSons(creditsWindow,31);
	creditPage=0;
	showCreditPage();
}

void Menu_CreditsNext(API_Entity* e)
{
	creditPage=(creditPage+1)%CREDIT_PAGES;
	showCreditPage();
}

void Menu_HideCredits(void)
{
	if(!creditsWindow)return;
	API_SetAlpha(creditsWindow,0);
	API_SetAlphaSons(creditsWindow,0);
}

void Menu_StartGame(API_Entity* e)
{
	int i;
	for(i=0;i<worldList.count;i++)
	{
		if(e==worldList.list[i].button && deleteMode)
		{
			askDelete(i);
			return;
		}
		if(e==worldList.list[i].button)
		{
			#ifdef FATONLY
				sprintf(mapPath,"worlds/%s",worldList.list[i].filename);
			#else
				if(worldList.list[i].nitro)sprintf(mapPath,"nitro:/dscraft/worlds/%s",worldList.list[i].filename);
				else sprintf(mapPath,"%s/%s/worlds/%s",basePath,ROOT,worldList.list[i].filename);
			#endif
		}
	}
	DS_ChangeState(&Game_State);
}

// files that belong to a world but are not worlds themselves (chest and furnace contents)
static bool hiddenFile(const char* path, const char* name)
{
	int len=strlen(name);
	return !strcmp(path,"worlds") && ((len>7 && !strcasecmp(name+len-7,".chests")) || (len>9 && !strcasecmp(name+len-9,".furnaces")));
}

void listFiles(char* path, filelist_struct* l, API_Entity* father, API_function function, u8 mode) //0 files, 1 both, 2 dir
{
	char str[255], shown[32];
	l->list=NULL;
	l->count=0;
	DIR *dir;
	struct dirent *ent;
	struct stat st;
	
	#ifndef FATONLY
		sprintf(str,"nitro:/%s/%s",ROOT,path);
		dir=opendir(str);
		if(dir)
		{
			while((ent=readdir(dir)))
			{
				stat(ent->d_name,&st);
				if(((S_ISDIR(st.st_mode) && mode) || !mode || (!S_ISDIR(st.st_mode) && mode==1))
				&& !(strlen(ent->d_name)==1 && ent->d_name[0]=='.')
				&& !(strlen(ent->d_name)==2 && ent->d_name[0]=='.' && ent->d_name[1]=='.') && !hiddenFile(path,ent->d_name))l->count++;
			}
			closedir(dir);
		}
	#endif
	#ifdef FATONLY
		sprintf(str,"%s",path);
	#else
		sprintf(str,"%s/%s/%s",basePath,ROOT,path);
	#endif
	dir=opendir(str);
	if(dir)
	{
		while((ent=readdir(dir)))
		{
			stat(ent->d_name,&st);
			if(((S_ISDIR(st.st_mode) && mode) || !mode || (!S_ISDIR(st.st_mode) && mode==1))
			&& !(strlen(ent->d_name)==1 && ent->d_name[0]=='.')
			&& !(strlen(ent->d_name)==2 && ent->d_name[0]=='.' && ent->d_name[1]=='.') && !hiddenFile(path,ent->d_name))l->count++;
		}
		closedir(dir);
	}
	
	l->list=malloc(sizeof(listFile_struct)*l->count);
	if(l->list)
	{
		l->count=0;
		
		#ifndef FATONLY
			sprintf(str,"nitro:/%s/%s",ROOT,path);
			dir=opendir(str);
			while((ent=readdir(dir)))
			{
				stat(ent->d_name,&st);
				if(((S_ISDIR(st.st_mode) && mode) || !mode || (!S_ISDIR(st.st_mode) && mode==1))
				&& !(strlen(ent->d_name)==1 && ent->d_name[0]=='.')
				&& !(strlen(ent->d_name)==2 && ent->d_name[0]=='.' && ent->d_name[1]=='.') && !hiddenFile(path,ent->d_name))
				{
					l->list[l->count].filename=malloc(strlen(ent->d_name)+1);
					strcpy(l->list[l->count].filename,ent->d_name);
					l->list[l->count].isDir=S_ISDIR(st.st_mode);
					l->list[l->count].nitro=true;
					displayName(l->list[l->count].filename,shown,sizeof(shown));
					l->list[l->count].button=API_CreateButtonFather(128-strlen(shown)*4, 32+20*l->count, RGB15(31,31,31), function, father, shown, "button.pcx", false);
					l->count++;
				}
			}
			closedir(dir);
		#endif
		
		#ifdef FATONLY
			sprintf(str,"%s",path);
		#else
			sprintf(str,"%s/%s/%s",basePath,ROOT,path);
		#endif
		dir=opendir(str);
		while((ent=readdir(dir)))
		{
			stat(ent->d_name,&st);
			if(((S_ISDIR(st.st_mode) && mode) || !mode || (!S_ISDIR(st.st_mode) && mode==1))
			&& !(strlen(ent->d_name)==1 && ent->d_name[0]=='.')
			&& !(strlen(ent->d_name)==2 && ent->d_name[0]=='.' && ent->d_name[1]=='.') && !hiddenFile(path,ent->d_name))
			{
				l->list[l->count].filename=malloc(strlen(ent->d_name)+1);
				strcpy(l->list[l->count].filename,ent->d_name);
				l->list[l->count].isDir=S_ISDIR(st.st_mode);
				l->list[l->count].nitro=false;
				displayName(l->list[l->count].filename,shown,sizeof(shown));
				l->list[l->count].button=API_CreateButtonFather(128-strlen(shown)*4, 32+20*l->count, RGB15(31,31,31), function, father, shown, "button.pcx", false);
				l->count++;
			}
		}
		closedir(dir);
	}
}

void setupButtons(filelist_struct* l, u16 index)
{
	int i;
	for(i=0;i<l->count;i++)
	{
		if(i>=index && i<index+4)
		{
			char shown[32];
			displayName(l->list[i].filename,shown,sizeof(shown));
			API_SetPosition(l->list[i].button, 128-strlen(shown)*4, 32+20*(i-index));
			API_SetAlpha(l->list[i].button,31);
		}else{
			API_SetAlpha(l->list[i].button,0);
		}
	}
}

void Menu_SingleSlider(API_Entity* e)
{
	APIE_SliderData* data=(APIE_SliderData*)e->data;
	setupButtons(&worldList, data->position*(max(worldList.count-4,0))/f32toint(e->Size.y));
}

void Menu_TextureSlider(API_Entity* e)
{
	APIE_SliderData* data=(APIE_SliderData*)e->data;
	setupButtons(&packList, data->position*(max(packList.count-4,0))/f32toint(e->Size.y));
}

void Menu_SelectScheme(API_Entity* e)
{
	int i;
	
	for(i=0;i<3;i++)
	{
		APIE_CheckBoxData* d=(APIE_CheckBoxData*)schemes[i]->data;
		if(e==schemes[i])
		{
			selectedScheme=i;
			API_SetAlpha(schemeWindows[i*2],31);
			API_SetAlpha(schemeWindows[i*2+1],31);
			d->checked=true;
		}else{
			d->checked=false;
			API_SetAlpha(schemeWindows[i*2],0);
			API_SetAlpha(schemeWindows[i*2+1],0);
		}
	}
}

void Menu_Init(void)
{
	menuRendered=false;
	singleScreen=false;
	
	#ifdef FATONLY
	#else
		chdir("nitro:/");
	#endif
	chdir(ROOT);
	sceneList1=(u32*)loadFile("testscene1.bin");
	subSceneList1=(u32*)loadFile("subtestscene1.bin");
		
	lcdMainOnTop();
	videoSetMode(MODE_3_3D | DISPLAY_BG0_ACTIVE);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_2D_BMP_256);
    vramSetPrimaryBanks(VRAM_A_TEXTURE,VRAM_B_TEXTURE,VRAM_C_SUB_BG,VRAM_D_SUB_SPRITE);	
	// The sub screen shows a bitmap (VRAM C) and bitmap sprites (VRAM D) fed by
	// display capture. Loading the menu takes a few seconds before the first
	// capture, so clear both: otherwise old VRAM contents (the debug console's
	// font) are displayed as coloured noise meanwhile.
	dmaFillWords(0, BG_GFX_SUB, 128*1024);
	dmaFillWords(0, SPRITE_GFX_SUB, 128*1024);
	BG_PALETTE_SUB[0]=RGB15(0,0,0);
	BG_PALETTE[0]=RGB15(0,0,0);
	vramSetBankE(VRAM_E_TEX_PALETTE);
	vramSetBankF(VRAM_F_LCD);
	vramSetBankG(VRAM_G_LCD);
	vramSetBankH(VRAM_H_LCD);
	vramSetBankI(VRAM_I_LCD);
	
	REG_BG2CNT_SUB = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(1);
        REG_BG2PA_SUB = 1 << 8;
        REG_BG2PB_SUB = 0;
        REG_BG2PC_SUB = 0;
        REG_BG2PD_SUB = 1 << 8;
		
        REG_BG2X_SUB = 0;
        REG_BG2Y_SUB = 0;
	
	REG_BG0CNT = BG_PRIORITY(0);
	
	glFlush(0);
	
	glReInit();
	
	Game_InitVramBanks(2);
	Game_InitTextures();
	
	srand(time(NULL));
	
	// enable antialiasing
	glEnable(GL_ANTIALIAS);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	
	// setup the rear plane
	glClearColor(0,0,0,31); // BG must be opaque for AA to work
	glClearPolyID(63); // BG must have a unique polygon ID for AA to work
	glClearDepth(0x7FFF);

	//this should work the same as the normal gl call
	glViewport(0,0,255,191);
	
	//any floating point gl call is being converted to fixed prior to being implemented
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, 256.0 / 192.0, 0.1, 40);
	
	gluLookAt(	0.0, 0.0, 0.0,		//camera possition
				0.0, 0.0, -1.0,		//look at
				0.0, 1.0, 0.0);		//up
	
	glLight(0, RGB15(31,31,31) , 0, floattov10(-100.0f), 0);
	
	//not a real gl function and will likely change
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_FORMAT_LIGHT0 ) ;
	Game_InitD3D();
	API_Init();
	NOGBA("api done");

	loadSettings();
	sprintf(packPath,gameSettings.texturePack);
	selectedScheme=gameSettings.controls;
	
	bottomScreen=API_CreateWindow(-128, 0, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	singleWindow=API_CreateWindow(-128, 0, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	textureWindow=API_CreateWindow(-128, 0, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	optionWindow=API_CreateWindow(-128, 0, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	genWindow=API_CreateWindow(-128, 0, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	kbWindow=API_CreateWindow(-128, 0, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	confirmWindow=API_CreateWindow(-128, 0, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	creditsWindow=API_CreateWindow(-128, 0, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	
	schemeWindows[0]=API_CreateWindow(-128+32, 16-192, 128, 256, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "control1_1.pcx");
	schemeWindows[1]=API_CreateWindow(-128+32+128, 16-192, 64, 256, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "control1_2.pcx");
	
	schemeWindows[2]=API_CreateWindow(-128+32, 16-192, 128, 256, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "control2_1.pcx");
	schemeWindows[3]=API_CreateWindow(-128+32+128, 16-192, 64, 256, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "control2_2.pcx");
	
	schemeWindows[4]=API_CreateWindow(-128+32, 16-192, 128, 256, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "control3_1.pcx");
	schemeWindows[5]=API_CreateWindow(-128+32+128, 16-192, 64, 256, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "control3_2.pcx");
	
	//main menu
	API_CreateButtonFather(128-strlen("Singleplayer")*4, 32+0, RGB15(31,31,31), &Menu_Singleplayer, bottomScreen, "Singleplayer", "button.pcx", false);
	API_CreateButtonFather(128-strlen("Texture packs")*4, 32+20, RGB15(31,31,31), &Menu_Texture, bottomScreen, "Texture packs", "button.pcx", false);
	API_CreateButtonFather(128-strlen("Options")*4, 32+40, RGB15(31,31,31), &Menu_Option, bottomScreen, "Options", "button.pcx", false);
	API_CreateButtonFather(128-strlen("Credits")*4, 32+60, RGB15(31,31,31), &Menu_Credits, bottomScreen, "Credits", "button.pcx", false);

	//credits
	{
		int i;
		for(i=0;i<CREDIT_LINES;i++)
			creditLabels[i]=API_CreateLabelFather(128-GENTEXT_LEN*4, 24+i*13, RGB15(31,31,31), creditsWindow, "                              ", true);
		creditPageLabel=API_CreateLabelFather(128-GENTEXT_LEN*4, 146, RGB15(31,31,31), creditsWindow, "                              ", true);
		API_CreateButtonFather(16, 192-32+16, RGB15(31,31,31), &Menu_CreditsNext, creditsWindow, "Next", "button.pcx", false);
		API_CreateButtonFather(256-64+16, 192-32+16, RGB15(31,31,31), &Menu_Back, creditsWindow, "Back", "button.pcx", false);
	}
	
	//new world
	API_CreateLabelFather(128-strlen("New world")*4, 16, RGB15(31,31,31), genWindow, "New world", true);
	genNameTitle=API_CreateLabelFather(32, 38, RGB15(31,31,31), genWindow, "Name:", false);
	genNameButton=API_CreateButtonFather(80, 34, RGB15(31,31,31), &Menu_EditName, genWindow, "                ", "button.pcx", false);
	genSeedTitle=API_CreateLabelFather(32, 60, RGB15(31,31,31), genWindow, "Seed:", false);
	genSeedButton=API_CreateButtonFather(80, 56, RGB15(31,31,31), &Menu_EditSeed, genWindow, "                ", "button.pcx", false);
	genModes[0]=API_CreateCheckBoxFather(40, 82, &Menu_SelectMode, genWindow, "Survival", false);
	genModes[1]=API_CreateCheckBoxFather(140, 82, &Menu_SelectMode, genWindow, "Creative", false);
	genFlat=API_CreateCheckBoxFather(40, 102, &Menu_ToggleFlat, genWindow, "Superflat", false);
	genCreate=API_CreateButtonFather(128-strlen("Create")*4, 124, RGB15(31,31,31), &Menu_CreateWorld, genWindow, "Create", "button.pcx", false);
	genLabel=API_CreateLabelFather(128-GENTEXT_LEN*4, 150, RGB15(31,31,31), genWindow, "                              ", true);

	//keyboard: 1-0, q-p, a-l, z-m like a keyboard, then shift, space, delete, OK
	{
		static const u8 rowStart[4]={0,10,20,29}, rowLen[4]={10,10,9,8}, rowX[4]={38,38,47,65};
		int r, c;
		kbNameLabel=API_CreateLabelFather(128-GENTEXT_LEN*4, 16, RGB15(31,31,31), kbWindow, "                              ", true);
		for(r=0;r<4;r++)for(c=0;c<rowLen[r];c++)
		{
			char key[2]={kbChars[rowStart[r]+c],0};
			kbKeys[rowStart[r]+c]=API_CreateButtonFather(rowX[r]+c*18, 40+r*22, RGB15(31,31,31), &Menu_Key, kbWindow, key, "button.pcx", false);
		}
		API_CreateButtonFather(24, 134, RGB15(31,31,31), &Menu_KeyShift, kbWindow, "Shift", "button.pcx", false);
		API_CreateButtonFather(128-strlen("Space")*4, 134, RGB15(31,31,31), &Menu_KeySpace, kbWindow, "Space", "button.pcx", false);
		API_CreateButtonFather(256-24-strlen("Del")*8-8, 134, RGB15(31,31,31), &Menu_KeyDelete, kbWindow, "Del", "button.pcx", false);
		API_CreateButtonFather(256-64+16, 192-32+16, RGB15(31,31,31), &Menu_KeyOK, kbWindow, "OK", "button.pcx", false);
	}

	//delete confirmation
	confirmLine1=API_CreateLabelFather(128-GENTEXT_LEN*4, 56, RGB15(31,31,31), confirmWindow, "                              ", true);
	confirmLine2=API_CreateLabelFather(128-GENTEXT_LEN*4, 76, RGB15(31,31,31), confirmWindow, "                              ", true);
	confirmDelete=API_CreateButtonFather(56, 112, RGB15(31,31,31), &Menu_ConfirmDelete, confirmWindow, "Delete", "button.pcx", false);
	API_CreateButtonFather(144, 112, RGB15(31,31,31), &Menu_CancelDelete, confirmWindow, "Cancel", "button.pcx", false);
	API_CreateButtonFather(256-64+16, 192-32+16, RGB15(31,31,31), &Menu_CancelWorld, genWindow, "Back", "button.pcx", false);
	
	//singleplayer
	singleTitle=API_CreateLabelFather(128-24*4, 16, RGB15(31,31,31), singleWindow, "                        ", true);
	deleteButton=API_CreateButtonFather(16, 192-32+16, RGB15(31,31,31), &Menu_ToggleDelete, singleWindow, "Delete", "button.pcx", false);
	deleteMode=false;
	updateDeleteMode();
	API_CreateButtonFather(256-64+16, 192-32+16, RGB15(31,31,31), &Menu_Back, singleWindow, "Back", "button.pcx", false);
	API_CreateVSliderFather(256-32, 32, 80, &Menu_SingleSlider, singleWindow, "", true);
	listFiles("worlds", &worldList, singleWindow, &Menu_StartGame, 1);
	API_CreateButtonFather(128-strlen("New world")*4, 192-32+16, RGB15(31,31,31), &Menu_NewWorld, singleWindow, "New world", "button.pcx", false);
	
	//texture
	API_CreateLabelFather(128-strlen("Select texture pack")*4, 16, RGB15(31,31,31), textureWindow, "Select texture pack", true);
	API_CreateButtonFather(256-64+16, 192-32+16, RGB15(31,31,31), &Menu_BackOptions, textureWindow, "Back", "button.pcx", false);
	API_CreateVSliderFather(256-32, 32, 80, &Menu_TextureSlider, textureWindow, "", true);
	listFiles("packs", &packList, textureWindow, &Menu_ChangePack, 1);
	
	//options
	API_CreateLabelFather(128-strlen("Options")*4, 16, RGB15(31,31,31), optionWindow, "Options", true);
	API_CreateButtonFather(256-64+16, 192-32+16, RGB15(31,31,31), &Menu_BackOptions, optionWindow, "Back", "button.pcx", false);
	schemes[0]=API_CreateCheckBoxFather(64, 64, Menu_SelectScheme, optionWindow, "Control scheme 1", true);
	schemes[1]=API_CreateCheckBoxFather(64, 64+20, Menu_SelectScheme, optionWindow, "Control scheme 2", true);
	schemes[2]=API_CreateCheckBoxFather(64, 64+40, Menu_SelectScheme, optionWindow, "Control scheme 3", true);
	
	selectedScheme%=3;
	Menu_SelectScheme(schemes[selectedScheme]);
	
	topScreen=API_CreateWindow(-128, -192, 256, 192, 31, RGB15(31,31,31), 0, RGB15(0,0,0), "transp.pcx");
	NOGBA("window done");
	
	API_SetAlpha(schemeWindows[0],0);
	API_SetAlpha(schemeWindows[1],0);
	API_SetAlpha(schemeWindows[2],0);
	API_SetAlpha(schemeWindows[3],0);
	API_SetAlpha(schemeWindows[4],0);
	API_SetAlpha(schemeWindows[5],0);
	
	API_SetAlpha(singleWindow,0);
	API_SetAlpha(textureWindow,0);
	API_SetAlpha(optionWindow,0);
	API_SetAlphaSons(singleWindow,0);
	API_SetAlphaSons(textureWindow,0);
	API_SetAlphaSons(optionWindow,0);
	API_SetAlpha(genWindow,0);
	API_SetAlphaSons(genWindow,0);
	API_SetAlpha(kbWindow,0);
	API_SetAlphaSons(kbWindow,0);
	API_SetAlpha(confirmWindow,0);
	API_SetAlphaSons(confirmWindow,0);
	API_SetAlpha(creditsWindow,0);
	API_SetAlphaSons(creditsWindow,0);
	
	initEnvironment(true);
	logo=Game_CreateAlphaMask("logo.pcx", "textures");
	loadBlockTextures(false, true);
	
	testAngleX=-9990;
	testAngleZ=4800;
	subtestAngleX=-9200;
	subtestAngleZ=-2200;
	logoPos.x=346832;
	logoPos.y=81800;
	
	API_ComputeDirections(&API_List, 0);
	
	if(showWorldsOnInit)
	{
		showWorldsOnInit=false;
		Menu_Singleplayer(NULL);
	}
	
	NOGBA("MEMORY : %dko used, %dko free    \n",DS_UsedMem()/1024,DS_FreeMem()/1024);
	Game_GetVramStatus();
}

int button;

//29;106 : 105;139
// Both screens are drawn by the single 3D engine, one per frame. Swapping the
// screens and switching the capture banks is only safe during the vertical
// blank: done any later, the first lines of a screen show the other screen's
// image. So they come first, right after the blank starts, and everything that
// takes time (button actions, world generation, drawing) comes after them.

void Menu_Frame(void)
{
	menuFrames++;
	swiWaitForVBlank();
	if(singleScreen)
	{
		// one screen: always the menu's own scene, on the top screen
		D3D_Screen=true;
		APIcall();
		if(keysDown() & KEY_B)Menu_CancelWorld(NULL);
		updateNewWorld();
		RenderScene();
		return;
	}
	lcdSwap();                     // every frame, the first one too: it sets which scene each screen gets
	Game_UpdateD3D();
	APIcall();
	updateNewWorld();
	RenderScene();
	// both screens hold a real image after two frames: show them (the game
	// starts with the screens dimmed, see main)
	if(menuRendered)setBrightness(3,0);
	menuRendered=true;
}

void Menu_Kill(void)
{
	if(generating){worldgenCancel();generating=false;}
	API_CleanUp();
	free(sceneList1);
	free(subSceneList1);
	freeEnvironment();
	DS_freeState(&Menu_State);
}


void Menu_VBlank(void)
{
}

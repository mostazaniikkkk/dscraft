#include "game/game_main.h"

// Pictures for the farm blocks and items that the Beta 1.7 texture packs do
// not have (carrots and pumpkin stems came with later versions). Drawn in
// Minecraft's style; the stem is grey and tinted by its stage the way
// Minecraft's BlockStem.getRenderColor tints it.

static const char* const carrotCrop[4][16]={
	{	"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"..G......G....G.",
		".GdG....GdG..GdG",
		"..d......d....d.",
		"..d......d....d."},
	{	"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"..G......G....G.",
		".G.G..G.G.G..G..",
		"..GdG..GGdG.GG.G",
		".GdGdG.GdG.GdGd.",
		"..Gd...dGd...dG.",
		"...d.....d...d..",
		"...d.....d...d.."},
	{	"................",
		"................",
		"................",
		"................",
		"................",
		"..G.......G.....",
		".GG..G...GG...G.",
		"..GG.G..G.GG.GG.",
		".GdGG..GGdG.GdG.",
		"GdGdG.GdGdGGdGdG",
		".GdGdGGdGdG.GdG.",
		"..GdG..GdG..GdGG",
		".GdGdG.GdGdGdGd.",
		"..Gd...dGd...dG.",
		"...d.....d...d..",
		"...d.....d...d.."},
	{	"................",
		"................",
		"................",
		"..G.......G.....",
		".GG..G...GG..G..",
		"..GG.G..G.GG.GG.",
		".GdGG..GGdG.GdGG",
		"GdGdG.GdGdGGdGdG",
		".GdGdGGdGdG.GdG.",
		"..GdGG.GdGG.GdGG",
		".GdGdG.GdGdGdGd.",
		"GGdGd.GGdGd.GdGG",
		"..Gd...dGd...dG.",
		"..OoO...OoO..OoO",
		"..oro...oro..oro",
		"...r.....r....r."}};

static const char* const stem[16]={
	"........W.......",
	".......Ws.......",
	".....sWw........",
	"......Ws........",
	".......ws.......",
	".......wWs......",
	".......ws.Ws....",
	"......sws.......",
	".....Ws.ws......",
	".......wsW......",
	".......ws.Ws....",
	"......Wws.......",
	".....s.ws.......",
	".......wsW......",
	".......ws.......",
	".......ws......."};

static const char* const stemBent[16]={
	"................",
	"................",
	"................",
	"................",
	"..........WWWWww",
	"........Wwwssss.",
	".......Wws......",
	"......Wws.......",
	".......ws.Ws....",
	"......sws.......",
	".....Ws.ws......",
	".......wsW......",
	".......ws.Ws....",
	"......Wws.......",
	".......ws.......",
	".......ws......."};

static const char* const carrotItem[16]={
	"................",
	"............G...",
	"..........GGdG..",
	"...........GdGG.",
	"..........OdGd..",
	".........OoOG...",
	"........OoOo....",
	".......OoroO....",
	"......OorOo.....",
	".....Ooroo......",
	"....OoOro.......",
	"...Oorro........",
	"...oror.........",
	"..ooro..........",
	"..rr............",
	"................"};

static const char* const seeds[16]={
	"................",
	"................",
	"................",
	"................",
	"................",
	"........bb......",
	".......bCcb.....",
	"...bb..bCcb.....",
	"..bCcb.bccb.....",
	"..bCcb..bb......",
	"..bccb.....bb...",
	"...bb.....bCcb..",
	"..........bCcb..",
	"..........bccb..",
	"...........bb...",
	"................"};

static u16 color(char c)
{
	switch(c)
	{
		case 'G': return RGB15(12,22,6)|BIT(15);     // leaf
		case 'd': return RGB15(6,14,3)|BIT(15);      // leaf shade
		case 'O': return RGB15(31,20,6)|BIT(15);     // carrot
		case 'o': return RGB15(29,15,3)|BIT(15);
		case 'r': return RGB15(22,10,1)|BIT(15);
		case 'W': return RGB15(30,30,30)|BIT(15);    // stem (grey, tinted)
		case 'w': return RGB15(24,24,24)|BIT(15);
		case 's': return RGB15(17,17,17)|BIT(15);
		case 'C': return RGB15(31,30,26)|BIT(15);    // seed
		case 'c': return RGB15(28,27,21)|BIT(15);
		case 'b': return RGB15(20,18,12)|BIT(15);
	}
	return 0;
}

static void draw(const char* const* rows, u16* out, bool mirror)
{
	int x, y;
	for(y=0;y<16;y++)for(x=0;x<16;x++)out[x+y*16]=color(rows[y][mirror?15-x:x]);
}

// BlockStem.getRenderColor: red stage*32, green 255-stage*8, blue stage*4
static void tint(u16* out, int stage)
{
	int n, r=stage*32, g=255-stage*8, b=stage*4;
	for(n=0;n<256;n++)
	{
		u16 c=out[n];
		if(!(c&BIT(15)))continue;
		out[n]=BIT(15)|RGB15((c&31)*r/255,((c>>5)&31)*g/255,((c>>10)&31)*b/255);
	}
}

void farmArtTile(int tile, u16* out)
{
	int n;
	memset(out,0,256*sizeof(u16));
	if(tile>=CARROT_TILE && tile<CARROT_TILE+4)draw(carrotCrop[tile-CARROT_TILE],out,false);
	else if(tile>=STEM_TILE && tile<STEM_TILE+8)
	{
		// the stem shows only its lower (stage*2+2)/16, as Minecraft draws it
		int stage=tile-STEM_TILE, h=stage*2+2;
		draw(stem,out,false);
		for(n=0;n<(16-h)*16;n++)out[n]=0;
		tint(out,stage);
	}
	else if(tile==STEM_BENT_TILE || tile==STEM_BENT_TILE+1)
	{
		draw(stemBent,out,tile!=STEM_BENT_TILE);
		tint(out,7);
	}
	else if(tile==CARROT_ITEM_TILE)draw(carrotItem,out,false);
	else if(tile==SEEDS_TILE)draw(seeds,out,false);
}

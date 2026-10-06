#include "game/game_main.h"
#include <math.h>

// Terrain: layered value noise for the height, bedrock / stone / dirt / grass,
// sand around the sea, water up to sea level, oak trees on grass, and patches
// of pumpkins and of wild carrots.
// Everything is a deterministic function of the seed, so the generator only
// keeps a sliding window of five rows of columns in memory.

#define CS WORLDGEN_COLUMNS
#define WB (CS*4)                  // world size in blocks
#define ZH 64                      // world height in blocks
#define COLBYTES 1024
#define ROWBYTES (CS*COLBYTES)

#define B_AIR 0
#define B_GRASS 1
#define B_DIRT 2
#define B_STONE 3
#define B_BEDROCK 5
#define B_SAND 6
#define B_LOG 8
#define B_LEAVES 10
#define B_COAL 61                  // coal ore (ITEM_COAL_ORE)
#define B_IRON ITEM_IRON_ORE

// Coal ore, as Minecraft Beta's WorldGenMinable: 20 veins of 16 per 16x16
// chunk over 128 blocks of height there; this world is 64 high, so 10 veins
// keep the same density. Each vein walks 17 points along a short random
// segment and turns the stone inside a small ellipsoid around each point into
// ore. A vein can reach into the next chunk, as in Minecraft.
#define CHUNK 16
#define CHUNKS (WB/CHUNK)
#define COAL_VEINS 10
#define COAL_SIZE 16
// Iron ore: Beta's 20 veins of 8 per chunk below height 64 of 128, so here 10
// veins below 32: about half as common as coal, and only in the deeper half.
#define IRON_VEINS 10
#define IRON_SIZE 8
#define IRON_HEIGHT 32
#define ORBS (COAL_VEINS*(COAL_SIZE+1)+IRON_VEINS*(IRON_SIZE+1))

typedef struct
{
	s16 x, y, k;                   // centre, 1/16 block
	u16 r2;                        // squared radius, (1/16 block)^2
	u16 rad;                       // radius rounded up, 1/16 block
	u8 ore;
}orb_struct;

static FILE* out;
static char outPath[256];
static u32 seed;
static bool flat;                  // superflat world
static void (*decorateHeader)(u8* header);
// Work is split in small units (one column of 4x4 blocks each) so a caller can
// spread it over frames: the menu must finish every frame in time because it
// draws both screens with the single 3D engine, one per frame.
static int row;                    // row of columns being written
static int writeCol;
static int genRow, genCol;         // next row/column to generate (genCol==CS: trees)
static int highRow, highCol;       // next row/column whose highest blocks are computed
static u8* rows[5];                // block rows, ring buffer indexed by row%5
static u8* high[3];                // highest visible opaque block, indexed by row%3
static u8 record[2*COLBYTES];
static orb_struct* orbRows[3];     // ore veins of three rows of chunks
static int orbRowId[3];
static float sinTable[COAL_SIZE+1];

/* ---------------------------------------------------------------------------
 * Noise and terrain shape
 * ------------------------------------------------------------------------- */

static u32 hash3(int x, int y, u32 s)
{
	u32 h=(u32)x*374761393u+(u32)y*668265263u+s*2246822519u;
	h=(h^(h>>13))*1274126177u;
	return h^(h>>16);
}

static int smooth(int f) // 0..256 -> 0..256, smoothstep
{
	return f*f*(768-2*f)/65536;
}

static int noise(int x, int y, int scale, u32 s) // 0..255
{
	int gx=x/scale, gy=y/scale;
	int fx=smooth((x%scale)*256/scale), fy=smooth((y%scale)*256/scale);
	int a=hash3(gx,gy,s)&255, b=hash3(gx+1,gy,s)&255;
	int c=hash3(gx,gy+1,s)&255, d=hash3(gx+1,gy+1,s)&255;
	int top=a+(((b-a)*fx)>>8), bottom=c+(((d-c)*fx)>>8);
	return top+(((bottom-top)*fy)>>8);
}

static int terrainHeight(int x, int y)
{
	if(flat)return WORLDGEN_FLAT_TOP;
	int h=12+noise(x,y,64,seed)*24/255+noise(x,y,32,seed+1)*12/255
	        +noise(x,y,16,seed+2)*6/255+noise(x,y,8,seed+3)*3/255;
	if(h<4)h=4;
	if(h>ZH-10)h=ZH-10;
	return h;
}

/* ---------------------------------------------------------------------------
 * Block window
 * ------------------------------------------------------------------------- */

static inline int blockIndex(int x, int y, int k)
{
	return (x/4)*COLBYTES+(x%4)+(y%4)*4+k*16;
}

static inline u8 blk(int x, int y, int k)
{
	if(x<0 || y<0 || k<0 || x>=WB || y>=WB || k>=ZH)return B_BEDROCK; // outside counts as solid
	return rows[(y/4)%5][blockIndex(x,y,k)];
}

static inline bool seeThroughBlock(u8 t)
{
	return !t || t>=WATERTYPE || t==13 || t==12 || t==B_LEAVES || isPlant(t) || isLadder(t) || isDoor(t);
}

// same rule as transparent3() in map.h
static inline bool showsFace(u8 self, u8 other)
{
	return (other==0 || other>=WATERTYPE || other==12 || other==13 || isPlant(other) || isLadder(other) || isDoor(other))
		&& !(other==self || (other>=WATERTYPE && self>=WATERTYPE));
}

static u8 faces(int x, int y, int k)
{
	u8 b=blk(x,y,k);
	if(!b)return 0;
	if(isDoor(b) || isLadder(b))return 1;
	return (showsFace(b,blk(x,y,k-1)))
		|(showsFace(b,blk(x,y,k+1))<<1)
		|(showsFace(b,blk(x,y-1,k))<<2)
		|(showsFace(b,blk(x,y+1,k))<<3)
		|(showsFace(b,blk(x-1,y,k))<<4)
		|(showsFace(b,blk(x+1,y,k))<<5);
}

static void setInRow(u8* buf, int r, int x, int y, int k, u8 type, bool onlyAir)
{
	if(y<r*4 || y>=r*4+4 || x<0 || x>=WB || k<0 || k>=ZH)return;
	u8* p=&buf[blockIndex(x,y,k)];
	if(!onlyAir || !*p)*p=type;
}

static bool treeAt(int x, int y)
{
	if(flat)return false;
	if(x<2 || y<2 || x>=WB-2 || y>=WB-2 || hash3(x,y,seed+77)%61)return false;
	int h=terrainHeight(x,y);
	return h>WORLDGEN_SEA+1 && h<=ZH-10;
}

static bool featureAt(int x, int y);

static bool goodSpawn(int x, int y)
{
	int dx, dy;
	if(flat)return true;
	if(terrainHeight(x,y)<=WORLDGEN_SEA+1)return false;
	for(dy=-2;dy<=2;dy++)for(dx=-2;dx<=2;dx++)if(treeAt(x+dx,y+dy) || featureAt(x+dx,y+dy))return false;
	return true;
}

static void placeTree(u8* buf, int r, int tx, int ty, int h, u32 hv)
{
	int k, dx, dy;
	int top=h+4+((hv>>8)&1);
	for(k=top-1;k<=top+1;k++)
	{
		int rad=(k<=top)?2:1;
		for(dy=-rad;dy<=rad;dy++)for(dx=-rad;dx<=rad;dx++)
		{
			if(abs(dx)==rad && abs(dy)==rad && (rad==1 || ((hv>>(16+(k-top+1)*4+(dx>0)+(dy>0)*2))&1)))continue;
			setInRow(buf,r,tx+dx,ty+dy,k,B_LEAVES,true);
		}
	}
	for(k=h+1;k<=top;k++)setInRow(buf,r,tx,ty,k,B_LOG,false);
}

static u32 rngState;
static u32 rngNext(void)
{
	rngState^=rngState<<13;
	rngState^=rngState>>17;
	rngState^=rngState<<5;
	return rngState;
}
static int rngInt(int n){ return rngNext()%n; }
static float rngFloat(void){ return (rngNext()>>8)/16777216.0f; }

/* ---------------------------------------------------------------------------
 * Pumpkins and wild carrots
 *
 * Minecraft Beta's WorldGenPumpkin, in one chunk out of 32: 64 tries around a
 * point, each up to 7 blocks away and 3 up or down, keep the ones on top of
 * grass. Beta picks the point's height at random over the whole world height,
 * so most patches find no ground; in a 256x256 world that would leave about
 * one patch per world, so the point is put on the ground instead. Wild carrots
 * (Minecraft only has them in village farms) grow the same way, grown, on a
 * patch of farmland.
 * ------------------------------------------------------------------------- */

typedef struct
{
	u16 x, y;
	u8 k, type;
}feature_struct;

static feature_struct* features;
static int featureCount, featureCap;

static bool nearTree(int x, int y)
{
	int dx, dy;
	for(dy=-2;dy<=2;dy++)for(dx=-2;dx<=2;dx++)if(treeAt(x+dx,y+dy))return true;
	return false;
}

static bool featureAt(int x, int y)
{
	int n;
	for(n=0;n<featureCount;n++)if(features[n].x==x && features[n].y==y)return true;
	return false;
}

static void addFeature(int x, int y, int k, u8 type)
{
	if(featureCount==featureCap)
	{
		int cap=featureCap?featureCap*2:256;
		feature_struct* f=realloc(features,cap*sizeof(feature_struct));
		if(!f)return;
		features=f;
		featureCap=cap;
	}
	features[featureCount].x=x;
	features[featureCount].y=y;
	features[featureCount].k=k;
	features[featureCount].type=type;
	featureCount++;
}

static void featurePatch(int cx, int cy, u32 salt, bool carrots, bool force)
{
	int x, y, k, n;
	rngState=hash3(cx,cy,seed+salt)|1;
	if(rngInt(32) && !force)return;
	x=cx*CHUNK+rngInt(CHUNK)+8;
	y=cy*CHUNK+rngInt(CHUNK)+8;
	if(x>=WB || y>=WB)return;
	k=terrainHeight(x,y)+1;
	for(n=0;n<64;n++)
	{
		int x1=x+rngInt(8)-rngInt(8), k1=k+rngInt(4)-rngInt(4), y1=y+rngInt(8)-rngInt(8), h;
		if(x1<1 || y1<1 || x1>=WB-1 || y1>=WB-1)continue;
		h=terrainHeight(x1,y1);
		// air on grass: not on the beach, under water or under a tree
		if(k1!=h+1 || h<=WORLDGEN_SEA+1 || h>=ZH-2 || nearTree(x1,y1) || featureAt(x1,y1))continue;
		if(carrots)
		{
			addFeature(x1,y1,h,FARMLAND_FIRST);
			addFeature(x1,y1,h+1,CARROT_FIRST+7);
		}
		else addFeature(x1,y1,h+1,PUMPKIN_FIRST+rngInt(4));
	}
}

static void planFeatures(void)
{
	int cx, cy, t, n;
	featureCount=0;
	if(flat)return;
	for(cy=0;cy<CHUNKS;cy++)for(cx=0;cx<CHUNKS;cx++)
	{
		featurePatch(cx,cy,555,false,false);
		featurePatch(cx,cy,666,true,false);
	}
	// This world is small and closed: make sure it has at least one patch of
	// each, or pumpkin seeds and carrots could not be had at all.
	for(t=0;t<2;t++)
	{
		u8 want=t?CARROT_FIRST+7:PUMPKIN_FIRST;
		bool found=false;
		for(n=0;n<featureCount && !found;n++)found=t?features[n].type==want:isPumpkin(features[n].type);
		for(n=0;n<CHUNKS*CHUNKS && !found;n++)
		{
			int c=(n+hash3(t,0,seed+999))%(CHUNKS*CHUNKS), before=featureCount;
			featurePatch(c%CHUNKS,c/CHUNKS,t?666:555,t,true);
			found=featureCount>before;
		}
	}
}

static void placeFeatures(int r)
{
	u8* buf=rows[r%5];
	int n;
	for(n=0;n<featureCount;n++)
	{
		feature_struct* f=&features[n];
		if(f->y<r*4 || f->y>=r*4+4)continue;
		setInRow(buf,r,f->x,f->y,f->k,f->type,f->type!=FARMLAND_FIRST);
	}
}

// the veins of one chunk, deterministic in the seed and the chunk position
// count veins of a size, starting below a height: WorldGenMinable
static orb_struct* minable(orb_struct* o, int cx, int cy, int count, int vsize, int height, u8 ore)
{
	int v, l;
	for(v=0;v<count;v++)
	{
		int ox=cx*CHUNK+rngInt(CHUNK), oh=rngInt(height), oy=cy*CHUNK+rngInt(CHUNK);
		float f=rngFloat()*3.14159265f;
		float sx=sinf(f)*vsize/8, sy=cosf(f)*vsize/8;
		float x0=ox+8+sx, x1=ox+8-sx;
		float y0=oy+8+sy, y1=oy+8-sy;
		float h0=oh+rngInt(3)-2, h1=oh+rngInt(3)-2;
		for(l=0;l<=vsize;l++,o++)
		{
			float t=(float)l/vsize;
			float size=rngFloat()*vsize/16;
			float radius=(((vsize==COAL_SIZE?sinTable[l]:sinf(l*3.14159265f/vsize))+1)*size+1)/2;
			o->x=(s16)((x0+(x1-x0)*t)*16);
			o->y=(s16)((y0+(y1-y0)*t)*16);
			o->k=(s16)((h0+(h1-h0)*t)*16);
			o->r2=(u16)(radius*16*radius*16);
			o->rad=(u16)(radius*16)+1;
			o->ore=ore;
		}
	}
	return o;
}

// the veins of one chunk, deterministic in the seed and the chunk position:
// coal, then iron (as Minecraft populates a chunk)
static void chunkVeins(int cx, int cy, orb_struct* out)
{
	rngState=hash3(cx,cy,seed+1234)|1;
	out=minable(out,cx,cy,COAL_VEINS,COAL_SIZE,ZH,B_COAL);
	minable(out,cx,cy,IRON_VEINS,IRON_SIZE,IRON_HEIGHT,B_IRON);
}

// veins of chunk row cy (cached: the generator moves forward row by row)
static orb_struct* veinRow(int cy)
{
	int slot=cy%3, cx;
	if(orbRowId[slot]!=cy)
	{
		for(cx=0;cx<CHUNKS;cx++)chunkVeins(cx,cy,&orbRows[slot][cx*ORBS]);
		orbRowId[slot]=cy;
	}
	return orbRows[slot];
}

static void placeCoal(int r, int cx)
{
	u8* buf=rows[r%5];
	int x0=cx*4, y0=r*4;
	int ccx, ccy, n, i, j, k;
	if(flat)return;
	// veins start in their chunk and reach at most a few blocks further
	for(ccy=y0/CHUNK-1;ccy<=y0/CHUNK;ccy++)
	{
		if(ccy<0)continue;
		orb_struct* rowOrbs=veinRow(ccy);
		for(ccx=x0/CHUNK-1;ccx<=x0/CHUNK;ccx++)
		{
			if(ccx<0)continue;
			orb_struct* o=&rowOrbs[ccx*ORBS];
			for(n=0;n<ORBS;n++,o++)
			{
				int rad=o->rad;
				if(o->x+rad<x0*16 || o->x-rad>=(x0+4)*16 || o->y+rad<y0*16 || o->y-rad>=(y0+4)*16)continue;
				int k0=(o->k-rad)/16-1, k1=(o->k+rad)/16+1;
				if(k0<0)k0=0;
				if(k1>=ZH)k1=ZH-1;
				for(k=k0;k<=k1;k++)for(j=0;j<4;j++)for(i=0;i<4;i++)
				{
					// block centres, as Minecraft tests (x+0.5)
					int dx=(x0+i)*16+8-o->x, dy=(y0+j)*16+8-o->y, dk=k*16+8-o->k;
					if(dx*dx+dy*dy+dk*dk>=o->r2)continue;
					u8* p=&buf[blockIndex(x0+i,y0+j,k)];
					if(*p==B_STONE)*p=o->ore;      // ores only replace stone
				}
			}
		}
	}
}

static void generateColumn(int r, int cx)
{
	u8* buf=rows[r%5];
	int x, y, k;
	memset(&buf[cx*COLBYTES],0,COLBYTES);
	for(y=r*4;y<r*4+4;y++)
	{
		for(x=cx*4;x<cx*4+4;x++)
		{
			int h=terrainHeight(x,y);
			bool beach=!flat && h<=WORLDGEN_SEA+1;
			u8* col=&buf[blockIndex(x,y,0)];
			for(k=0;k<=h;k++)
			{
				u8 t;
				if(!k || (!flat && k<5 && k<h && (int)(hash3(x*5+k,y,seed+31)%5)>=k))t=B_BEDROCK;
				else if(k<h-3)t=B_STONE;
				else if(k<h)t=beach?B_SAND:B_DIRT;
				else t=beach?B_SAND:B_GRASS;
				col[k*16]=t;
			}
			if(!flat)for(k=h+1;k<=WORLDGEN_SEA;k++)col[k*16]=WATERTYPE;
		}
	}
	placeCoal(r,cx);
}

// trees rooted up to two blocks outside this row can reach into it
static void plantTrees(int r)
{
	u8* buf=rows[r%5];
	int x, y;
	for(y=r*4-2;y<r*4+6;y++)
	{
		if(y<2 || y>=WB-2)continue;
		for(x=2;x<WB-2;x++)
		{
			if(!treeAt(x,y))continue;
			placeTree(buf,r,x,y,terrainHeight(x,y),hash3(x,y,seed+77));
		}
	}
	placeFeatures(r);
}

static void computeHighestColumn(int r, int cx)
{
	u8* h=high[r%3];
	int x, j, k;
	for(j=0;j<4;j++)
	{
		for(x=cx*4;x<cx*4+4;x++)
		{
			int y=r*4+j;
			u8 best=0;
			for(k=ZH-1;k>=0;k--)
			{
				if(!seeThroughBlock(blk(x,y,k)) && (faces(x,y,k)&63)){best=k;break;}
			}
			h[x+j*WB]=best;
		}
	}
}

static inline int highestAt(int x, int y)
{
	return high[(y/4)%3][x+(y%4)*WB];
}

/* ---------------------------------------------------------------------------
 * Output
 * ------------------------------------------------------------------------- */

static bool writeColumn(int r, int cx)
{
	int i, j, k, c;
	{
		u8* data=record;
		u8* t=record+COLBYTES;
		memcpy(data,&rows[r%5][cx*COLBYTES],COLBYTES);
		for(k=0;k<ZH;k++)for(j=0;j<4;j++)for(i=0;i<4;i++)
		{
			int x=cx*4+i, y=r*4+j;
			int hh=highestAt(x,y);
			bool ls=(k==hh);
			if(!ls && x<WB-1)ls=k>highestAt(x+1,y);
			if(!ls && y<WB-1)ls=k>highestAt(x,y+1);
			if(!ls && x>0)ls=k>highestAt(x-1,y);
			if(!ls && y>0)ls=k>highestAt(x,y-1);
			t[i+j*4+k*16]=faces(x,y,k)|(ls<<6);
		}
		// occlusion walls: a cluster blocks sight along an axis when every line
		// of four blocks along that axis contains an opaque block
		for(c=0;c<16;c++)
		{
			u8 wall=7;
			for(i=0;i<4;i++)for(j=0;j<4;j++)
			{
				bool d1=false, d2=false, d3=false;
				for(k=0;k<4;k++)
				{
					d1=d1||!seeThroughBlock(data[i+j*4+(c*4+k)*16]);
					d2=d2||!seeThroughBlock(data[i+k*4+(c*4+j)*16]);
					d3=d3||!seeThroughBlock(data[k+j*4+(c*4+i)*16]);
				}
				wall&=d1|(d2<<1)|(d3<<2);
			}
			t[c*64]|=(wall&1)<<7;
			t[c*64+1]|=((wall>>1)&1)<<7;
			t[c*64+2]|=((wall>>2)&1)<<7;
		}
		if(fwrite(record,1,sizeof(record),out)!=sizeof(record))return false;
	}
	return true;
}

static void freeBuffers(void)
{
	int i;
	for(i=0;i<3;i++){if(orbRows[i])free(orbRows[i]);orbRows[i]=NULL;}
	for(i=0;i<5;i++){if(rows[i])free(rows[i]);rows[i]=NULL;}
	for(i=0;i<3;i++){if(high[i])free(high[i]);high[i]=NULL;}
	if(features)free(features);
	features=NULL;
	featureCount=featureCap=0;
}

void worldgenCancel(void)
{
	if(out)
	{
		fclose(out);
		out=NULL;
		remove(outPath);
	}
	freeBuffers();
}

static bool writeHeader(void)
{
	u8 header[2048];
	header_struct* h=(header_struct*)header;
	int x=WB/2, y=WB/2, d;
	memset(header,0,sizeof(header));
	// spawn on dry land, as close to the centre as possible
	for(d=0;d<WB/2-4 && !goodSpawn(x,y);d++)
	{
		x=WB/2+(int)(hash3(d,0,seed+9)%(2*d+1))-d;
		y=WB/2+(int)(hash3(d,1,seed+9)%(2*d+1))-d;
	}
	h->sizeX=CS;
	h->sizeY=CS;
	h->magicVersionNumber=VERSIONMAGIC;
	h->spawnX=x;
	h->spawnY=y;
	h->spawnZ=(terrainHeight(x,y)+3-ZH/2)*rTilesize2;
	if(decorateHeader)decorateHeader(header);
	return fwrite(header,1,sizeof(header),out)==sizeof(header);
}

bool worldgenSeedFromText(const char* text, u32* seed)
{
	const char* p=text;
	bool negative=false, number=true;
	long long v=0;
	int digits=0;
	while(*p==' ')p++;
	if(!*p)return false;           // empty: a random seed
	if(*p=='-'){negative=true;p++;}
	for(;*p;p++)
	{
		if(*p<'0' || *p>'9' || digits>18){number=false;break;}
		v=v*10+(*p-'0');
		digits++;
	}
	if(number && digits)
	{
		if(negative)v=-v;
		// fits in 32 bits: as it is; larger numbers are folded
		if(v>=-2147483648LL && v<=4294967295LL)*seed=(u32)v;
		else *seed=(u32)v^(u32)((unsigned long long)v>>32);
		return true;
	}
	// Java's String.hashCode, which Minecraft uses for text seeds
	u32 h=0;
	for(p=text;*p;p++)h=h*31+(u8)*p;
	*seed=h;
	return true;
}

bool worldgenNewPath(const char* dir, const char* name, char* path, int size)
{
	int n;
	for(n=1;n<1000;n++)
	{
		if(n==1)snprintf(path,size,"%s/%s.map",dir,name);
		else snprintf(path,size,"%s/%s %d.map",dir,name,n);
		FILE* f=fopen(path,"rb");
		if(!f)return true;
		fclose(f);
	}
	return false;
}

bool worldgenStart(const char* path, u32 s, bool isFlat, void (*decorate)(u8* header))
{
	int i;
	worldgenCancel();
	seed=s;
	flat=isFlat;
	decorateHeader=decorate;
	row=0;
	for(i=0;i<5;i++)if(!(rows[i]=malloc(ROWBYTES))){freeBuffers();return false;}
	for(i=0;i<3;i++)if(!(high[i]=malloc(WB*4))){freeBuffers();return false;}
	for(i=0;i<3;i++)
	{
		if(!(orbRows[i]=malloc(CHUNKS*ORBS*sizeof(orb_struct)))){freeBuffers();return false;}
		orbRowId[i]=-1;
	}
	for(i=0;i<=COAL_SIZE;i++)sinTable[i]=sinf(i*3.14159265f/COAL_SIZE);
	planFeatures();
	strncpy(outPath,path,sizeof(outPath)-1);
	outPath[sizeof(outPath)-1]=0;
	out=fopen(outPath,"wb");
	if(!out){freeBuffers();return false;}
	if(!writeHeader()){worldgenCancel();return false;}
	writeCol=0;
	genRow=0; genCol=0;
	highRow=0; highCol=0;
	return true;
}

// One unit of work. Writing row r needs rows r-1..r+1 of blocks and of highest
// blocks; the highest blocks of row h need rows h-1..h+1 of blocks. The ring
// buffers keep five rows of blocks and three of highest blocks, which is enough.
int worldgenStep(void)
{
	if(!out)return -1;
	if(genRow<CS && genRow<=row+2)
	{
		if(genCol<CS)generateColumn(genRow,genCol);
		else plantTrees(genRow);
		if(++genCol>CS){genCol=0;genRow++;}
	}else if(highRow<CS && highRow<=row+1)
	{
		computeHighestColumn(highRow,highCol);
		if(++highCol>=CS){highCol=0;highRow++;}
	}else{
		if(!writeColumn(row,writeCol)){worldgenCancel();return -1;}
		if(++writeCol>=CS){writeCol=0;row++;}
		if(row>=CS)
		{
			if(fclose(out)){out=NULL;remove(outPath);freeBuffers();return -1;}
			out=NULL;
			freeBuffers();
			return 100;
		}
	}
	return (row*CS+writeCol)*100/(CS*CS);
}

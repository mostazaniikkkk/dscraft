#ifndef MAP_9
#define MAP_9

#define LADDERTYPE 40
#define DOORTYPE 44

#define BLOCKTEXTURES 75         // 45, 46: crafting table; 47..53: tools and stick (items.png); 54: coal ore; 55: coal (items.png); 56..62: chest; 63..66: furnace; 67: charcoal (items.png); 68: sapling; 69: apple (items.png); 70..74: farmland and pumpkin (75..92: farm.h)
#define BLOCKS 46

#ifdef FOGLIGHT
	#define LIGHTAMBIENT 4
	#define LIGHTAMBIENT2 7
	#define SUNAMBIENT 10
#else
	#define LIGHTAMBIENT 10
	#define LIGHTAMBIENT2 10
	#define SUNAMBIENT 6
#endif
#define MOONLIGHT 8

#define CACHESIZE 1024*50
#define CACHEBLOCK 64

#define LIGHTCACHESIZE 1024*4
#define LIGHTCACHEBLOCK 1024

#define WATERNUMBER 8*1024
#define WATERNUMBER2 4*1024
#define WATERMIN 8
#define WATERMAX 120
#define WATERTYPE 239
#define WATERSPREAD 6

#define CLUSTERSIZE 4
#define SUPERCLUSTERSIZE 32

#define LISTSIZE 4096
#define SCALEFACTOR 32
#define tilesize 1
// quad directions: 0..5 faces, 6..9 crossed planes (torch, plants), 10..13 door
// and ladder faces, 14..17 a plane through the middle (bent pumpkin stem)
#define QUAD_DIRECTIONS 18
#define tilesize2 (tilesize*2)
#define bsize ((tilesize2*CLUSTERSIZE)<<6)
#define bsizeR ((tilesize2*CLUSTERSIZE))
#define max(a,b) ((a)<(b))?(b):(a)
#define min(a,b) ((a)>(b))?(b):(a)
#define qgetCluster(m, i, j, k) ((i)+(j)*SUPERCLUSTERSIZE+(k)*SUPERCLUSTERSIZE*SUPERCLUSTERSIZE)

#define rTilesize ((tilesize*SCALEFACTOR)<<6)
#define rTilesize2 ((tilesize2*SCALEFACTOR)<<6)

#define dir_x (1)
#define dir_y (1<<1)
#define dir_z (1<<2)

#define t1 NORMAL_PACK(-32,-32,0)
#define t2 NORMAL_PACK(-32,32,0)
#define t3 NORMAL_PACK(32,32,0)
#define t4 NORMAL_PACK(32,-32,0)

bool testBuffer;

typedef struct
{
	int32 x, y, z;
}vect3D;

typedef struct
{
	u16 sizeX, sizeY;
	u32 magicVersionNumber;
	u16 spawnX, spawnY;
	int32 spawnZ;
}header_struct;

typedef struct
{
	s8 i, j, k;
	u8 level;                      // 14: torch, 13: lit furnace
	void* next;
} __attribute__((__packed__)) lightsource_struct;

typedef struct
{
	void *first;
	u8 count;
}lightsourceList_struct;

typedef struct
{
	u8 mID;
	u8 direction, type, light;
	void* next;
} __attribute__((__packed__)) quad_struct;

typedef struct
{
	quad_struct *first;
	u16 count;
}quadList_struct;

typedef struct
{
	quadList_struct quadList, specialList;
	lightsourceList_struct lightList;
	u8 wall;
}cluster_struct;

typedef struct
{
	u8 data[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*16];
	u8 highest[CLUSTERSIZE*CLUSTERSIZE];
	cluster_struct cluster[16];
	u8 changed;
}clusterColumn_struct;

typedef struct
{
	header_struct* header;
	vect3D size, clusterSize, offset;
	clusterColumn_struct* superCluster[32][32];
	clusterColumn_struct* transitionCluster[4*32];
	u8 transitionStuff[4*4*4*16];
	u8 transitioning[4];
	u16 clusterDraw[SUPERCLUSTERSIZE*SUPERCLUSTERSIZE*16];
	u16 clusterDrawn[SUPERCLUSTERSIZE*SUPERCLUSTERSIZE*16];
	u32* headerMap;
	u32* fileMap;
	void* fileHandle;
}map_struct;

typedef struct
{
	u16 i, j, k;
	u8 direction;
}listElement_struct;

typedef struct
{
	listElement_struct elements[LISTSIZE];
	u16 size;
}list_struct;

typedef struct
{
	u16 i, j, k;
	void* next;
}toProcess_struct;

typedef struct
{
	void* first;
	u16 count;
}toProcessList_struct;

typedef struct
{
	u32 pos;
} __attribute__((__packed__)) water_struct;

typedef struct
{
	u8 top, side, bottom;
}block_struct;

typedef struct
{
	u8 i, j;
}blockTex_struct;

extern const block_struct blocks[];
extern const blockTex_struct blockTextures[];

char packPath[255];
char mapPath[255];

s16 cubeAngleX;

water_struct waterSpread[WATERNUMBER];
water_struct waterToSpread[WATERNUMBER2];
u32 waterCount, waterCursor;
u32 waterCount2, waterCursor2;

list_struct openList, closedList;

toProcessList_struct lightProcess;

MTL_img* cursorTexture;
MTL_img* waterTexture;
MTL_img* blockSuperTexture;
MTL_img *crossHair;
u8 degradTable[8*8*5];
u8 lightTable[16*16*16*QUAD_DIRECTIONS];
u8 lightComputeTable[256*32];
u8 lightSun[6];

map_struct map;

u16 testquads;
s8 cursorDir;

vect3D waterAnim;
u32* uvMapCur;
u32* lightMapCur;
u32 uvMap[256*4];
u32 uvMapWater[4*6];
u32 lightMap[256*QUAD_DIRECTIONS];
u32 lightMap2[256*QUAD_DIRECTIONS];
u32 xyMap[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*QUAD_DIRECTIONS*4];
u8 imIDtable[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE];
u8 jmIDtable[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE];
u8 kmIDtable[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE];
u32 ijkmIDtable2[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE];

quad_struct* cache[CACHESIZE];
u16 cacheNumber;
u16 cacheCursor;
u16 cacheRecord;

lightsource_struct* lightCache[LIGHTCACHESIZE];
u16 lightCacheNumber;
u16 lightCacheCursor;
u16 lightCacheRecord;

u8 fogMode;

bool cull;

int TESTVALUE;
int TESTVALUE2;
int TESTVALUE3;

bool readSectors(u32 sector, u32 number, u8* buffer);
bool writeSectors(u32 sector, u32 number, u8* buffer);

void loadBlockTextures(bool spr, bool tex);
void initSuperCluster(map_struct* m);
void initMap(map_struct* m, vect3D size);
void addBlock(map_struct* m, int i, int j, int k);
void fixGap(map_struct* m, int i, int j, int k);
void renderClusterList(map_struct* m, int x, int y, int z);

void (*readClusterColumn)(map_struct*, u16, u16, clusterColumn_struct*, u8*, void*);
void readClusterColumn2048(map_struct*, u16, u16, clusterColumn_struct*, u8*, void*);
void readClusterColumn1024(map_struct*, u16, u16, clusterColumn_struct*, u8*, void*);
void readClusterColumn512(map_struct*, u16, u16, clusterColumn_struct*, u8*, void*);
void readClusterColumnNOCASH(map_struct*, u16, u16, clusterColumn_struct*, u8*, void*);

void (*writeClusterColumn)(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f);
void writeClusterColumn2048(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f);
void writeClusterColumn1024(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f);
void writeClusterColumn512(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f);
void writeClusterColumnNOCASH(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f);

void (*openMap)(char*, map_struct*);
void openMap2048(char*, map_struct*);
void openMap1024(char*, map_struct*);
void openMap512(char*, map_struct*);
void openMapNOCASH(char*, map_struct*);

static inline vect3D getCluster(map_struct* m, int i, int j, int k)
{
	if(i<0 || j<0 || k<0 || i>=m->size.x || j>=m->size.y || k>=m->size.z)return (vect3D){0,0,0};
	return (vect3D){(i-i%CLUSTERSIZE)/CLUSTERSIZE,(j-j%CLUSTERSIZE)/CLUSTERSIZE,(k-k%CLUSTERSIZE)/CLUSTERSIZE};
}

static inline u8* getBlockPE(map_struct* m, int i, int j, int k)
{
	if(i<0 || j<0 || k<0 || i>=m->size.x || j>=m->size.y || k>=m->size.z)return &m->superCluster[0][0][0].data[0];
	vect3D cluster=getCluster(m, i, j, k);
	i-=cluster.x*CLUSTERSIZE;j-=cluster.y*CLUSTERSIZE;
	cluster.x-=m->offset.x;cluster.y-=m->offset.y;cluster.z-=m->offset.z;
	if((cluster.x<0 && cluster.y<0) || (cluster.x>=32 && cluster.y<0) || (cluster.x>=32 && cluster.y>=32) || (cluster.x<0 && cluster.y>=32) || cluster.x<-1 || cluster.y<-1 || cluster.x>32 || cluster.y>32 || cluster.z<0 || cluster.z>=32)return &m->superCluster[0][0]->data[0];
	if(cluster.x==-1)return &m->transitionCluster[cluster.y]->data[i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE];
	else if(cluster.x==32)return &m->transitionCluster[32+cluster.y]->data[i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE];
	else if(cluster.y==-1)return &m->transitionCluster[32*2+cluster.x]->data[i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE];
	else if(cluster.y==32)return &m->transitionCluster[32*3+cluster.x]->data[i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE];
	return &m->superCluster[cluster.x][cluster.y]->data[i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE];
}

static inline u8* getBlockP(map_struct* m, int i, int j, int k)
{
	if(i<0 || j<0 || k<0 || i>=m->size.x || j>=m->size.y || k>=m->size.z)return &m->superCluster[0][0][0].data[0];
	vect3D cluster=getCluster(m, i, j, k);
	i-=cluster.x*CLUSTERSIZE;j-=cluster.y*CLUSTERSIZE;
	cluster.x-=m->offset.x;cluster.y-=m->offset.y;cluster.z-=m->offset.z;
	if(cluster.x<0 || cluster.y<0 || cluster.z<0 || cluster.x>=32 || cluster.y>=32 || cluster.z>=32)return &m->superCluster[0][0]->data[0];
	return &m->superCluster[cluster.x][cluster.y]->data[i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE];
}

static inline bool isLadder(u8 t)
{
	return (t>=LADDERTYPE && t<LADDERTYPE+4);
}

static inline bool isDoor(u8 t)
{
	return (t>=DOORTYPE && t<DOORTYPE+16);
}

static inline bool isOpenDoor(u8 t)
{
	return (t>=DOORTYPE+8 && t<DOORTYPE+16);
}

#include "game/blockids.h"

static inline bool transparent2(map_struct* m, int i, int j, int k, int i2, int j2, int k2)
{
	const u8 t=*getBlockP(m,i2,j2,k2);
	const u8 tt=*getBlockP(m,i,j,k);
	return ((t==0 || t>=WATERTYPE || t==12 || t==13 || isPlant(t) || isLadder(t) || isDoor(t)) && (!(t==tt || (t>=WATERTYPE && tt>=WATERTYPE))));
}

static inline bool transparent3(map_struct* m, int i, int j, int k, int i2, int j2, int k2)
{
	const u8 t=*getBlockPE(m,i2,j2,k2);
	const u8 tt=*getBlockPE(m,i,j,k);
	return ((t==0 || t>=WATERTYPE || t==12 || t==13 || isPlant(t) || isLadder(t) || isDoor(t)) && (!(t==tt || (t>=WATERTYPE && tt>=WATERTYPE))));
}

static inline bool solid(u8 type)
{
	return (type && type<WATERTYPE && type!=13 && !isPlant(type) && !isLadder(type) && !isOpenDoor(type));
}

static inline bool block(u8 type)
{
	return (type && type!=13 && !isPlant(type) && !isLadder(type) && !isDoor(type));
}

static inline bool tangible(u8 type)
{
	return (type && type<WATERTYPE);
}

// light tables are made for a torch (level 14); dimmer sources scale them
static inline u8 lightScale(u8 v, u8 level){ return level>=14?v:(u8)(v*level/14); }

static inline void getLightWC(map_struct* m, int i, int j, int k, u8* light, u8 direction, lightsource_struct* q)
{
	u8 s=((*light)&(1<<7));
	*light=0;
	const u16 d=direction<<12;
	while(q)
	{
		u8 i2=-(i%CLUSTERSIZE)+q->i+8;
		u8 j2=-(j%CLUSTERSIZE)+q->j+8;
		u8 k2=-(k%CLUSTERSIZE)+q->k+8;
		{
			*light=min((*light)+lightScale(lightTable[i2+j2*16+k2*16*16+d],q->level),127);
		}
		q=q->next;
	}
	(*light)|=s;
}

static inline void getLight(map_struct* m, int i, int j, int k, u8* light, u8 direction)
{
	vect3D clusterCoord=getCluster(m,i,j,k);
	lightsource_struct *q=m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].lightList.first;
	getLightWC(m, i, j, k, light, direction, q);
}

static inline u8 getHighest(map_struct* m, int i, int j)
{
	vect3D clusterCoord=getCluster(m,i,j,16);
	i%=CLUSTERSIZE;
	j%=CLUSTERSIZE;
	return m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->highest[i+j*CLUSTERSIZE];
}

static inline void surface(map_struct* m, int i, int j, int k, u8* light) //A OPTIMISER
{
	const vect3D clusterCoord=getCluster(m,i,j,k);
	const vect3D clusterCoord1=getCluster(m,i-1,j,k);
	const vect3D clusterCoord2=getCluster(m,i+1,j,k);
	const vect3D clusterCoord3=getCluster(m,i,j-1,k);
	const vect3D clusterCoord4=getCluster(m,i,j+1,k);
	if(k>=m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->highest[(i%CLUSTERSIZE)+(j%CLUSTERSIZE)*CLUSTERSIZE]
	|| k>m->superCluster[clusterCoord1.x-m->offset.x][clusterCoord1.y-m->offset.y]->highest[((i-1)%CLUSTERSIZE)+((j)%CLUSTERSIZE)*CLUSTERSIZE]
	|| k>m->superCluster[clusterCoord2.x-m->offset.x][clusterCoord2.y-m->offset.y]->highest[((i+1)%CLUSTERSIZE)+((j)%CLUSTERSIZE)*CLUSTERSIZE]
	|| k>m->superCluster[clusterCoord3.x-m->offset.x][clusterCoord3.y-m->offset.y]->highest[((i)%CLUSTERSIZE)+((j-1)%CLUSTERSIZE)*CLUSTERSIZE]
	|| k>m->superCluster[clusterCoord4.x-m->offset.x][clusterCoord4.y-m->offset.y]->highest[((i)%CLUSTERSIZE)+((j+1)%CLUSTERSIZE)*CLUSTERSIZE])(*light)|=(1<<7);
}

static inline bool seeThrough(u8 type)
{
	return (!type || type>=WATERTYPE || type==13 || type==12 || isLadder(type) || isOpenDoor(type) || isPlant(type) || isLeaves(type));
}

static inline void surface2(map_struct* m, int i, int j, int k, u8* light) //A OPTIMISER
{
	const vect3D clusterCoord=getCluster(m,i,j,k);
	const clusterColumn_struct *cC=m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y];
	i%=CLUSTERSIZE;
	j%=CLUSTERSIZE;
	(*light)=(1<<7);
	u8 l;
	for(l=63;l>k;l--)
	{
		if(!seeThrough(cC->data[(i)+(j)*CLUSTERSIZE+l*CLUSTERSIZE*CLUSTERSIZE])){(*light)=0;return;}
	}
}

//cache stuff
void cacheAllocateBlock(void);

static inline quad_struct* getQuad(void)
{
	if(!cacheNumber)cacheAllocateBlock();
	cacheCursor++;
	cacheNumber--;
	quad_struct* p=cache[cacheCursor];
	cache[cacheCursor]=NULL;
	return p;
}

static inline void releaseQuad(quad_struct** q)
{
	cache[cacheCursor]=*q;
	*q=NULL;
	
	if(cacheNumber<CACHESIZE)
	{
		cacheCursor--;
		cacheNumber++;
	}//test
}

void drawTestMap(map_struct* m);
void globalSaveMap(map_struct* m);
void lightSourceOn(map_struct* m, int i, int j, int k);              // the block at i,j,k now gives light
void plantRefresh(map_struct* m, int i, int j, int k, u8 type);      // a plant's id changed (stage, bent stem)
void lightSourceOff(map_struct* m, int i, int j, int k, u8 level);   // it stopped giving light
void translateSuperCluster(map_struct* m, u8 dir);
void initFilesystem(void);
void loadTestMap(map_struct* m);
void freeMap(map_struct* m);
void freeQuadCache(void);
void freeLightCache(void);
void drawTestCube(void);
void setFog(u8 mode);
void updateLightMap(void);
bool addWater(map_struct* m, u16 i, u16 j, u16 k, u8 t);
void changeBlock(map_struct* m, int i, int j, int k, u8 type);
void removeBlock(map_struct* m, int i, int j, int k, bool fix);
void addQuad(quadList_struct* ql, map_struct* m, u8 direction, u8 light, u32 bid, u8* data, u16 i, u16 j, u16 k);

#endif
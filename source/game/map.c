#include "game/game_main.h"

#define waterm(a) (((a)>=WATERTYPE)?(a):(300))

u16 cullMagic;
u8 fsFormat;


void initFilesystem(void)
{
	switch(fsMode)
	{
		case 1:
			openMap=&openMapNOCASH;
			readClusterColumn=&readClusterColumnNOCASH;
			writeClusterColumn=&writeClusterColumnNOCASH;
			fsFormat=0;
			break;
		default:
			openMap=&openMap2048;
			readClusterColumn=&readClusterColumn2048;
			writeClusterColumn=&writeClusterColumn2048;
			fsFormat=1;
			break;
	}
}

void initLightMap(void)
{
	int i;
	for(i=0;i<256*QUAD_DIRECTIONS;i++)lightMap[i]=RGB15(31,31,31);
	#ifdef FOGLIGHT
		for(i=0;i<256*QUAD_DIRECTIONS;i++)lightMap2[i]=RGB15(21,21,21);
	#endif
}

void addWaterFace(map_struct* m, int i, int j, int k, u8 f)
{
	vect3D clusterCoord=getCluster(m,i,j,k);
	quadList_struct* ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
	u8* t=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->highest[(i%CLUSTERSIZE)+(j%CLUSTERSIZE)*CLUSTERSIZE];
	if(k>(*t))(*t)=k;
	u8 light=0;
	surface(m, i, j, k, &light);
	getLight(m, i, j, k, &light, f);
	addQuad(ql, m, f, light, 0, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
}

void removeWaterFace(map_struct* m, int i, int j, int k, u8 face)
{
	vect3D clusterCoord=getCluster(m,i,j,k);
	quadList_struct* ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
	quad_struct* oq=ql->first;
	quad_struct* q;
	if(oq)q=oq->next;
	else q=NULL;
	i%=CLUSTERSIZE;
	j%=CLUSTERSIZE;
	k%=CLUSTERSIZE;
	u8 mID=i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE;
	while(q)
	{
		if(q->mID==mID && q->direction==face)
		{
			oq->next=q->next;
			releaseQuad(&q);
			ql->count--;
			q=oq->next;
			return;
		}else{
			oq=q;
			q=q->next;
		}
	}
	q=ql->first;
	if(q && q->mID==mID && q->direction==face)
	{
		ql->first=q->next;
		ql->count--;
		releaseQuad(&q);
	}
}

void initWater(void)
{
	waterCount=0;
	waterCursor=0;
	waterCount2=0;
	waterCursor2=0;
}

void addWater2(map_struct* m, water_struct w)
{
	waterSpread[waterCount++]=w;
	if(waterCount>=WATERNUMBER)waterCount-=WATERNUMBER;
}

bool addWater(map_struct* m, u16 i, u16 j, u16 k, u8 t)
{
	if(t-WATERTYPE>=WATERSPREAD)return false;
	water_struct* w;
	if(i-m->offset.x*CLUSTERSIZE >= 120
	|| i-m->offset.x*CLUSTERSIZE < 8
	|| j-m->offset.y*CLUSTERSIZE >= 120
	|| j-m->offset.y*CLUSTERSIZE < 8)w=&waterToSpread[waterCount2++];
	else w=&waterSpread[waterCount++];
	w->pos=(i&8191)|((j&8191)<<13)|((k&63)<<26);
	if(waterCount>=WATERNUMBER)waterCount-=WATERNUMBER;
	if(waterCount2>=WATERNUMBER2)waterCount2-=WATERNUMBER2;
	return true;
}

void processWater(map_struct* m)
{
	if((waterCount-waterCursor)<=0)return;
	water_struct* w=&waterSpread[waterCursor++];
	const u16 i=w->pos&8191, j=(w->pos>>13)&8191, k=(w->pos>>26)&63;
	const u8 type=*getBlockP(m,i,j,k);
	if(/*w->dir&1 && */!solid(*getBlockP(m,i,j,k-1)) && type-WATERTYPE<WATERSPREAD)
	{
		u8 d=*getBlockP(m,i,j-1,k);
		if(d==0)addWaterFace(m, i, j, k, 5);
		else if(d>=WATERTYPE)removeWaterFace(m, i, j-1, k, 4);
		d=*getBlockP(m,i,j+1,k);
		if(d==0)addWaterFace(m, i, j, k, 4);
		else if(d>=WATERTYPE)removeWaterFace(m, i, j+1, k, 5);
		d=*getBlockP(m,i-1,j,k);
		if(d==0)addWaterFace(m, i, j, k, 3);
		else if(d>=WATERTYPE)removeWaterFace(m, i-1, j, k, 2);
		d=*getBlockP(m,i+1,j,k);
		if(d==0)addWaterFace(m, i, j, k, 2);
		else if(d>=WATERTYPE)removeWaterFace(m, i+1, j, k, 3);
		d=*getBlockP(m,i,j,k-1);
		if(d>=WATERTYPE)removeWaterFace(m, i, j, k-1, 0);
		d=*getBlockP(m,i,j,k+1);
		if(d>=WATERTYPE)removeWaterFace(m, i, j, k+1, 1);
		*getBlockP(m,i,j,k-1)=type;
		addWater(m, i, j, k-1, type);
	}else if(type-WATERTYPE==WATERSPREAD-1){
		u8 d=*getBlockP(m,i,j-1,k);
		if(!d)addWaterFace(m, i, j, k, 5);
		else if(d>=WATERTYPE)removeWaterFace(m, i, j-1, k, 4);
		d=*getBlockP(m,i,j+1,k);
		if(!d)addWaterFace(m, i, j, k, 4);
		else if(d>=WATERTYPE)removeWaterFace(m, i, j+1, k, 5);
		d=*getBlockP(m,i-1,j,k);
		if(!d)addWaterFace(m, i, j, k, 3);
		else if(d>=WATERTYPE)removeWaterFace(m, i-1, j, k, 2);
		d=*getBlockP(m,i+1,j,k);
		if(!d)addWaterFace(m, i, j, k, 2);
		else if(d>=WATERTYPE)removeWaterFace(m, i+1, j, k, 3);
		d=*getBlockP(m,i,j,k+1);
		if(d>=WATERTYPE)removeWaterFace(m, i, j, k+1, 1);
	}else if(type+1-WATERTYPE<WATERSPREAD){
		u8 d=*getBlockP(m,i,j-1,k);
		if(!d)
		{
			if(*getBlockP(m,i,j-1,k-1)>=WATERTYPE)removeWaterFace(m, i, j-1, k-1, 0);
			if(!*getBlockP(m,i,j-1,k+1))addWaterFace(m, i, j-1, k, 0);
			
			*getBlockP(m,i,j-1,k)=type+1;
			addWater(m, i, j-1, k, type+1);
		}else if(d>=WATERTYPE)removeWaterFace(m, i, j-1, k, 4);
		d=*getBlockP(m,i,j+1,k);
		if(!d)
		{
			if(*getBlockP(m,i,j+1,k-1)>=WATERTYPE)removeWaterFace(m, i, j+1, k-1, 0);
			if(!*getBlockP(m,i,j+1,k+1))addWaterFace(m, i, j+1, k, 0);
			
			*getBlockP(m,i,j+1,k)=type+1;
			addWater(m, i, j+1, k, type+1);
		}else if(d>=WATERTYPE)removeWaterFace(m, i, j+1, k, 5);
		d=*getBlockP(m,i-1,j,k);
		if(!d)
		{
			if(*getBlockP(m,i-1,j,k-1)>=WATERTYPE)removeWaterFace(m, i-1, j, k-1, 0);
			if(!*getBlockP(m,i-1,j,k+1))addWaterFace(m, i-1, j, k, 0);
			
			*getBlockP(m,i-1,j,k)=type+1;
			addWater(m, i-1, j, k, type+1);
		}else if(d>=WATERTYPE)removeWaterFace(m, i-1, j, k, 2);
		d=*getBlockP(m,i+1,j,k);
		if(!d)
		{
			if(*getBlockP(m,i+1,j,k-1)>=WATERTYPE)removeWaterFace(m, i+1, j, k-1, 0);
			if(!*getBlockP(m,i+1,j,k+1))addWaterFace(m, i+1, j, k, 0);
			
			*getBlockP(m,i+1,j,k)=type+1;
			addWater(m, i+1, j, k, type+1);
		}else if(d>=WATERTYPE)removeWaterFace(m, i+1, j, k, 3);
		d=*getBlockP(m,i,j,k+1);
		if(d>=WATERTYPE)removeWaterFace(m, i, j, k+1, 1);
	}
	if(waterCursor>=WATERNUMBER)waterCursor-=WATERNUMBER;
}

void freeQuadCache(void)
{
	int i;
	for(i=0;i<cacheRecord;i++)
	{
		free(((quad_struct**)VRAM_F)[i]);
	}
}

void freeLightCache(void)
{
	int i;
	for(i=0;i<lightCacheRecord;i++)
	{
		free(((lightsource_struct**)VRAM_G)[i]);
	}
}

void initQuadCache(void)
{
	int i;
	for(i=0;i<CACHESIZE;i++)
	{
		cache[i]=NULL;
	}
	vramSetBankF(VRAM_F_LCD);
	cacheRecord=0;
	cacheNumber=0;
	cacheCursor=CACHESIZE-1;
}

void initLightCache(void)
{
	int i;
	for(i=0;i<LIGHTCACHESIZE;i++)
	{
		lightCache[i]=NULL;
	}
	vramSetBankG(VRAM_G_LCD);
	lightCacheRecord=0;
	lightCacheNumber=0;
	lightCacheCursor=LIGHTCACHESIZE-1;
}

void cacheAllocateBlock(void) //ONLY IF EMPTY
{
	quad_struct* p=malloc(sizeof(quad_struct)*CACHEBLOCK);
	((quad_struct**)VRAM_F)[cacheRecord++]=p;
	u8 i;
	for(i=0;i<CACHEBLOCK;i++)
	{
		cache[CACHESIZE-1-i]=&p[i];
	}
	cacheCursor=CACHESIZE-1-i;
	cacheNumber+=CACHEBLOCK;
}

void lightCacheAllocateBlock(void) //ONLY IF EMPTY
{
	lightsource_struct* p=malloc(sizeof(lightsource_struct)*LIGHTCACHEBLOCK);
	((lightsource_struct**)VRAM_G)[lightCacheRecord++]=p;
	u16 i;
	for(i=0;i<LIGHTCACHEBLOCK;i++)
	{
		lightCache[LIGHTCACHESIZE-1-i]=&p[i];
	}
	lightCacheCursor=LIGHTCACHESIZE-1-i;
	lightCacheNumber+=LIGHTCACHEBLOCK;
}

lightsource_struct* getLightSource(void)
{
	if(!lightCacheNumber)lightCacheAllocateBlock();
	lightCacheCursor++;
	lightCacheNumber--;
	lightsource_struct* p=lightCache[lightCacheCursor];
	lightCache[lightCacheCursor]=NULL;
	p->next=NULL;
	return p;
}

void releaseLight(lightsource_struct** q)
{
	lightCache[lightCacheCursor]=*q;
	*q=NULL;
	lightCacheCursor--;
	lightCacheNumber++;
	return;
}

void initmIDTables(void)
{
	u8 i, j, k;
	for(i=0;i<CLUSTERSIZE;i++)
	{
		for(j=0;j<CLUSTERSIZE;j++)
		{
			for(k=0;k<CLUSTERSIZE;k++)
			{
				u8 mID=i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE;
				imIDtable[mID]=i;
				jmIDtable[mID]=j;
				kmIDtable[mID]=k;
				ijkmIDtable2[mID]=(i)+(j<<4)+(k<<8);
			}	
		}	
	}
}

void initLightTable(void) //ADD DIRECTION
{
	int i, j, k, d;
	vect3D normal;
	for(d=0;d<QUAD_DIRECTIONS;d++)
	{
		switch(d)
		{
			case 0:
				normal=(vect3D){0,0,inttof32(-1)};
				break;
			case 1:
				normal=(vect3D){0,0,inttof32(1)};
				break;
			case 5:
				normal=(vect3D){0,inttof32(1),0};
				break;
			case 4:
				normal=(vect3D){0,inttof32(-1),0};
				break;
			case 3:
				normal=(vect3D){inttof32(1),0,0};
				break;
			case 2:	
				normal=(vect3D){inttof32(-1),0,0};
				break;
			case 12:
				normal=(vect3D){0,inttof32(1),0};
				break;
			case 13:
				normal=(vect3D){0,inttof32(-1),0};
				break;
			case 10:
				normal=(vect3D){inttof32(1),0,0};
				break;
			case 11:	
				normal=(vect3D){inttof32(-1),0,0};
				break;
			// crossed planes (6, 7: one diagonal, 8, 9: the other) and the
			// middle planes: lit from their own side (like the faces above,
			// this is the opposite of the side the quad faces)
			case 6:
				normal=(vect3D){2896,2896,0};
				break;
			case 7:
				normal=(vect3D){-2896,-2896,0};
				break;
			case 8:
				normal=(vect3D){2896,-2896,0};
				break;
			case 9:
				normal=(vect3D){-2896,2896,0};
				break;
			case 14:
				normal=(vect3D){inttof32(1),0,0};
				break;
			case 15:
				normal=(vect3D){inttof32(-1),0,0};
				break;
			case 16:
				normal=(vect3D){0,inttof32(-1),0};
				break;
			case 17:
				normal=(vect3D){0,inttof32(1),0};
				break;
		}
		for(i=-8;i<=7;i++)
		{
			for(j=-8;j<=7;j++)
			{
				for(k=-8;k<=7;k++)
				{
					int32 length=sqrtf32(inttof32(i*i+j*j+k*k));
					vect3D vec=(vect3D){-divf32(inttof32(i),(length)),-divf32(inttof32(j),(length)),-divf32(inttof32(k),(length))};
					int32 ps=max(mulf32(normal.x,vec.x)+mulf32(normal.y,vec.y)+mulf32(normal.z,vec.z),0);
					lightTable[((i+8)+(j+8)*16+(k+8)*16*16)+(d<<12)]=f32toint(ps*(max(31-((i)*(i)+(j)*(j)+(k)*(k)),0)));
					#ifdef FOGLIGHT
						if(!lightTable[((i+8)+(j+8)*16+(k+8)*16*16)+(d<<12)])lightTable[((i+8)+(j+8)*16+(k+8)*16*16)+(d<<12)]=1;
					#endif
				}	
			}	
		}
	}
	for(i=0;i<256;i++)
	{
		for(j=0;j<32;j++)
		{
			lightComputeTable[i+(j<<8)]=min((i&127)+j,127)|(i&(1<<7));
		}
	}
	lightProcess.first=NULL;
	lightProcess.count=0;
}

vect3D waterAnimV;

void initUVmap(void)
{
	int i;
	for(i=0;i<256;i++)
	{
		u8 u=(i%16)*16;
		u8 v=((i-(i%16))/16)*16;
		uvMap[i*4+0]=TEXTURE_PACK(16*(0+u)+1, 16*(0+v)+1);
		uvMap[i*4+1]=TEXTURE_PACK(16*(16+u)-1, 16*(0+v)+1);
		uvMap[i*4+2]=TEXTURE_PACK(16*(16+u)-1, 16*(16+v)-1);
		uvMap[i*4+3]=TEXTURE_PACK(16*(0+u)+1, 16*(16+v)-1);
	}
	waterAnim.x=16<<5;
	waterAnim.y=12<<5;
	waterAnimV.x=30;
	waterAnimV.y=-50;
}

s16 waterFall;

void updateUVwater(void)
{
	uvMapWater[0]=TEXTURE_PACK(16*(0+(waterAnim.x>>9))+1, 16*(0+(waterAnim.y>>9))+1);
	uvMapWater[1]=TEXTURE_PACK(16*(16+(waterAnim.x>>9))-1, 16*(0+(waterAnim.y>>9))+1);
	uvMapWater[2]=TEXTURE_PACK(16*(16+(waterAnim.x>>9))-1, 16*(16+(waterAnim.y>>9))-1);
	uvMapWater[3]=TEXTURE_PACK(16*(0+(waterAnim.x>>9))+1, 16*(16+(waterAnim.y>>9))-1);
	uvMapWater[0+4]=uvMapWater[0];
	uvMapWater[1+4]=uvMapWater[1];
	uvMapWater[2+4]=uvMapWater[2];
	uvMapWater[3+4]=uvMapWater[3];
	int i;
	for(i=2;i<6;i++)
	{
		uvMapWater[0+4*(i)]=TEXTURE_PACK(16*(0)+1, 16*(0+(waterFall>>2))+1);
		uvMapWater[1+4*(i)]=TEXTURE_PACK(16*(16)-1, 16*(0+(waterFall>>2))+1);
		uvMapWater[2+4*(i)]=TEXTURE_PACK(16*(16)-1, 16*(16+(waterFall>>2))-1);
		uvMapWater[3+4*(i)]=TEXTURE_PACK(16*(0)+1, 16*(16+(waterFall>>2))-1);
	}
	waterAnimV.x+=-waterAnim.x/128;
	waterAnimV.y+=-waterAnim.y/128;
	waterAnim.x+=waterAnimV.x;
	waterAnim.y+=waterAnimV.y;
	waterFall--;
}

void initXYmap(void)
{
	int i, j, k, d;
	for(d=0;d<QUAD_DIRECTIONS;d++)
	{
		for(i=0;i<CLUSTERSIZE;i++)
		{
			for(j=0;j<CLUSTERSIZE;j++)
			{
				for(k=0;k<CLUSTERSIZE;k++)
				{
					switch(d)
					{
						case 0:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							break;
						case 1:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 2:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 3:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 4:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 5:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;	
						case 6:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;	
						case 7:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							break;
						case 8:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;	
						case 9:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							break;	
						case 10:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 11:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 12:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 13:
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((-tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize+tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;	
						case 14: // plane through the middle along j, u towards +j
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 15: // the same plane seen from the other side, u towards -j
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize2*i),(tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize2*i),(-tilesize+tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize2*i),(-tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize2*i),(tilesize+tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 16: // plane through the middle along i, u towards +i
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize2*j),(-tilesize+tilesize2*k));
							break;
						case 17: // the same plane seen from the other side, u towards -i
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+0] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+1] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize2*j),(tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+2] = NORMAL_PACK((-tilesize+tilesize2*i),(tilesize2*j),(-tilesize+tilesize2*k));
							xyMap[i*4+j*4*CLUSTERSIZE+k*4*CLUSTERSIZE*CLUSTERSIZE+d*CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*4+3] = NORMAL_PACK((tilesize+tilesize2*i),(tilesize2*j),(-tilesize+tilesize2*k));
							break;
					}
				}		
			}		
		}
	}
}

void initDegradTable(void)
{
	int k, l, n;
	s16 a,b,c,d;
	for(k=0;k<8;k++)
	{
		for(l=0;l<8;l++)
		{
			if((k || l) && (k<7 || l<7) && !(!k && l==7) && !(!l && k==7))
			{
				n=k*5+8*5*l;
				a=divf32(inttof32(1),(sqrtf32(inttof32(k*k+l*l))));
				b=divf32(inttof32(1),(sqrtf32(inttof32((8-k)*(8-k)+l*l))));
				c=divf32(inttof32(1),(sqrtf32(inttof32((8-k)*(8-k)+(8-l)*(8-l)))));
				d=divf32(inttof32(1),(sqrtf32(inttof32(k*k+(8-l)*(8-l)))));
				degradTable[n+4]=(divf32(inttof32(1),(a+b+c+d)))>>6;
				degradTable[n+0]=((a)>>4)-1;
				degradTable[n+1]=((b)>>4)-1;
				degradTable[n+2]=((c)>>4)-1;
				degradTable[n+3]=((d)>>4)-1;
				if(k<3 && l<3)NOGBA("%d, %d, %d, %d : %d (%d, %d, %d, %d)",degradTable[n+0],degradTable[n+1],degradTable[n+2],degradTable[n+3],degradTable[n+4], a, b, c, d);
			}
		}
	}
	n=0*5+8*5*0;
	degradTable[n+0]=255;
	degradTable[n+1]=0;
	degradTable[n+2]=0;
	degradTable[n+3]=0;
	degradTable[n+4]=1<<6;
	n=7*5+8*5*0;
	degradTable[n+0]=0;
	degradTable[n+1]=255;
	degradTable[n+2]=0;
	degradTable[n+3]=0;
	degradTable[n+4]=1<<6;
	n=7*5+8*5*7;
	degradTable[n+0]=0;
	degradTable[n+1]=0;
	degradTable[n+2]=255;
	degradTable[n+3]=0;
	degradTable[n+4]=1<<6;
	n=0*5+8*5*7;
	degradTable[n+0]=0;
	degradTable[n+1]=0;
	degradTable[n+2]=0;
	degradTable[n+3]=255;
	degradTable[n+4]=1<<6;
}

void addLightProcess(u16 i, u16 j, u16 k)
{
	toProcess_struct* q=malloc(sizeof(toProcess_struct));
	q->next=lightProcess.first;
	q->i=i;
	q->j=j;
	q->k=k;
	lightProcess.first=q;
	lightProcess.count++;
}

// tiles the block atlas holds: 128 in the game, 64 in the menu (whose own
// textures leave no room for more; its scene only uses the terrain tiles)
static int atlasTiles;

void addTileTexture(u8 id, u16* buffer)
{
	u8 i, j;
	if(!blockSuperTexture || !blockSuperTexture->addr || id>=atlasTiles)return;
	u8 u=(id%16)*16;
	u8 v=((id-(id%16))/16)*16;
	NOGBA("TILE ! %d : %d,%d",id,u,v);
	u32 vramTemp = vramSetPrimaryBanks(VRAM_A_LCD,VRAM_B_LCD,VRAM_C_LCD,VRAM_D_LCD);
		for(i=0;i<16;i++)
		{
			for(j=0;j<16;j++)
			{
				((u16*)blockSuperTexture->addr)[(u+i)+(v+j)*256]=buffer[i+j*16];
			}		
		}
	vramRestorePrimaryBanks(vramTemp);
}

MTL_img* processTile(u16* buffer, u8 x, u8 y, u8 id)
{
	int i, j;
	int i16, j16;
	u16 texture[16*16];
	for(i16=0;i16<16;i16++)
	{
		for(j16=0;j16<16;j16++)
		{
			i=x*16+i16;
			j=y*16+j16;
			texture[i16+j16*16]=buffer[i+j*256];
		}	
	}
	addTileTexture(id, texture);
	return NULL;
}

MTL_img* processTileFilter(u8* buffer, u8 x, u8 y, u8 r, u8 g, u8 b, u8 id)
{
	int i, j;
	int i16, j16;
	u16 texture[16*16];
	for(i16=0;i16<16;i16++)
	{
		for(j16=0;j16<16;j16++)
		{
			i=x*16+i16;
			j=y*16+j16;
			texture[i16+j16*16]=RGB15((buffer[i*4+j*256*4]*r)>>11,
									(buffer[i*4+j*256*4+1]*g)>>11,
									(buffer[i*4+j*256*4+2]*b)>>11)|((buffer[i*4+j*256*4+3]!=0)<<15);
		}	
	}
	addTileTexture(id, texture);
	return NULL;
}

MTL_img* processTileFilterMask(u8* buffer, u16* buff, u8 x, u8 y, u8 x2, u8 y2, u8 r, u8 g, u8 b, u8 id)
{
	int i, j;
	int i2, j2;
	int i16, j16;
	u16 texture[16*16];
	for(i16=0;i16<16;i16++)
	{
		for(j16=0;j16<16;j16++)
		{
			i=x*16+i16;
			j=y*16+j16;
			i2=x2*16+i16;
			j2=y2*16+j16;
			if(!buffer[i2*4+j2*256*4+3])texture[i16+j16*16]=buff[i+j*256];
			else texture[i16+j16*16]=RGB15((buffer[i2*4+j2*256*4]*r)>>11,
									(buffer[i2*4+j2*256*4+1]*g)>>11,
									(buffer[i2*4+j2*256*4+2]*b)>>11)|(1<<15);
		}	
	}
	addTileTexture(id, texture);
	return NULL;
}

u16 getPixel(int i, int j, int x, int y, u16* buff, u8 lum)
{
	int r=buff[(x+i*16)+(y+j*16)*256]&31;
	int g=(buff[(x+i*16)+(y+j*16)*256]>>5)&31;
	int b=(buff[(x+i*16)+(y+j*16)*256]>>10)&31;
	bool a=(buff[(x+i*16)+(y+j*16)*256]>>15)&1;
	return RGB15(r*lum/32,g*lum/32,b*lum/32)|(a<<15);
}

void getIcon(u16* buff, int id, int i1, int j1)
{
	int x, y;
	int offset=256*288/2;
	for(x=0;x<16;x++)
	{
		for(y=0;y<16;y++)
		{
			u16* dest=&SPRITE_GFX_SUB[offset+x+(id%8)*16+(y+((id-(id%8))/8)*16)*128];
			*dest = getPixel(i1,j1,x,y,buff,32);
		}
	}
}

void makeIcon(u16* buff, int id, int i1, int j1, int i2, int j2)
{
	int x, y;
	int offset=256*288/2;
	for(x=0;x<16;x++)
	{
		for(y=0;y<16;y++)
		{
			int dx=x, dy=y;
			int dx2=x, dy2=y;
			while(dy>0){dx-=2;dy--;}
			while(dx2>0){dx2-=2;dy2++;}
			dx+=8;
			dy2-=4;
			dy2--;dy2*=2;
			u16* dest=&SPRITE_GFX_SUB[offset+x+(id%8)*16+(y+((id-(id%8))/8)*16)*128];
			if(y<4){if(abs(8-x)<y*2){*dest = getPixel(i1,j1,dx,dy2,buff,32);}else *dest = RGB15(0,0,0);}
			else if(y<4*2 && abs(8-x)<16-y*2){*dest = getPixel(i1,j1,dx,dy2,buff,32);}
			else if(x>8 && abs(8-x)<2*(15-y) && y<15)*dest = getPixel(i2,j2,(x-9)*2,(y-5-(16-x)/2)*2,buff,22);
			else if(x>0 && abs(8-x)<2*(15-y) && y<15)*dest = getPixel(i2,j2,(x-1)*2,(y-5-(x-1)/2)*2,buff,32);
			else *dest = RGB15(0,0,0);
		}
	}
}

void loadBlockTextures(bool spr, bool tex)
{
	int i, j;
	if(tex)
	{
		waterTexture=Game_CreateTextureAlpha("12.pcx", "textures", 27);
		cursorTexture=Game_CreateTexture("256.pcx", "textures");
		crossHair=Game_CreateTexture("crosshair.pcx", "textures");
	}
	
	char path[255];
	getcwd(path,255);
	chdir(packPath);
	
	if(tex)
	{
		atlasTiles=spr?128:64;
		blockSuperTexture=Game_CreateTextureBuffer16(NULL, 256, atlasTiles/16*16, false);
		if(!blockSuperTexture->addr)NOGBA("no room in VRAM for the block atlas");
	}

	unsigned char* buffer;
	unsigned char* image;
	size_t buffersize, imagesize;
	LodePNG_Decoder decoder;
	
	char path2[255];
	getcwd(path2,255);
	chdir("misc");
	LodePNG_loadFile(&buffer, &buffersize, "grasscolor.png");
	LodePNG_Decoder_init(&decoder);
	LodePNG_Decoder_decode(&decoder, &image, &imagesize, buffer, buffersize);
	
	u8 biomeX=120, biomeY=120;
	
	u8 grassR=image[biomeX*4+biomeY*4*decoder.infoPng.width];
	u8 grassG=image[biomeX*4+biomeY*4*decoder.infoPng.width+1];
	u8 grassB=image[biomeX*4+biomeY*4*decoder.infoPng.width+2];
	
	free(image);
	free(buffer);
	LodePNG_Decoder_cleanup(&decoder);
	
	LodePNG_loadFile(&buffer, &buffersize, "foliagecolor.png");
	LodePNG_Decoder_init(&decoder);
	LodePNG_Decoder_decode(&decoder, &image, &imagesize, buffer, buffersize);
	
	u8 foliageR=image[biomeX*4+biomeY*4*decoder.infoPng.width];
	u8 foliageG=image[biomeX*4+biomeY*4*decoder.infoPng.width+1];
	u8 foliageB=image[biomeX*4+biomeY*4*decoder.infoPng.width+2];
	
	free(image);
	free(buffer);
	LodePNG_Decoder_cleanup(&decoder);
	
	chdir(path2);

	LodePNG_loadFile(&buffer, &buffersize, "terrain.png");
	LodePNG_Decoder_init(&decoder);
	LodePNG_Decoder_decode(&decoder, &image, &imagesize, buffer, buffersize);
	if(decoder.error) NOGBA("error %u: %s\n", decoder.error, LodePNG_error_text(decoder.error));
	else
	{
		u16* buff=malloc(decoder.infoPng.width*decoder.infoPng.height*sizeof(u16));
		for(i=0;i<decoder.infoPng.width;i++)
		{
			for(j=0;j<decoder.infoPng.height;j++)
			{
				buff[i+j*decoder.infoPng.width]=RGB15(image[i*4+j*4*decoder.infoPng.width]>>3,
													  image[i*4+j*4*decoder.infoPng.width+1]>>3,
													  image[i*4+j*4*decoder.infoPng.width+2]>>3)|((image[i*4+j*4*decoder.infoPng.width+3]!=0)<<15);
			}
		}
		
		for(i=0;i<BLOCKTEXTURES;i++)
		{
			if(i==55 || i==CHARCOAL_TILE || i==APPLE_TILE)continue;     // items.png tiles
			switch(i)
			{
				case 1:
					processTileFilterMask(image, buff, blockTextures[i].i, blockTextures[i].j, 6, 2, grassR,grassG,grassB,i);
					break;
				case 2:
					processTileFilter(image,blockTextures[i].i,blockTextures[i].j,grassR,grassG,grassB,i);
					break;
				case 10:
					processTileFilter(image,blockTextures[i].i,blockTextures[i].j,foliageR,foliageG,foliageB,i);
					break;
				default:
					processTile(buff,blockTextures[i].i,blockTextures[i].j,i);
					break;
			}
		}
		NOGBA("supertex done");
		processTile(buff,1,2,IRON_ORE_TILE);
		// carrots, stems and seeds: the Beta packs have no pictures of them
		{
			u16 art[16*16];
			int t, x, y;
			for(t=FARM_ART_FIRST;t<=FARM_ART_LAST;t++){farmArtTile(t,art);addTileTexture(t,art);}
			if(spr)
			{
				farmArtTile(CARROT_ITEM_TILE,art);
				for(y=0;y<16;y++)for(x=0;x<16;x++)*survivalIconPixel(ITEM_CARROT,x,y)=art[x+y*16];
				farmArtTile(SEEDS_TILE,art);
				for(y=0;y<16;y++)for(x=0;x<16;x++)*survivalIconPixel(ITEM_PUMPKIN_SEEDS,x,y)=art[x+y*16];
			}
		}
		if(spr)
		{
			for(i=0;i<BLOCKS;i++)
			{
				switch(i)
				{
					case 13:
						getIcon(buff, i, blockTextures[i].i, blockTextures[i].j);
						break;
					case LADDERTYPE:
						getIcon(buff, i, blockTextures[i].i, blockTextures[i].j);
						break;
					default:
						makeIcon(buff, i, blockTextures[blocks[i].top].i, blockTextures[blocks[i].top].j, blockTextures[blocks[i].side].i, blockTextures[blocks[i].side].j);
						break;
				}
			}
			makeIcon(buff, ITEM_CRAFTING_TABLE, 11, 2, 11, 3);
			makeIcon(buff, ITEM_COAL_ORE, 2, 2, 2, 2);
			makeIcon(buff, ITEM_IRON_ORE, 1, 2, 1, 2);
			makeIcon(buff, ITEM_CHEST, 9, 1, 11, 1);
			makeIcon(buff, ITEM_FURNACE, 14, 3, 12, 2);
			getIcon(buff, ITEM_SAPLING, 15, 0);
			makeIcon(buff, ITEM_PUMPKIN, 6, 6, 7, 7);
		}
		free(buff);
		NOGBA("supertex done2");
	}

	/*cleanup decoder*/
	free(image);
	free(buffer);
	LodePNG_Decoder_cleanup(&decoder);
	
	getcwd(path2,255);
	chdir("gui");
	LodePNG_loadFile(&buffer, &buffersize, "items.png");
	LodePNG_Decoder_init(&decoder);
	LodePNG_Decoder_decode(&decoder, &image, &imagesize, buffer, buffersize);
	packHasItems=!decoder.error && image && decoder.infoPng.width==256;
	if(packHasItems)
	{
		u16* buff=malloc(decoder.infoPng.width*decoder.infoPng.height*sizeof(u16));
		for(i=0;i<decoder.infoPng.width;i++)
		{
			for(j=0;j<decoder.infoPng.height;j++)
			{
				buff[i+j*decoder.infoPng.width]=RGB15(image[i*4+j*4*decoder.infoPng.width]>>3,
													  image[i*4+j*4*decoder.infoPng.width+1]>>3,
													  image[i*4+j*4*decoder.infoPng.width+2]>>3)|((image[i*4+j*4*decoder.infoPng.width+3]!=0)<<15);
			}
		}
		getIcon(buff, 11, 11, 4);
		processTile(buff,11,4,11);
		getIcon(buff,DOORTYPE,11,2);
		processTile(buff,11,2,DOORTYPE);
		// tools (shovels row 5, pickaxes row 6, axes row 7; wood, stone) and the stick
		for(i=0;i<SURVIVAL_TOOLS;i++)
		{
			u8 tool=ITEM_TOOL_FIRST+i, x=i/3, y=(i%3==0)?6:((i%3==1)?5:7);
			if(spr)getIcon(buff,tool,x,y);
			processTile(buff,x,y,blocks[tool].top);
		}
		if(spr)getIcon(buff,ITEM_STICK,5,3);
		processTile(buff,5,3,blocks[ITEM_STICK].top);
		if(spr)getIcon(buff,ITEM_COAL,7,0);
		processTile(buff,7,0,blocks[ITEM_COAL].top);
		// charcoal: the coal picture, tinted (Beta has no picture of its own)
		{
			u16 keep[16*16];
			for(j=0;j<16;j++)for(i=0;i<16;i++){keep[i+j*16]=buff[7*16+i+j*256];buff[7*16+i+j*256]=charcoalTint(keep[i+j*16]);}
			if(spr)getIcon(buff,ITEM_CHARCOAL,7,0);
			processTile(buff,7,0,CHARCOAL_TILE);
			for(j=0;j<16;j++)for(i=0;i<16;i++)buff[7*16+i+j*256]=keep[i+j*16];
		}
		if(spr)getIcon(buff,ITEM_APPLE,10,0);
		processTile(buff,10,0,APPLE_TILE);
		if(spr)getIcon(buff,ITEM_WOOD_HOE,0,8);
		processTile(buff,0,8,WOOD_HOE_TILE);
		if(spr)getIcon(buff,ITEM_STONE_HOE,1,8);
		processTile(buff,1,8,STONE_HOE_TILE);
		// iron tools (column 2: shovel row 5, pickaxe 6, axe 7, hoe 8) and the ingot
		for(i=0;i<4;i++)
		{
			static const u8 id[4]={ITEM_IRON_PICKAXE,ITEM_IRON_SHOVEL,ITEM_IRON_AXE,ITEM_IRON_HOE}, row[4]={6,5,7,8};
			if(spr)getIcon(buff,id[i],2,row[i]);
			processTile(buff,2,row[i],IRON_PICKAXE_TILE+i);
		}
		if(spr)getIcon(buff,ITEM_IRON_INGOT,7,1);
		processTile(buff,7,1,IRON_INGOT_TILE);
		// iron armour: column 2, helmet to boots (their ids are past the icons' room: icons 52..55)
		for(i=0;i<4;i++)
		{
			if(spr)getIcon(buff,ARMOR_ICON_SLOT+i,2,i);
			processTile(buff,2,i,ARMOR_TILE+i);
		}
		free(buff);
	}

	free(image);
	free(buffer);
	LodePNG_Decoder_cleanup(&decoder);

	// the furnace screen's flame and arrow
	image=NULL; buffer=NULL;
	LodePNG_loadFile(&buffer, &buffersize, "furnace.png");
	LodePNG_Decoder_init(&decoder);
	if(buffer)LodePNG_Decoder_decode(&decoder, &image, &imagesize, buffer, buffersize);
	if(buffer && !decoder.error && image)inventoryLoadFurnaceGui(image,decoder.infoPng.width,decoder.infoPng.height);
	else inventoryLoadFurnaceGui(NULL,0,0);
	free(image);
	free(buffer);
	LodePNG_Decoder_cleanup(&decoder);

	// the armour bar's pictures
	image=NULL; buffer=NULL;
	LodePNG_loadFile(&buffer, &buffersize, "icons.png");
	LodePNG_Decoder_init(&decoder);
	if(buffer)LodePNG_Decoder_decode(&decoder, &image, &imagesize, buffer, buffersize);
	if(buffer && !decoder.error && image)survivalLoadArmorIcons(image,decoder.infoPng.width,decoder.infoPng.height);
	else survivalLoadArmorIcons(NULL,0,0);
	free(image);
	free(buffer);
	LodePNG_Decoder_cleanup(&decoder);
	NOGBA("supertex done3");
	chdir(path);
}

void cleanList(list_struct* l)
{
	l->size=0;
}

static inline void addListElement(list_struct* l, u16 i, u16 j, u16 k, u8 direction)
{
	l->elements[l->size].i=i;l->elements[l->size].j=j;l->elements[l->size].k=k;l->elements[l->size].direction=direction;
	l->size++;
}

int olcursor;

void cullClusters(map_struct* m, list_struct* ol, list_struct* cl, int sI, int sJ, int sK)
{
	int i, bid, bid2, count;
	int cx=SUPERCLUSTERSIZE, cxy=SUPERCLUSTERSIZE*SUPERCLUSTERSIZE;
	cleanList(cl);
	if(!testBuffer)
	{
		cullMagic++;
		cleanList(ol);
		addListElement(cl, sI, sJ, sK, 0);
		bid=qgetCluster(m,sI,sJ,sK);m->clusterDraw[bid]=cullMagic;
		if(sI<SUPERCLUSTERSIZE-1){addListElement(ol, sI+1, sJ, sK, dir_x);m->clusterDraw[bid+1]=cullMagic;}
		if(sI>0){addListElement(ol, sI-1, sJ, sK, dir_x);m->clusterDraw[bid-1]=cullMagic;}
		if(sJ<SUPERCLUSTERSIZE-1){addListElement(ol, sI, sJ+1, sK, dir_y);m->clusterDraw[bid+cx]=cullMagic;}
		if(sJ>0){addListElement(ol, sI, sJ-1, sK, dir_y);m->clusterDraw[bid-cx]=cullMagic;}
		if(sK<m->clusterSize.z-1){addListElement(ol, sI, sJ, sK+1, dir_z);m->clusterDraw[bid+cxy]=cullMagic;}
		if(sK>0){addListElement(ol, sI, sJ, sK-1, dir_z);m->clusterDraw[bid-cxy]=cullMagic;}
		olcursor=0;
	}
	
	count=0;
		i=olcursor;
		listElement_struct *le=&ol->elements[i];
		listElement_struct *le2=&ol->elements[i+1];
		if(i<ol->size)BoxTest_Asynch((le->i)*bsize-(tilesize<<6),(le->j)*bsize-(tilesize<<6),(le->k)*bsize-(tilesize<<6),bsize,bsize,bsize);
		int r, n=0;
		u8 o=0;
		for(i=olcursor;i+1<ol->size && n<400 && count<1700;i++) //test
		{
			bid=qgetCluster(m,le->i,le->j,le->k);
			if(m->clusterDrawn[bid]<cullMagic-1)r=BoxTestResult();
			else {r=3;}
			if(i+1<ol->size)
			{
				le2=&ol->elements[i+1];
				bid2=qgetCluster(m,le2->i,le2->j,le2->k);
				if(m->clusterDrawn[bid2]<cullMagic-1)BoxTest_Asynch((le2->i)*bsize-(tilesize<<6),(le2->j)*bsize-(tilesize<<6),(le2->k)*bsize-(tilesize<<6),bsize,bsize,bsize);
			}
			if(r || i<6) //test
			{
				if((m->superCluster[le->i][le->j]->cluster[le->k].quadList.count || m->superCluster[le->i][le->j]->cluster[le->k].specialList.count) && r)
				{
					addListElement(cl, le->i, le->j, le->k, 0);
				}
				if(r<3){m->clusterDrawn[bid]=cullMagic+o%3;o++;}
				n++;
				m->clusterDraw[bid]=cullMagic;
				count+=m->superCluster[le->i][le->j]->cluster[le->k].quadList.count+m->superCluster[le->i][le->j]->cluster[le->k].specialList.count;
				bool d1=m->superCluster[le->i][le->j]->cluster[le->k].wall&1, d2=(m->superCluster[le->i][le->j]->cluster[le->k].wall>>1)&1, d3=(m->superCluster[le->i][le->j]->cluster[le->k].wall>>2)&1;
				
				u8 newdir=dir_x|le->direction;		
				if(!(d3 && le->direction&dir_x))
				{
					if(le->i<SUPERCLUSTERSIZE-1 && m->clusterDraw[bid+1]!=cullMagic){addListElement(ol, le->i+1, le->j, le->k, newdir);m->clusterDraw[bid+1]=cullMagic;}
					if(le->i>0 && m->clusterDraw[bid-1]!=cullMagic){addListElement(ol, le->i-1, le->j, le->k, newdir);m->clusterDraw[bid-1]=cullMagic;}
				}
				if(!(d2 && le->direction&dir_y))
				{
					newdir=dir_y|le->direction;
					if(le->j<SUPERCLUSTERSIZE-1 && m->clusterDraw[bid+cx]!=cullMagic){addListElement(ol, le->i, le->j+1, le->k, newdir);m->clusterDraw[bid+cx]=cullMagic;}
					if(le->j>0 && m->clusterDraw[bid-cx]!=cullMagic){addListElement(ol, le->i, le->j-1, le->k, newdir);m->clusterDraw[bid-cx]=cullMagic;}
				}
				if(!(d1 && le->direction&dir_z))
				{
					newdir=dir_z|le->direction;
					if(le->k<m->clusterSize.z-1 && m->clusterDraw[bid+cxy]!=cullMagic){addListElement(ol, le->i, le->j, le->k+1, newdir);m->clusterDraw[bid+cxy]=cullMagic;}
					if(le->k>0 && m->clusterDraw[bid-cxy]!=cullMagic){addListElement(ol, le->i, le->j, le->k-1, newdir);m->clusterDraw[bid-cxy]=cullMagic;}
				}
			}
			le=le2;
		}
	olcursor=i;
	#ifdef DEBUGMODE
	iprintf("\nculled %d clust (%d %d %d)",cl->size,i,ol->size,count);
	#endif
}

void addQuad(quadList_struct* ql, map_struct* m, u8 direction, u8 light, u32 bid, u8* data, u16 i, u16 j, u16 k)
{
	quad_struct* q=getQuad();
	ql->count++;
	
	i%=CLUSTERSIZE;
	j%=CLUSTERSIZE;
	
	q->light=light;
	q->direction=direction;q->next=NULL;
	q->type=data[i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE];
	if(q->type>=WATERTYPE)
	{
		q->type=direction;
	}else if(isFarmland(q->type) || isPumpkin(q->type) || isCarrotCrop(q->type) || isStem(q->type))
	{
		q->type=farmTexture(q->type,direction);
	}else if(isFurnaceBlock(q->type))
	{
		q->type=furnaceTexture(q->type,direction);
	}else if(isChestBlock(q->type))
	{
		q->type=chestTexture(q->type,direction);
	}else{
		switch(q->type)
		{
			case 1:
				if(!q->direction)q->type=2;
				else if(q->direction==1 || (k<m->size.z && data[i+j*CLUSTERSIZE+(k+1)*CLUSTERSIZE*CLUSTERSIZE]))q->type=0;
				else q->type=1;
				break;
			default :
				if(!q->direction)q->type=blocks[q->type].top;
				else if(q->direction==1)q->type=blocks[q->type].bottom;
				else q->type=blocks[q->type].side;
				break;
		}
	}
	k%=CLUSTERSIZE;
	q->mID=i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE;
	if(ql->first)q->next=ql->first;
	ql->first=q;
}

void addLight(lightsourceList_struct* ql, map_struct* m, s8 i, s8 j, s8 k, u8 level)
{
	lightsource_struct* q=getLightSource();
	q->level=level;
	ql->count=(((ql->count&127)+1)&127)|(ql->count&128);
	q->i=i;
	q->j=j;
	q->k=k;
	q->next=ql->first;
	ql->first=q;
}

void adjustBlockLight(map_struct* m, int i, int j, int k)
{
	
	vect3D clusterCoord=getCluster(m,i,j,k);
	quadList_struct* ql;
	u8 type=*getBlockP(m,i,j,k);
	if(type>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
	else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
	quad_struct* q=ql->first;
	bool ls=(k==getHighest(m, i, j)
			|| k>getHighest(m, i+1, j)
			|| k>getHighest(m, i-1, j)
			|| k>getHighest(m, i, j+1)
			|| k>getHighest(m, i, j-1));
	NOGBA("ls : %d (%d %d %d %d %d)",ls,getHighest(m, i, j),getHighest(m, i+1, j),getHighest(m, i-1, j),getHighest(m, i, j+1),getHighest(m, i, j-1));
	i%=CLUSTERSIZE;
	j%=CLUSTERSIZE;
	k%=CLUSTERSIZE;
	u8 mID=i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE;
	while(q)
	{
		if(q->mID==mID)
		{
			q->light=(q->light&127)|((ls&1)<<7);
		}
		q=q->next;
	}
}

void removeBlock(map_struct* m, int i, int j, int k, bool fix)
{
	
	vect3D clusterCoord=getCluster(m,i,j,k);
	quadList_struct* ql;
	u8 type=*getBlockP(m,i,j,k);
	if(type>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
	else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
	quad_struct* oq=ql->first;
	quad_struct* q;
	if(oq)q=oq->next;
	else q=NULL;
	int i1=i,j1=j,k1=k;
	i%=CLUSTERSIZE;
	j%=CLUSTERSIZE;
	k%=CLUSTERSIZE;
	u8 mID=i+j*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE;
	while(q)
	{
		if(q->mID==mID)
		{
			oq->next=q->next;
			releaseQuad(&q);
			ql->count--;
			q=oq->next;
		}else{
			oq=q;
			q=q->next;
		}
	}
	q=ql->first;
	if(q && q->mID==mID)
	{
		ql->first=q->next;
		ql->count--;
		releaseQuad(&q);
	}
	if(fix)fixGap(m, i1, j1, k1);
}

void fixGap(map_struct* m, int i, int j, int k)
{
	if(*getBlockP(m,i,j,k))return;
	u32 bid=(i)+(j)*(m)->size.x+(k)*(m)->size.y*(m)->size.x;
	if(block(*getBlockP(m,i,j,k-1)))
	{
		u8 light=0;
		surface(m, i, j, k-1, &light);
		getLight(m, i, j, k-1, &light, 0);
		vect3D clusterCoord=getCluster(m,i,j,k-1);
		quadList_struct* ql;
		if(*getBlockP(m,i,j,k-1)>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
		else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		addQuad(ql, m, 0, light, bid-(m)->size.y*(m)->size.x, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k-1);
	}
	if(block(*getBlockP(m,i,j,k+1)))
	{
		u8 light=0;
		surface(m, i, j, k+1, &light);
		getLight(m, i, j, k+1, &light, 1);
		vect3D clusterCoord=getCluster(m,i,j,k+1);
		quadList_struct* ql;
		if(*getBlockP(m,i,j,k+1)>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
		else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		addQuad(ql, m, 1, light, bid+(m)->size.y*(m)->size.x, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k+1);
	}
	
	if(block(*getBlockP(m,i-1,j,k)))
	{
		u8 light=0;
		surface(m, i-1, j, k, &light);
		getLight(m, i-1, j, k, &light, 2);
		vect3D clusterCoord=getCluster(m,i-1,j,k);
		quadList_struct* ql;
		if(*getBlockP(m,i-1,j,k)>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
		else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		addQuad(ql, m, 2, light, bid-1, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i-1, j, k);
	}
	if(block(*getBlockP(m,i+1,j,k)))
	{
		u8 light=0;
		surface(m, i+1, j, k, &light);
		getLight(m, i+1, j, k, &light, 3);
		vect3D clusterCoord=getCluster(m,i+1,j,k);
		quadList_struct* ql;
		if(*getBlockP(m,i+1,j,k)>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
		else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		addQuad(ql, m, 3, light, bid+1, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i+1, j, k);
	}
	
	if(block(*getBlockP(m,i,j-1,k)))
	{
		u8 light=0;
		surface(m, i, j-1, k, &light);
		getLight(m, i, j-1, k, &light, 4);
		vect3D clusterCoord=getCluster(m,i,j-1,k);
		quadList_struct* ql;
		if(*getBlockP(m,i,j-1,k)>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
		else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		addQuad(ql, m, 4, light, bid-(m)->size.x, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j-1, k);
	}
	if(block(*getBlockP(m,i,j+1,k)))
	{
		u8 light=0;
		surface(m, i, j+1, k, &light);
		getLight(m, i, j+1, k, &light, 5);
		vect3D clusterCoord=getCluster(m,i,j+1,k);
		quadList_struct* ql;
		if(*getBlockP(m,i,j+1,k)>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
		else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
		addQuad(ql, m, 5, light, bid+(m)->size.x, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j+1, k);
	}
}

// A plant's quads: crossed planes, or for a bent pumpkin stem a single plane
// (both sides) towards its pumpkin. lit: take the light sources into account
// now (a plant loaded with its cluster gets them with the cluster's lights).
static void plantQuads(quadList_struct* ql, map_struct* m, u8 type, u8 light, u32 bid, u8* data, int i, int j, int k, bool lit)
{
	u8 d, first=6, last=9, l;
	if(isAttachedStem(type))
	{
		first=(type-STEM_ATTACHED>=2)?STEM_PLANE_Y:STEM_PLANE_X;
		last=first+1;
	}
	for(d=first;d<=last;d++)
	{
		l=light;
		if(lit)getLight(m, i, j, k, &l, d);
		addQuad(ql, m, d, l, bid, data, i, j, k);
	}
}

void plantRefresh(map_struct* m, int i, int j, int k, u8 type)
{
	vect3D c=getCluster(m,i,j,k);
	quadList_struct* ql=&m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->cluster[c.z-m->offset.z].quadList;
	u8 light=0;
	removeBlock(m, i, j, k, false);
	(*getBlockP(m, i, j, k))=type;
	surface(m, i, j, k, &light);
	plantQuads(ql, m, type, light, 0, m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->data, i, j, k, true);
	m->superCluster[c.x-m->offset.x][c.y-m->offset.y]->changed=1;
	plantsTrack(i,j,k);
}

// light given by a block (Minecraft levels)
static u8 lightEmission(u8 t)
{
	if(t==13)return 14;
	if(isLitFurnace(t))return FURNACE_LIGHT;
	return 0;
}

void processLight(map_struct* m, int i, int j, int k, const vect3D clusterCoord)
{
	int x, y, z;
	const u8 level=lightEmission(*getBlockP(m,i,j,k));
	if(!level)return;                // queued, but no longer there
	const u32 ijk3=i+j*16+k*16*16;
	u16 x1=max(clusterCoord.x-1,m->offset.x),x2=min(clusterCoord.x+2,m->offset.x+SUPERCLUSTERSIZE-1);
	u16 y1=max(clusterCoord.y-1,m->offset.y),y2=min(clusterCoord.y+2,m->offset.y+SUPERCLUSTERSIZE-1);
	u16 z1=max(clusterCoord.z-1,0),z2=min(clusterCoord.z+2,m->clusterSize.z);
	NOGBA("hoho : %d %d %d %d vs %d %d",x1,x2,y1,y2,clusterCoord.x,clusterCoord.y);
	for(x=x1;x<x2;x++)
	{
		for(y=y1;y<y2;y++)
		{
			for(z=z1;z<z2;z++)
			{
				u32 xyz2=(x+(y<<4)+(z<<8))<<(2); //CLUSTERSIZE EN HARD ! *CLUSTERSIZE
				{
					quadList_struct* ql=&m->superCluster[x-m->offset.x][y-m->offset.y]->cluster[z-m->offset.z].quadList;
					quad_struct* q=ql->first;
					while(q)
					{
							q->light=lightComputeTable[q->light+(lightScale(lightTable[ijk3-xyz2-(ijkmIDtable2[q->mID])+8+8*16+8*16*16+(q->direction<<12)],level)<<8)];
						q=q->next;
					}
				}
				{
					quadList_struct* ql=&m->superCluster[x-m->offset.x][y-m->offset.y]->cluster[z-m->offset.z].specialList;
					quad_struct* q=ql->first;
					while(q)
					{
							q->light=lightComputeTable[q->light+(lightScale(lightTable[ijk3-xyz2-(ijkmIDtable2[q->mID])+8+8*16+8*16*16+(q->direction<<12)],level)<<8)];
						q=q->next;
					}
				}
				addLight(&m->superCluster[x-m->offset.x][y-m->offset.y]->cluster[z-m->offset.z].lightList, m, i-CLUSTERSIZE*x, j-CLUSTERSIZE*y, k-CLUSTERSIZE*z, level);
			}
		}
	}
}

// Remove the light of the source at i,j,k (a torch, or a furnace going out),
// if it was added: a source still waiting in the light queue has none yet.
void lightSourceOff(map_struct* m, int i, int j, int k, u8 level)
{
	int x, y, z;
	vect3D clusterCoord=getCluster(m,i,j,k);
	{
		lightsource_struct* q=m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].lightList.first;
		s8 i2=i-CLUSTERSIZE*clusterCoord.x, j2=j-CLUSTERSIZE*clusterCoord.y, k2=k-CLUSTERSIZE*clusterCoord.z;
		while(q && !(q->i==i2 && q->j==j2 && q->k==k2))q=q->next;
		if(!q)return;
	}
	u16 x1=max(clusterCoord.x-1,m->offset.x),x2=min(clusterCoord.x+2,m->offset.x+SUPERCLUSTERSIZE-1);
	u16 y1=max(clusterCoord.y-1,m->offset.y),y2=min(clusterCoord.y+2,m->offset.y+SUPERCLUSTERSIZE-1);
	u16 z1=max(clusterCoord.z-1,0),z2=min(clusterCoord.z+2,m->clusterSize.z);
	NOGBA("hoho : %d %d %d %d vs %d %d",x1,x2,y1,y2,clusterCoord.x,clusterCoord.y);
	for(x=x1;x<x2;x++)
	{
		for(y=y1;y<y2;y++)
		{
			for(z=z1;z<z2;z++)
			{
				{
					lightsourceList_struct* ql=&m->superCluster[x-m->offset.x][y-m->offset.y]->cluster[z-m->offset.z].lightList;
					lightsource_struct* oq=ql->first;
					s8 i2=(i-CLUSTERSIZE*x), j2=(j-CLUSTERSIZE*y), k2=(k-CLUSTERSIZE*z);
					lightsource_struct* q;
					if(oq)q=oq->next;
					else q=NULL;
					while(q)
					{
						if(q->i==i2 && q->j==j2 && q->k==k2)
						{
							oq->next=q->next;
							releaseLight(&q);
							ql->count--;
							q=oq->next;
						}else{
							oq=q;
							q=q->next;
						}
					}
					q=ql->first;
					if(q && q->i==i2 && q->j==j2 && q->k==k2)
					{
						ql->first=q->next;
						ql->count=(((ql->count&127)-1)&127)|(ql->count&128);
						releaseLight(&q);
					}
				}
				{
					quadList_struct* ql=&m->superCluster[x-m->offset.x][y-m->offset.y]->cluster[z-m->offset.z].quadList;
					quad_struct* q=ql->first;
					while(q)
					{
						int i2=i-(x*CLUSTERSIZE+imIDtable[q->mID])+8,j2=j-(y*CLUSTERSIZE+jmIDtable[q->mID])+8,k2=k-(z*CLUSTERSIZE+kmIDtable[q->mID])+8;
						{
							q->light=max((q->light&127)-lightScale(lightTable[i2+j2*16+k2*16*16+(q->direction<<12)],level),0)|(q->light&(1<<7));
						}
						q=q->next;
					}
				}
				{
					quadList_struct* ql=&m->superCluster[x-m->offset.x][y-m->offset.y]->cluster[z-m->offset.z].specialList;
					quad_struct* q=ql->first;
					while(q)
					{
						int i2=i-(x*CLUSTERSIZE+imIDtable[q->mID])+8,j2=j-(y*CLUSTERSIZE+jmIDtable[q->mID])+8,k2=k-(z*CLUSTERSIZE+kmIDtable[q->mID])+8;
						{
							q->light=max((q->light&127)-lightScale(lightTable[i2+j2*16+k2*16*16+(q->direction<<12)],level),0)|(q->light&(1<<7));
						}
						q=q->next;
					}
				}
			}
		}
	}
}

void lightSourceOn(map_struct* m, int i, int j, int k)
{
	processLight(m,i,j,k,getCluster(m,i,j,k));
}

bool compPos(vect3D p1, vect3D p2)
{
	return p1.x==p2.x && p1.z==p2.z && p1.y==p2.y;
}

void changeBlock(map_struct* m, int i, int j, int k, u8 type)
{
	u8 ot=(*getBlockP(m, i, j, k));
	if(ot==5 || (ot>=WATERTYPE && !type))return;
		if(solid(type))
		{
			vect3D block=(vect3D){i,j,k};
			if(compPos(getPointBlockPos(m, Player.position.x+BBSIZE, Player.position.y-BBSIZE, Player.position.z-6000),block)
			|| compPos(getPointBlockPos(m, Player.position.x+BBSIZE, Player.position.y+BBSIZE, Player.position.z-6000),block)
			|| compPos(getPointBlockPos(m, Player.position.x-BBSIZE, Player.position.y+BBSIZE, Player.position.z-6000),block)
			|| compPos(getPointBlockPos(m, Player.position.x-BBSIZE, Player.position.y-BBSIZE, Player.position.z-6000),block)
			|| compPos(getPointBlockPos(m, Player.position.x+BBSIZE, Player.position.y-BBSIZE, Player.position.z-2000),block)
			|| compPos(getPointBlockPos(m, Player.position.x+BBSIZE, Player.position.y+BBSIZE, Player.position.z-2000),block)
			|| compPos(getPointBlockPos(m, Player.position.x-BBSIZE, Player.position.y+BBSIZE, Player.position.z-2000),block)
			|| compPos(getPointBlockPos(m, Player.position.x-BBSIZE, Player.position.y-BBSIZE, Player.position.z-2000),block)
			|| compPos(getPointBlockPos(m, Player.position.x+BBSIZE, Player.position.y-BBSIZE, Player.position.z+2000),block)
			|| compPos(getPointBlockPos(m, Player.position.x+BBSIZE, Player.position.y+BBSIZE, Player.position.z+2000),block)
			|| compPos(getPointBlockPos(m, Player.position.x-BBSIZE, Player.position.y+BBSIZE, Player.position.z+2000),block)
			|| compPos(getPointBlockPos(m, Player.position.x-BBSIZE, Player.position.y-BBSIZE, Player.position.z+2000),block))return;
		}	
	
	NOGBA("HEHE : %d %d", ot, type);
	vect3D clusterCoord=getCluster(m,i,j,k);
	u8* t=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->highest[(i%CLUSTERSIZE)+(j%CLUSTERSIZE)*CLUSTERSIZE];
	u8 oldt=*t;
	if(!type)
	{
		(*getBlockP(m, i, j, k))=type;
		if(k==(*t))
		{
			while(seeThrough(*getBlockP(m, i, j, *t)))(*t)--;
		}
		removeBlock(m, i, j, k, true);
		if(ot==13)
		{
			lightSourceOff(m,i,j,k,14);
		}else{
			if(isLitFurnace(ot))lightSourceOff(m,i,j,k,FURNACE_LIGHT);
			u8 minim=min(min(waterm(*getBlockP(m, i-1, j, k)),waterm((*getBlockP(m, i+1, j, k)))),min(waterm((*getBlockP(m, i, j-1, k))),waterm((*getBlockP(m, i, j+1, k)))));
			if((*getBlockP(m, i-1, j, k))==minim)addWater(m, i-1, j, k, (*getBlockP(m, i-1, j, k)));
			else if((*getBlockP(m, i+1, j, k))==minim)addWater(m, i+1, j, k, (*getBlockP(m, i+1, j, k)));
			else if((*getBlockP(m, i, j-1, k))==minim)addWater(m, i, j-1, k, (*getBlockP(m, i, j-1, k)));
			else if((*getBlockP(m, i, j+1, k))==minim)addWater(m, i, j+1, k, (*getBlockP(m, i, j+1, k)));
			else if((*getBlockP(m, i, j, k+1))>=WATERTYPE)addWater(m, i, j, k+1, (*getBlockP(m, i, j, k+1)));
		}
		if(oldt!=*t)
		{
			NOGBA("lalala : %d %d",oldt,*t);
			int l;
			for(l=*t;l<=oldt;l++)
			{
				adjustBlockLight(m, i, j, l);
				adjustBlockLight(m, i+1, j, l);
				adjustBlockLight(m, i-1, j, l);
				adjustBlockLight(m, i, j+1, l);
				adjustBlockLight(m, i, j-1, l);
			}
		}
	}
	else if(!solid(ot))
	{
		if(k>(*t) && !seeThrough(type))(*t)=k;
		if(isPlant(type))
		{
			// drawn like a torch (two crossed planes), lit like any block
			vect3D clusterCoord=getCluster(m,i,j,k);
			quadList_struct* ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
			u8 light=0;
			removeBlock(m, i, j, k, false);
			(*getBlockP(m, i, j, k))=type;
			surface(m, i, j, k, &light);
			plantQuads(ql, m, type, light, 0, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k, true);
			plantsTrack(i,j,k);
		}else if(type==13)
		{
			vect3D clusterCoord=getCluster(m,i,j,k);
			quadList_struct* ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
			(*getBlockP(m, i, j, k))=type;
			addQuad(ql, m, 6, 31, 0, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
			addQuad(ql, m, 7, 31, 0, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
			addQuad(ql, m, 8, 31, 0, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
			addQuad(ql, m, 9, 31, 0, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
			
			PROF_START();
			processLight(m,i,j,k,clusterCoord);
			PROF_END(TESTVALUE);
		}else{
			removeBlock(m, i, j, k, false);
			(*getBlockP(m, i, j, k))=type;
			addBlock(m, i, j, k);
			if(i<m->size.x-1 && block(*getBlockP(m,i+1,j,k)))
			{
				removeBlock(m, i+1, j, k, false);
				addBlock(m, i+1, j, k);
			}
			if(i>0 && block(*getBlockP(m,i-1,j,k)))
			{
				removeBlock(m, i-1, j, k, false);
				addBlock(m, i-1, j, k);
			}
			if(j<m->size.y-1 && block(*getBlockP(m,i,j+1,k)))
			{
				removeBlock(m, i, j+1, k, false);
				addBlock(m, i, j+1, k);
			}
			if(j>0 && block(*getBlockP(m,i,j-1,k)))
			{
				removeBlock(m, i, j-1, k, false);
				addBlock(m, i, j-1, k);
			}
			if(k<m->size.z-1 && block(*getBlockP(m,i,j,k+1)))
			{
				removeBlock(m, i, j, k+1, false);
				addBlock(m, i, j, k+1);
			}
			if(k>0 && block(*getBlockP(m,i,j,k-1)))
			{
				removeBlock(m, i, j, k-1, false);
				addBlock(m, i, j, k-1);
			}
		}
		if(oldt!=*t)
		{
			int l;
			for(l=oldt;l<=*t;l++)
			{
				adjustBlockLight(m, i, j, l);
				adjustBlockLight(m, i+1, j, l);
				adjustBlockLight(m, i-1, j, l);
				adjustBlockLight(m, i, j+1, l);
				adjustBlockLight(m, i, j-1, l);
			}
		}
	}
	renderClusterList(m, clusterCoord.x, clusterCoord.y, clusterCoord.z);
	m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->changed=1;
	#ifdef DEBUGMODE
	iprintf("\ncount : %d   ",m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z].quadList.count);
	#endif
}

void addBlock(map_struct* m, int i, int j, int k)
{
	vect3D clusterCoord=getCluster(m,i,j,k);
	quadList_struct* ql;
	const u8 d=(*getBlockP(m,i,j,k));
	if(!d)return;
	else if(d>=WATERTYPE)ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].specialList;
	else ql=&m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->cluster[clusterCoord.z-m->offset.z].quadList;
	
	u32 bid=(i)+(j)*(m)->size.x+(k)*(m)->size.y*(m)->size.x;
	u8 light=0;
	surface(m, i, j, k, &light);
	if(transparent2(m,i,j,k,i,j,k-1))
	{
		getLight(m, i, j, k, &light, 1);
		addQuad(ql, m, 1, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
	}
	if(transparent2(m,i,j,k,i,j,k+1))
	{
		getLight(m, i, j, k, &light, 0);
		addQuad(ql, m, 0, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
	}
	
	if(transparent2(m,i,j,k,i-1,j,k))
	{
		getLight(m, i, j, k, &light, 3);
		addQuad(ql, m, 3, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
	}
	if(transparent2(m,i,j,k,i+1,j,k))
	{
		getLight(m, i, j, k, &light, 2);
		addQuad(ql, m, 2, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
	}
	
	if(transparent2(m,i,j,k,i,j-1,k))
	{
		getLight(m, i, j, k, &light, 5);
		addQuad(ql, m, 5, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);		
	}
	if(transparent2(m,i,j,k,i,j+1,k))
	{
		getLight(m, i, j, k, &light, 4);
		addQuad(ql, m, 4, light, bid, m->superCluster[clusterCoord.x-m->offset.x][clusterCoord.y-m->offset.y]->data, i, j, k);
	}
}

void precalcCollumn(map_struct* m, int x, int y, u8* t)
{
	int i, j, k;
	int x2=x-m->offset.x, y2=y-m->offset.y;
	x*=CLUSTERSIZE;
	y*=CLUSTERSIZE;
	for(i=x;i<x+CLUSTERSIZE;i++)
	{	
		for(j=y;j<y+CLUSTERSIZE;j++)
		{	
			for(k=0;k<16*CLUSTERSIZE;k++)
			{
				if(!*getBlockPE(m,i,j,k))t[(i-x)+(j-y)*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE]=0;
				else if(isDoor(*getBlockPE(m,i,j,k)) || isLadder(*getBlockPE(m,i,j,k)))t[(i-x)+(j-y)*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE]=1;
				else {t[(i-x)+(j-y)*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE]=(((transparent3(m,i,j,k,i,j,k-1))&1))
																|((((transparent3(m,i,j,k,i,j,k+1))&1)<<1))
																|((((transparent3(m,i,j,k,i,j-1,k))&1)<<2))
																|((((transparent3(m,i,j,k,i,j+1,k))&1)<<3))
																|((((transparent3(m,i,j,k,i-1,j,k))&1)<<4))
																|((((transparent3(m,i,j,k,i+1,j,k))&1)<<5));
					}
				char ls=(k==getHighest(m, i, j));
				if(!ls && i<m->size.x-1)ls=k>getHighest(m, i+1, j);
				if(!ls && j<m->size.y-1)ls=k>getHighest(m, i, j+1);
				if(!ls && i>0)ls=k>getHighest(m, i-1, j);
				if(!ls && j>0)ls=k>getHighest(m, i, j-1);
				t[(i-x)+(j-y)*CLUSTERSIZE+k*CLUSTERSIZE*CLUSTERSIZE]|=((ls&1)<<6);
			}
		}
	}
	for(i=0;i<16;i++)
	{
		bool d1=m->superCluster[x2][y2]->cluster[i].wall&1, d2=(m->superCluster[x2][y2]->cluster[i].wall>>1)&1, d3=(m->superCluster[x2][y2]->cluster[i].wall>>2)&1;
		t[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*i]|=(d1<<7);
		t[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*i+1]|=(d2<<7);
		t[CLUSTERSIZE*CLUSTERSIZE*CLUSTERSIZE*i+2]|=(d3<<7);
	}
}

void setFog(u8 mode)
{
	fogMode=mode;
	int i;
	if(mode)
	{
		glEnable(GL_FOG);
		glFogShift(2);
		glFogColor(0,0,0,31);
		for(i=0;i<32;i++)glFogDensity(i,(i*36)/10);
		glFogOffset(0x6100);
	}else{
		//TEST TEST TEST
		glEnable(GL_FOG);
		glFogShift(2);
		glFogColor(8,19,21,31);//water
		for(i=0;i<32;i++)glFogDensity(i,127);
		for(i=0;i<31;i++)glFogDensity(i,(i+1)*4);//water
		glFogOffset(0x6500);//water
		//TEST TEST TEST
	}
}

void generateQuads(map_struct* m, clusterColumn_struct* cC, u8* t, u16 x, u16 y)
{
	int i, j, k;
	
	quadList_struct* ql;
	cluster_struct* c=cC->cluster;
	u32 bid;
	c--;
	x*=CLUSTERSIZE;
	y*=CLUSTERSIZE;
	u8* d=cC->data;
	for(i=0;i<CLUSTERSIZE*CLUSTERSIZE;i++)cC->highest[i]=0;
	for(k=0;k<CLUSTERSIZE*16;k++)
	{
		if(!(k%CLUSTERSIZE))
		{
			c++;
			{
				quad_struct *oq=NULL, *q=c->quadList.first;
				while(q)
				{
					oq=q;
					q=q->next;
					releaseQuad(&oq);
				}//optimisable
				c->quadList.first=NULL;c->quadList.count=0;
			}
			{
				quad_struct *oq=NULL, *q=c->specialList.first;
				while(q)
				{
					oq=q;
					q=q->next;
					releaseQuad(&oq);
				}//optimisable
				c->specialList.first=NULL;c->specialList.count=0;
			}
			{
				lightsource_struct *oq=NULL, *q=c->lightList.first;
				while(q)
				{
					oq=q;
					q=q->next;
					releaseLight(&oq);
				}//optimisable
				c->lightList.first=NULL;c->lightList.count=0;
				#ifdef FOGLIGHT
					c->lightList.count=1<<7;
				#endif
			}
			c->wall=(((*t)>>7)&1)|((((*(t+1))>>7)&1)<<1)|((((*(t+2))>>7)&1)<<2);
		}
		u8* h=cC->highest;
		for(j=0;j<CLUSTERSIZE;j++)
		{
			for(i=0;i<CLUSTERSIZE;i++)
			{
				// plants, farmland and marked leaves get random ticks, even where no face shows
				if(*d==LEAVES_DECAY || isPlant(*d) || isFarmland(*d))plantsTrack(x+i, y+j, k);
				if((*t)&63)
				{
					if(!seeThrough(*d))*h=k;
					if(*d>=WATERTYPE)
					{
						ql=&c->specialList;
						if(((*t)&63)!=2 || *d>WATERTYPE)addWater(m, x+i, y+j, k, *d);//à optimisay //pas sûr que la nouvelle condition corrige tout
					}else ql=&c->quadList;
					if(*d>=LADDERTYPE && *d<LADDERTYPE+4+16)
					{
						if(*d<LADDERTYPE+4)
						{
							u8 dir;
							switch(*d+2-LADDERTYPE)
							{
								case 2:
									dir=8+3;
									break;
								case 3:
									dir=8+2;
									break;
								case 4:
									dir=8+5;
									break;
								case 5:
									dir=8+4;
									break;
							}
							const u8 light=((((*t)>>6)&1)<<7);
							addQuad(ql, m, dir, light, bid, cC->data, x+i, y+j, k);
						}else if(*d<DOORTYPE+8){
							const u8 dir=(*d-DOORTYPE)/2+2;
							const u8 light=((((*t)>>6)&1)<<7);
							addQuad(ql, m, dir, light, bid, cC->data, x+i, y+j, k);
							addQuad(ql, m, dir+8, light, bid, cC->data, x+i, y+j, k);
						}else{
							u8 dir=(*d-DOORTYPE)/2+2;
							switch(dir)
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
							const u8 light=((((*t)>>6)&1)<<7);
							addQuad(ql, m, dir, light, bid, cC->data, x+i, y+j, k);
							addQuad(ql, m, dir+8, light, bid, cC->data, x+i, y+j, k);
						}
					}else if(isPlant(*d))
					{
						const u8 light=((((*t)>>6)&1)<<7);
						plantQuads(ql, m, *d, light, bid, cC->data, x+i, y+j, k, false);
					}else if(*d==13)
					{
						addLightProcess(x+i, y+j, k);
						const u8 light=31;
						addQuad(ql,m, 6, light, bid, cC->data, x+i, y+j, k);
						addQuad(ql,m, 7, light, bid, cC->data, x+i, y+j, k);
						addQuad(ql,m, 8, light, bid, cC->data, x+i, y+j, k);
						addQuad(ql,m, 9, light, bid, cC->data, x+i, y+j, k);
					}else{
						const u8 light=((((*t)>>6)&1)<<7);
						if(isLitFurnace(*d))addLightProcess(x+i, y+j, k);
						if((*t)&1){addQuad(ql,m, 1, light, bid, cC->data, x+i, y+j, k);}
						if(((*t)>>1)&1){addQuad(ql,m, 0, light, bid, cC->data, x+i, y+j, k);}
						if(((*t)>>2)&1){addQuad(ql,m, 5, light, bid, cC->data, x+i, y+j, k);}
						if(((*t)>>3)&1){addQuad(ql,m, 4, light, bid, cC->data, x+i, y+j, k);}
						if(((*t)>>4)&1){addQuad(ql,m, 3, light, bid, cC->data, x+i, y+j, k);}
						if(((*t)>>5)&1){addQuad(ql,m, 2, light, bid, cC->data, x+i, y+j, k);}
						#ifdef FOGLIGHT
							c->lightList.count&=light;
						#endif
					}
				}
				t++;
				d++;
				h++;
			}
		}
	}
}

void translateSuperCluster(map_struct* m, u8 dir)
{
	int i, j;
	
	clusterColumn_struct* temp[32];
	
	switch(dir)
	{
		case 0:
			for(j=0;j<32;j++)
			{
				if(m->superCluster[30][j]->changed){globalSaveMap(m);break;}
			}
			for(j=0;j<32;j++)
			{
				temp[j]=m->transitionCluster[j+32];
				m->transitionCluster[j+32]=m->superCluster[31][j];
			}
			for(i=32-1;i>0;i--)
			{
				for(j=0;j<32;j++)
				{
					m->superCluster[i][j]=m->superCluster[i-1][j];
				}
			}
			for(j=0;j<32;j++)
			{
				m->superCluster[0][j]=m->transitionCluster[j];
				m->transitionCluster[j]=temp[j];
			}
			m->offset.x--;
			m->transitioning[0]=34;
			break;
		case 1:
			for(j=0;j<32;j++)
			{
				if(m->superCluster[1][j]->changed){globalSaveMap(m);break;}
			}
			for(j=0;j<32;j++)
			{
				temp[j]=m->transitionCluster[j];
				m->transitionCluster[j]=m->superCluster[0][j];
			}
			for(i=0;i<32-1;i++)
			{
				for(j=0;j<32;j++)
				{
					m->superCluster[i][j]=m->superCluster[i+1][j];
				}
			}
			for(j=0;j<32;j++)
			{
				m->superCluster[31][j]=m->transitionCluster[j+32];
				m->transitionCluster[j+32]=temp[j];
			}
			m->offset.x++;
			m->transitioning[1]=34;
			break;
		case 2:
			for(j=0;j<32;j++)
			{
				if(m->superCluster[j][30]->changed){globalSaveMap(m);break;}
			}
			for(j=0;j<32;j++)
			{
				temp[j]=m->transitionCluster[j+32*3];
				m->transitionCluster[j+32*3]=m->superCluster[j][31];
			}
			for(i=32-1;i>0;i--)
			{
				for(j=0;j<32;j++)
				{
					m->superCluster[j][i]=m->superCluster[j][i-1];
				}
			}
			for(j=0;j<32;j++)
			{
				m->superCluster[j][0]=m->transitionCluster[j+32*2];
				m->transitionCluster[j+32*2]=temp[j];
			}
			m->offset.y--;
			m->transitioning[2]=34;
			break;
		case 3:
			for(j=0;j<32;j++)
			{
				if(m->superCluster[j][1]->changed){globalSaveMap(m);break;}
			}
			for(j=0;j<32;j++)
			{
				temp[j]=m->transitionCluster[j+32*2];
				m->transitionCluster[j+32*2]=m->superCluster[j][0];
			}
			for(i=0;i<32-1;i++)
			{
				for(j=0;j<32;j++)
				{
					m->superCluster[j][i]=m->superCluster[j][i+1];
				}
			}
			for(j=0;j<32;j++)
			{
				m->superCluster[j][31]=m->transitionCluster[j+32*3];
				m->transitionCluster[j+32*3]=temp[j];
			}
			m->offset.y++;
			m->transitioning[3]=34;
			break;
	}
}

void initMap(map_struct* m, vect3D clusterSize)
{
	int i;

	cull=true;
	
	initWater();
	initUVmap();
	initXYmap();
	initLightMap();
	initmIDTables();
	initLightTable();
	initDegradTable();
	initLightCache();
	initQuadCache();
	
	//DEBUG
	initStats(&frameTime);
	initStats(&streamRead);
	initStats(&streamCalc);
	initStats(&columnWrite);
	
	cullMagic=1;
	m->clusterSize=clusterSize;
	m->size.x=clusterSize.x*CLUSTERSIZE;m->size.y=clusterSize.y*CLUSTERSIZE;m->size.z=clusterSize.z*CLUSTERSIZE;
	iprintf("RAM after data :\n%dko used, %dko free    \n",DS_UsedMem()/1024,DS_FreeMem()/1024);
	NOGBA("RAM after data : %dko used, %dko free    \n",DS_UsedMem()/1024,DS_FreeMem()/1024);
	initSuperCluster(m);
	for(i=0;i<SUPERCLUSTERSIZE*SUPERCLUSTERSIZE*m->clusterSize.z;i++){m->clusterDraw[i]=0;}
	for(i=0;i<SUPERCLUSTERSIZE*SUPERCLUSTERSIZE*m->clusterSize.z;i++){m->clusterDrawn[i]=0;}
	m->transitioning[0]=0;
	m->transitioning[1]=0;
	m->transitioning[2]=0;
	m->transitioning[3]=0;
	iprintf("RAM after superclusters :\n%dko used, %dko free    \n",DS_UsedMem()/1024,DS_FreeMem()/1024);
	NOGBA("RAM after superclusters : %dko used, %dko free    \n",DS_UsedMem()/1024,DS_FreeMem()/1024);
	openList.size=0;
	closedList.size=0;
	
	testCursor=0;
	
}

void initClusterColumn(clusterColumn_struct* cC)
{
	int k;
	for(k=0;k<16;k++)
	{
		cC->cluster[k].quadList.first=NULL;
		cC->cluster[k].quadList.count=0;
		cC->cluster[k].specialList.first=NULL;
		cC->cluster[k].specialList.count=0;
		cC->cluster[k].lightList.first=NULL;
		cC->cluster[k].lightList.count=0;
	}
}

void initSuperCluster(map_struct* m)
{
	int i, j;
	m->offset=(vect3D){0,0,0};
	for(i=0;i<SUPERCLUSTERSIZE;i++)
	{
		for(j=0;j<SUPERCLUSTERSIZE;j++)
		{
			m->superCluster[i][j]=malloc(sizeof(clusterColumn_struct));
			initClusterColumn(m->superCluster[i][j]);
		}
	}
}

void freeMap(map_struct* m)
{
	int i, j;
	if(openMap==openMapNOCASH)fclose(m->fileHandle);
	else{
		if(m->fileMap)free(m->fileMap);
		if(m->headerMap)free(m->headerMap);
		if(m->fileHandle)free(m->fileHandle);
	}
	for(i=0;i<SUPERCLUSTERSIZE;i++)
	{
		free(m->transitionCluster[i]);
		free(m->transitionCluster[i+32]);
		free(m->transitionCluster[i+32*2]);
		free(m->transitionCluster[i+32*3]);
		for(j=0;j<SUPERCLUSTERSIZE;j++)
		{
			free(m->superCluster[i][j]);
		}
	}
	if(m->header)free(m->header);
}

FILE_POSITION _FAT_getPosition(u32* pos);

void openMap2048(char* filename, map_struct* m)
{
	m->fileHandle=(void*)sOpen(filename);
	FILE_STRUCT* fZ=(FILE_STRUCT*)m->fileHandle;
	TESTVALUE3=fZ->partition->sectorsPerCluster;
	switch(fZ->partition->sectorsPerCluster)
	{
		case 1:
			openMap=&openMap512;
			readClusterColumn=&readClusterColumn512;
			writeClusterColumn=&writeClusterColumn512;
			openMap(filename,m);
			fsFormat=3;
			return;
		case 2:
			openMap=&openMap1024;
			readClusterColumn=&readClusterColumn1024;
			writeClusterColumn=&writeClusterColumn1024;
			fsFormat=2;
			openMap(filename,m);
			return;
	}
	m->header=malloc(2048);
	u32 poZ;
	sSeek(fZ,0,SEEK_SET);
	m->headerMap=malloc(sizeof(u32));
	FILE_POSITION fpZ=_FAT_getPosition(&poZ);
	*(m->headerMap)=_FAT_fat_clusterToSector(fZ->partition,fpZ.cluster)+fpZ.sector;
	sRead(fZ, (char*)m->header, 2048);
	m->clusterSize=(vect3D){m->header->sizeX,m->header->sizeY,16};
	
	if(m->header->magicVersionNumber!=VERSIONMAGIC)
	{
		m->header->spawnX=64;
		m->header->spawnY=64;
		m->header->spawnZ=0;
	}
	
	m->fileMap=malloc(sizeof(u32)*m->clusterSize.x*m->clusterSize.y);
	int i, j;
	sSeek(fZ,2048,SEEK_SET);//test
	for(j=0;j<m->clusterSize.y;j++)
	{
		for(i=0;i<m->clusterSize.x;i++)
		{
			u32 po;
			FILE_POSITION fp=_FAT_getPosition(&po);
			m->fileMap[i+j*m->clusterSize.x]=_FAT_fat_clusterToSector(fZ->partition,fp.cluster)+fp.sector;
			sSeek(fZ,(4*4*4)*16+(4*4*4)*16,SEEK_CUR);//test
		}
	}
}

void openMap1024(char* filename, map_struct* m)
{
	m->fileHandle=(void*)sOpen(filename);
	FILE_STRUCT* fZ=(FILE_STRUCT*)m->fileHandle;
	m->header=malloc(2048);
	sRead(fZ, (char*)m->header, 2048);
	u32 poZ;
	sSeek(fZ,0,SEEK_SET);
	m->headerMap=malloc(sizeof(u32)*2);
	FILE_POSITION fpZ=_FAT_getPosition(&poZ);
	m->headerMap[0]=_FAT_fat_clusterToSector(fZ->partition,fpZ.cluster)+fpZ.sector;
	sSeek(fZ,1024,SEEK_SET);
	fpZ=_FAT_getPosition(&poZ);
	m->headerMap[1]=_FAT_fat_clusterToSector(fZ->partition,fpZ.cluster)+fpZ.sector;
	if(m->header->magicVersionNumber!=VERSIONMAGIC)
	{
		m->header->spawnX=64;
		m->header->spawnY=64;
	}
	m->clusterSize=(vect3D){m->header->sizeX,m->header->sizeY,16};
	m->fileMap=malloc(sizeof(u32)*m->clusterSize.x*m->clusterSize.y*2);
	int i, j;
	sSeek(fZ,2048,SEEK_SET);//test
	for(j=0;j<m->clusterSize.y;j++)
	{
		for(i=0;i<m->clusterSize.x*2;i++)
		{
			u32 po;
			FILE_POSITION fp=_FAT_getPosition(&po);
			m->fileMap[i+j*m->clusterSize.x*2]=_FAT_fat_clusterToSector(fZ->partition,fp.cluster)+fp.sector;
			sSeek(fZ,(4*4*4)*16,SEEK_CUR);//test
		}
	}
}

void openMap512(char* filename, map_struct* m)
{
	m->fileHandle=(void*)sOpen(filename);
	FILE_STRUCT* fZ=(FILE_STRUCT*)m->fileHandle;
	m->header=malloc(2048);
	sRead(fZ, (char*)m->header, 2048);
	u32 poZ;
	sSeek(fZ,0,SEEK_SET);
	m->headerMap=malloc(sizeof(u32)*4);
	FILE_POSITION fpZ=_FAT_getPosition(&poZ);
	m->headerMap[0]=_FAT_fat_clusterToSector(fZ->partition,fpZ.cluster)+fpZ.sector;
	sSeek(fZ,512,SEEK_CUR);
	fpZ=_FAT_getPosition(&poZ);
	m->headerMap[1]=_FAT_fat_clusterToSector(fZ->partition,fpZ.cluster)+fpZ.sector;
	sSeek(fZ,512,SEEK_CUR);
	fpZ=_FAT_getPosition(&poZ);
	m->headerMap[2]=_FAT_fat_clusterToSector(fZ->partition,fpZ.cluster)+fpZ.sector;
	sSeek(fZ,512,SEEK_CUR);
	fpZ=_FAT_getPosition(&poZ);
	m->headerMap[3]=_FAT_fat_clusterToSector(fZ->partition,fpZ.cluster)+fpZ.sector;
	if(m->header->magicVersionNumber!=VERSIONMAGIC)
	{
		m->header->spawnX=64;
		m->header->spawnY=64;
	}
	m->clusterSize=(vect3D){m->header->sizeX,m->header->sizeY,16};
	m->fileMap=malloc(sizeof(u32)*m->clusterSize.x*m->clusterSize.y*4);
	int i, j;
	sSeek(fZ,2048,SEEK_SET);//test
	for(j=0;j<m->clusterSize.y;j++)
	{
		for(i=0;i<m->clusterSize.x*4;i++)
		{
			u32 po;
			FILE_POSITION fp=_FAT_getPosition(&po);
			m->fileMap[i+j*m->clusterSize.x*4]=_FAT_fat_clusterToSector(fZ->partition,fp.cluster)+fp.sector;
			sSeek(fZ,(4*4*4)*16/2,SEEK_CUR);//test
		}
	}
}

void openMapNOCASH(char* filename, map_struct* m)
{
	m->fileHandle=fopen(filename,"rb");
	m->header=malloc(2048);
	fread(m->header, 2048, 1, (FILE*)m->fileHandle);
	if(m->header->magicVersionNumber!=VERSIONMAGIC)
	{
		m->header->spawnX=64;
		m->header->spawnY=64;
	}
	m->clusterSize=(vect3D){m->header->sizeX,m->header->sizeY,16};
	NOGBA("file handle : %p (%s)",m->fileHandle,filename);
}

void loadTestMap(map_struct* m)
{
	openMap(mapPath, m);
	initMap(m,(vect3D){m->header->sizeX,m->header->sizeY,16});
	TESTVALUE2=0;
	int i8, j8;
	NOGBA("spawn ! %d %d (%d)",m->header->spawnX,m->header->spawnY,m->header->sizeX);
	if(m->header->spawnX<(SUPERCLUSTERSIZE/2)*CLUSTERSIZE)
	{
		m->offset.x=0;
	}else if(m->header->spawnX>(m->header->sizeX-(SUPERCLUSTERSIZE/2))*CLUSTERSIZE)
	{
		m->offset.x=((m->header->sizeX-SUPERCLUSTERSIZE)*CLUSTERSIZE)/CLUSTERSIZE;
	}else{
		m->offset.x=m->header->spawnX-(SUPERCLUSTERSIZE/2)*CLUSTERSIZE;
		m->offset.x=(m->offset.x-(m->offset.x%(CLUSTERSIZE)))/CLUSTERSIZE;
	}
	if(m->header->spawnY<(SUPERCLUSTERSIZE/2)*CLUSTERSIZE)
	{
		m->offset.y=0;
	}else if(m->header->spawnY>(m->header->sizeY-(SUPERCLUSTERSIZE/2))*CLUSTERSIZE)
	{
		m->offset.y=((m->header->sizeY-SUPERCLUSTERSIZE)*CLUSTERSIZE)/CLUSTERSIZE;
	}else{
		m->offset.y=m->header->spawnY-(SUPERCLUSTERSIZE/2)*CLUSTERSIZE;
		m->offset.y=(m->offset.y-(m->offset.y%(CLUSTERSIZE)))/CLUSTERSIZE;
	}
	NOGBA("offset ! %d %d",m->offset.x,m->offset.y);
	for(j8=0;j8<32;j8++)
	{
		for(i8=0;i8<32;i8++)
		{
			readClusterColumn(m, i8+m->offset.x, j8+m->offset.y, m->superCluster[i8][j8], m->transitionStuff, m->fileHandle);
		}
	}
	//init transition X
	i8=-1;
	for(j8=0;j8<32;j8++)
	{
		m->transitionCluster[j8]=malloc(sizeof(clusterColumn_struct));
		initClusterColumn(m->transitionCluster[j8]);
		if(m->offset.x)readClusterColumn(m, i8+m->offset.x, j8+m->offset.y, m->transitionCluster[j8], m->transitionStuff, m->fileHandle);
	}
	i8=32;
	for(j8=0;j8<32;j8++)
	{
		m->transitionCluster[j8+32]=malloc(sizeof(clusterColumn_struct));
		initClusterColumn(m->transitionCluster[j8+32]);
		if(m->offset.x<m->header->sizeX-(SUPERCLUSTERSIZE)-1)readClusterColumn(m, i8+m->offset.x, j8+m->offset.y, m->transitionCluster[j8+32], m->transitionStuff, m->fileHandle);
	}
	
	j8=-1;
	for(i8=0;i8<32;i8++)
	{
		m->transitionCluster[i8+32*2]=malloc(sizeof(clusterColumn_struct));
		initClusterColumn(m->transitionCluster[i8+32*2]);
		if(m->offset.y)readClusterColumn(m, i8+m->offset.x, j8+m->offset.y, m->transitionCluster[i8+32*2], m->transitionStuff, m->fileHandle);
	}
	j8=32;
	for(i8=0;i8<32;i8++)
	{
		m->transitionCluster[i8+32*3]=malloc(sizeof(clusterColumn_struct));
		initClusterColumn(m->transitionCluster[i8+32*3]);
		if(m->offset.y<m->header->sizeY-(SUPERCLUSTERSIZE)-1)readClusterColumn(m, i8+m->offset.x, j8+m->offset.y, m->transitionCluster[i8+32*3], m->transitionStuff, m->fileHandle);
	}
}

void readClusterColumn2048(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f)
{
	PROF_START();
	readSectors(m->fileMap[i+m->clusterSize.x*j], 2, c->data);
	readSectors(m->fileMap[i+m->clusterSize.x*j]+2, 2, t);
	int time; PROF_END(time);
	addValue(&streamRead,time);
	PROF_START();
	c->changed=0;
	generateQuads(m, c, t, i, j); // ADD SUPPORT FOR WALL GENERATION (done ?)
	PROF_END(time);
	addValue(&streamCalc,time);
	#ifdef DEBUGMODE
	iprintf("\nprecalc : %d  ",time);
	#endif
}

void readClusterColumn1024(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f)
{
	PROF_START();
	readSectors(m->fileMap[i*2+m->clusterSize.x*2*j], 2, c->data);
	readSectors(m->fileMap[i*2+1+m->clusterSize.x*2*j], 2, t);
	int time; PROF_END(time);
	addValue(&streamRead,time);
	PROF_START();
	c->changed=0;
	generateQuads(m, c, t, i, j); // ADD SUPPORT FOR WALL GENERATION (done ?)
	PROF_END(time);
	addValue(&streamCalc,time);
}

void readClusterColumn512(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f)
{
	PROF_START();
	readSectors(m->fileMap[i*4+m->clusterSize.x*4*j], 1, c->data);
	readSectors(m->fileMap[i*4+1+m->clusterSize.x*4*j], 1, &c->data[512]);
	readSectors(m->fileMap[i*4+2+m->clusterSize.x*4*j], 1, t);
	readSectors(m->fileMap[i*4+3+m->clusterSize.x*4*j], 1, &t[512]);
	int time; PROF_END(time);
	addValue(&streamRead,time);
	PROF_START();
	c->changed=0;
	generateQuads(m, c, t, i, j); // ADD SUPPORT FOR WALL GENERATION (done ?)
	PROF_END(time);
	addValue(&streamCalc,time);
}

void readClusterColumnNOCASH(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f)
{
	FILE* fp=(FILE*)f;
	fseek(fp,2048+((4*4*4)*16*2)*(i+j*m->clusterSize.x),SEEK_SET);
	fread(c,(4*4*4)*16,1,f);
	fread(t,(4*4*4)*16,1,f);
	c->changed=0;
	generateQuads(m, c, t, i, j); // ADD SUPPORT FOR WALL GENERATION (done ?)
}

void writeClusterColumn2048(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f)
{
	int time;
	PROF_START();
	precalcCollumn(m, i, j, t);
	writeSectors(m->fileMap[i+m->clusterSize.x*j], 2, c->data);
	writeSectors(m->fileMap[i+m->clusterSize.x*j]+2, 2, t);
	PROF_END(time);
	addValue(&columnWrite,time);
	if(!c->changed)c->changed=2;
}

void writeClusterColumn1024(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f)
{
	int time;
	PROF_START();
	precalcCollumn(m, i, j, t);
	writeSectors(m->fileMap[i*2+m->clusterSize.x*2*j], 2, c->data);
	writeSectors(m->fileMap[i*2+1+m->clusterSize.x*2*j], 2, t);
	PROF_END(time);
	addValue(&columnWrite,time);
	if(!c->changed)c->changed=2;
}

void writeClusterColumn512(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f)
{
	int time;
	PROF_START();
	precalcCollumn(m, i, j, t);
	writeSectors(m->fileMap[i*4+m->clusterSize.x*4*j], 1, c->data);
	writeSectors(m->fileMap[i*4+1+m->clusterSize.x*4*j], 1, &c->data[512]);
	writeSectors(m->fileMap[i*4+2+m->clusterSize.x*4*j], 1, t);
	writeSectors(m->fileMap[i*4+3+m->clusterSize.x*4*j], 1, &t[512]);
	PROF_END(time);
	addValue(&columnWrite,time);
	if(!c->changed)c->changed=2;
}

void writeClusterColumnNOCASH(map_struct* m, u16 i, u16 j, clusterColumn_struct* c, u8* t, void* f)
{
	//DO NOTHING
	precalcCollumn(m, i, j, t);
	if(!c->changed)c->changed=2;
}

void updateLightMap(void)
{
	u8	d=0, i=0;
	u32* lm=lightMap;
	for(i=0;i<32-LIGHTAMBIENT;i++)
	{
		u8 l=LIGHTAMBIENT+(i&127);
		*lm=RGB15(l,l,l);
		lm++;
	}
	for(i=i;i<128;i++)*(lm++)=RGB15(31,31,31);
	do{
		u8 l=min(LIGHTAMBIENT2+(lightSun[d]+(i&127)),31);
		*lm=RGB15(l,l,l);
		lm++;
		i++;
		if(l==31)break;
	}while(i);
	for(i=i;i;i++)*(lm++)=RGB15(31,31,31);
	for(d=1;d<6;d++)
	{
		memcpy(lm,lightMap,128*4);
		lm+=128;
		i=128;
		do{
			u8 l=min(LIGHTAMBIENT2+(lightSun[d]+(i&127)),31);
			*lm=RGB15(l,l,l);
			lm++;
			i++;
			if(l==31)break;
		}while(i);
		for(i=i;i;i++)*(lm++)=RGB15(31,31,31);
	}
	memcpy(&lightMap[256*10],&lightMap[256*3],256*4);
	memcpy(&lightMap[256*11],&lightMap[256*2],256*4);
	memcpy(&lightMap[256*12],&lightMap[256*5],256*4);
	memcpy(&lightMap[256*13],&lightMap[256*4],256*4);
	// upright planes are shaded like a side face (a torch's quads, lit at 31,
	// stay at full light)
	for(d=6;d<10;d++)memcpy(&lightMap[256*d],&lightMap[256*2],256*4);
	for(d=14;d<QUAD_DIRECTIONS;d++)memcpy(&lightMap[256*d],&lightMap[256*2],256*4);
}


void drawTestQuadOpt(map_struct* m, quad_struct* q)
{
	#ifdef DEBUGMODE
	testquads++;
	#endif
	
	u32* uv=&uvMapCur[q->type<<2];
	const u16 d=(q->direction<<8);
	u32* vert=&xyMap[(q->mID<<2)+d];
	GFX_COLOR = lightMapCur[d+q->light];
	
	GFX_TEX_COORD = *uv;
	GFX_VERTEX10 = *vert;
	GFX_TEX_COORD = *(++uv);
	GFX_VERTEX10 = *(++vert);
	GFX_TEX_COORD = *(++uv);
	GFX_VERTEX10 = *(++vert);
	GFX_TEX_COORD = *(++uv);
	GFX_VERTEX10 = *(++vert);
}

void drawQuadList(map_struct* m, quadList_struct* ql, u16 x, u16 y, u16 z)
{
	if(!ql->count)return;
	quad_struct* q=ql->first;
	int i=0;
	glPushMatrix();
	glTranslatef32(x*bsize,y*bsize,z*bsize);
	glBegin(GL_QUADS);
		while(q)
		{
			i++;
			drawTestQuadOpt(m, q);
			q=q->next;
		}
	glPopMatrix(1);
}

void drawDrops(map_struct* m)
{
	int n, dir;
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
	for(n=0;n<DROPS_MAX;n++)
	{
		const drop_struct* d=dropsGet(n);
		if(!d)continue;
		int i=(d->x+DROP_UNIT/2)>>12, j=(d->y+DROP_UNIT/2)>>12, k=(d->z+DROP_UNIT/2)>>12;
		if(!dropsLoaded(m,i,j,k<0?0:k))continue;
		// sky light needs the neighbouring columns, so skip it on the edge of the loaded area
		u8 light=0;
		if(k<0)light=0;                // falling through the void, under the world
		else if(dropsLoaded(m,i-1,j-1,k) && dropsLoaded(m,i+1,j+1,k))
		{
			surface(m,i,j,k,&light);
			getLight(m,i,j,k,&light,0);
		}else light=1<<7;
		int32 bob=(sinLerp(d->age*600)*DROP_HALF/2)>>12;
		glPushMatrix();
		glTranslatef32(d->x/SCALEFACTOR-m->offset.x*bsize, d->y/SCALEFACTOR-m->offset.y*bsize, (d->z+DROP_HALF/2+bob)/SCALEFACTOR-m->offset.z*bsize);
		glRotateZi(d->age*300);
		glScalef32(inttof32(1)/4,inttof32(1)/4,inttof32(1)/4);
		if(d->item==LADDERTYPE || d->item==DOORTYPE || (d->item>=ITEM_TOOL_FIRST && !isCubeItem(d->item)))
		{
			// items are flat sprites that spin, as in Minecraft
			glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
			glTranslatef32(-64,0,0);
			glBegin(GL_QUADS);
			u32* uv=&uvMap[blocks[d->item].bottom<<2];
			u32* vert=&xyMap[2<<8];
			GFX_COLOR = lightMap[(2<<8)+light];
			GFX_TEX_COORD = *uv;
			GFX_VERTEX10 = *vert;
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = *(++vert);
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = *(++vert);
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = *(++vert);
			glPopMatrix(1);
			glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
			continue;
		}
		glBegin(GL_QUADS);
		for(dir=0;dir<6;dir++)
		{
			u8 tex=(dir==0)?blocks[d->item].top:((dir==1)?blocks[d->item].bottom:blocks[d->item].side);
			u32* uv=&uvMap[tex<<2];
			u32* vert=&xyMap[dir<<8];
			GFX_COLOR = lightMap[(dir<<8)+light];
			GFX_TEX_COORD = *uv;
			GFX_VERTEX10 = *vert;
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = *(++vert);
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = *(++vert);
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = *(++vert);
		}
		glPopMatrix(1);
	}
}

void drawCursor(map_struct* m, int i, int j, int k)
{
	glPushMatrix();
	glTranslatef32(((tilesize2*i)<<6)-m->offset.x*bsize,((tilesize2*j)<<6)-m->offset.y*bsize,((tilesize2*k)<<6)-m->offset.z*bsize);
	quad_struct q=(quad_struct){0, cursorDir, 0, 31, NULL};
	glScalef32(inttof32(11)/10,inttof32(11)/10,inttof32(11)/10);
		glBegin(GL_QUADS);
	u8 progress=survivalMiningProgress();
	if(progress)
	{
		u32* uv=&uvMapCur[q.type<<2];
		u32* vert=&xyMap[(q.mID<<2)+(q.direction<<8)];
		GFX_COLOR = RGB15(31,31-progress/9,31-progress/9);
		GFX_TEX_COORD = *uv;
		GFX_VERTEX10 = *vert;
		GFX_TEX_COORD = *(++uv);
		GFX_VERTEX10 = *(++vert);
		GFX_TEX_COORD = *(++uv);
		GFX_VERTEX10 = *(++vert);
		GFX_TEX_COORD = *(++uv);
		GFX_VERTEX10 = *(++vert);
	}else drawTestQuadOpt(m, &q);
	glPopMatrix(1);
}

void renderClusterList(map_struct* m, int x, int y, int z)
{
	cluster_struct* c=&m->superCluster[x-m->offset.x][y-m->offset.y]->cluster[z-m->offset.z];
	int i, j, k;
	bool d1, d2, d3;
	c->wall=7;
	for(i=0;i<CLUSTERSIZE;i++)
	{
		for(j=0;j<CLUSTERSIZE;j++)
		{
			d1=false;d2=false;d3=false;
			for(k=0;k<CLUSTERSIZE;k++)
			{
				d1=d1||*getBlockPE(m,x*CLUSTERSIZE+i,y*CLUSTERSIZE+j,z*CLUSTERSIZE+k); //no reason, just testing
				d2=d2||*getBlockPE(m,x*CLUSTERSIZE+i,y*CLUSTERSIZE+k,z*CLUSTERSIZE+j);
				d3=d3||*getBlockPE(m,x*CLUSTERSIZE+k,y*CLUSTERSIZE+j,z*CLUSTERSIZE+i);
			}
			c->wall&=(d1)|(d2<<1)|(d3<<2);
		}
	}
	NOGBA("%d %d %d vs %d %d %d : %d",x,y,z,x-m->offset.x,y-m->offset.y,z-m->offset.z,c->wall);
}

void writeMapHeader(map_struct* m)
{
	m->header->magicVersionNumber=VERSIONMAGIC;
	m->header->spawnX=Player.position.x/(rTilesize2)+(SUPERCLUSTERSIZE/2+m->offset.x)*CLUSTERSIZE;
	m->header->spawnY=Player.position.y/(rTilesize2)+(SUPERCLUSTERSIZE/2+m->offset.y)*CLUSTERSIZE;
	m->header->spawnZ=Player.position.z;
	survivalWriteHeader(m);
	NOGBA("player pos : %d %d",m->header->spawnX,m->header->spawnY);
	switch(fsFormat)
	{
		case 1:
			writeSectors(m->headerMap[0], 4, (u8*)m->header);
			break;
		case 2:
			writeSectors(m->headerMap[0], 2, &(((u8*)m->header)[0]));
			writeSectors(m->headerMap[1], 2, &(((u8*)m->header)[1024]));
			break;
		case 3:
			writeSectors(m->headerMap[0], 1, &(((u8*)m->header)[0]));
			writeSectors(m->headerMap[1], 1, &(((u8*)m->header)[512]));
			writeSectors(m->headerMap[2], 1, &(((u8*)m->header)[1024]));
			writeSectors(m->headerMap[3], 1, &(((u8*)m->header)[1024+512]));
			break;
		case 0:
			NOGBA("writing header !");
			break;
	}
}

void globalSaveMap(map_struct* m)
{
	plantsFlush(m);                  // a tree still appearing is finished first
	startSave();
	writeMapHeader(m);
	chestsSave(mapPath);
	furnacesSave(mapPath);
	int i, j;
	for(i=0;i<SUPERCLUSTERSIZE;i++)
	{
		for(j=0;j<SUPERCLUSTERSIZE;j++)
		{
			if(m->superCluster[i][j]->changed==1)
			{
				writeClusterColumn(m, i+m->offset.x, j+m->offset.y, m->superCluster[i][j], m->transitionStuff, m->fileHandle);
				if(i<SUPERCLUSTERSIZE-1)writeClusterColumn(m, i+m->offset.x+1, j+m->offset.y, m->superCluster[i+1][j], m->transitionStuff, m->fileHandle);
				if(i>0)writeClusterColumn(m, i+m->offset.x-1, j+m->offset.y, m->superCluster[i-1][j], m->transitionStuff, m->fileHandle);
				if(j<SUPERCLUSTERSIZE-1)writeClusterColumn(m, i+m->offset.x, j+m->offset.y+1, m->superCluster[i][j+1], m->transitionStuff, m->fileHandle);
				if(j>0)writeClusterColumn(m, i+m->offset.x, j+m->offset.y-1, m->superCluster[i][j-1], m->transitionStuff, m->fileHandle);
			}
		}
	}
	for(i=0;i<SUPERCLUSTERSIZE;i++)
	{
		for(j=0;j<SUPERCLUSTERSIZE;j++)
		{
			m->superCluster[i][j]->changed=0;
		}
	}
	endSave();
}

int maT1, maT2, miT1, miT2;
int rt1, rt2;

void drawTestCube(void)
{
	glPushMatrix();
	glTranslatef32(0,768,0);
	glTranslatef32(480+(cosLerp(walkAngle>>1)>>7),0,-512+(cosLerp(walkAngle>>2)>>8));
	glTranslatef32(0,0,-1000);
	glRotateZi(degreesToAngle(33));
	glRotateXi(cubeAngleX);
	glTranslatef32(0,0,1000);
	if(cubeAngleX && cubeAngleX>-3000)cubeAngleX-=600;
	else cubeAngleX=0;
	glScalef32(inttof32(5)/2,inttof32(5)/2,inttof32(5)/2);
	Game_ApplyMTL(blockSuperTexture);
	glBegin(GL_QUADS);
	
		if(cursorBlock==13 || cursorBlock==LADDERTYPE || cursorBlock==DOORTYPE || (cursorBlock>=ITEM_TOOL_FIRST && !isCubeItem(cursorBlock)))
		{
			u8 type=blocks[cursorBlock].bottom;
			u32* uv=&uvMapCur[type<<2];
			glColor(RGB15(31,31,31));
			
			//side
			GFX_TEX_COORD = *uv;
			GFX_VERTEX10 = NORMAL_PACK((0),(tilesize2*2-tilesize2),(tilesize2*2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0),(0-tilesize2),(tilesize2*2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0),(0-tilesize2),(0-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0),(tilesize2*2-tilesize2),(0-tilesize));
		}else if(cursorBlock==11)
		{
			u8 type=blocks[cursorBlock].top;
			u32* uv=&uvMapCur[type<<2];
			glColor(RGB15(31,31,31));
			
			//side
			GFX_TEX_COORD = *uv;
			GFX_VERTEX10 = NORMAL_PACK((0),(tilesize2*2-tilesize2),(tilesize2*2-tilesize*2));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0),(0-tilesize2),(tilesize2*2-tilesize*2));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0),(0-tilesize2),(0-tilesize*2));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0),(tilesize2*2-tilesize2),(0-tilesize*2));
		}else{
			u8 type=blocks[cursorBlock].top;
			u32* uv=&uvMapCur[type<<2];
			glColor3b(200,200,200);
			
			GFX_TEX_COORD = *uv;
			GFX_VERTEX10 = NORMAL_PACK((0-tilesize),(0-tilesize),(tilesize2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0-tilesize),(tilesize2-tilesize),(tilesize2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((tilesize2-tilesize),(tilesize2-tilesize),(tilesize2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((tilesize2-tilesize),(0-tilesize),(tilesize2-tilesize));
			
			type=blocks[cursorBlock].side;
			uv=&uvMapCur[type<<2];
			glColor3b(100,100,100);
			
			//side
			GFX_TEX_COORD = *uv;
			GFX_VERTEX10 = NORMAL_PACK((0-tilesize),(tilesize2-tilesize),(tilesize2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0-tilesize),(0-tilesize),(tilesize2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0-tilesize),(0-tilesize),(0-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0-tilesize),(tilesize2-tilesize),(0-tilesize));
			
			uv=&uvMapCur[type<<2];
			glColor3b(50,50,50);
			
			GFX_TEX_COORD = *uv;
			GFX_VERTEX10 = NORMAL_PACK((0-tilesize),(0-tilesize),(tilesize2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((tilesize2-tilesize),(0-tilesize),(tilesize2-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((tilesize2-tilesize),(0-tilesize),(0-tilesize));
			GFX_TEX_COORD = *(++uv);
			GFX_VERTEX10 = NORMAL_PACK((0-tilesize),(0-tilesize),(0-tilesize));
		}
	glEnd();
	glPopMatrix(1);
}

void drawTestMap(map_struct* m)
{
	int i, j, k, time;
	
	glPushMatrix();
	glScalef32(inttof32(SCALEFACTOR),inttof32(SCALEFACTOR),inttof32(SCALEFACTOR));
	glTranslatef32(-(SUPERCLUSTERSIZE*CLUSTERSIZE*(tilesize2<<6))/2, -(SUPERCLUSTERSIZE*CLUSTERSIZE*(tilesize2<<6))/2,-(m->size.z*(tilesize2<<6))/2);
	{
		testquads=0;
		PROF_START();
		if(Player.inWater==2)glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK | POLY_FOG); //TEST TEST TEST
		else glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
		Game_FastBind(blockSuperTexture);
		uvMapCur=uvMap;
		listElement_struct* le=closedList.elements;
		lightMapCur=lightMap;
		if(!Player.inCave && fogMode)
		{
			for(i=0;i<closedList.size;i++)
			{
				#ifdef FOGLIGHT
					if(!Player.inCave && !m->superCluster[le->i][le->j]->cluster[le->k].lightList.count && le->i >= Player.clusterCoord.x-1 && le->j >= Player.clusterCoord.y-1 && le->k >= Player.clusterCoord.z-1
					&& le->i <= Player.clusterCoord.x+1 && le->j <= Player.clusterCoord.y+1 && le->k <= Player.clusterCoord.z+1)
					{
						glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK | POLY_FOG);
						lightMapCur=lightMap2;
					}else{
						glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
						lightMapCur=lightMap;
					}
				#endif
				drawQuadList(m, &m->superCluster[le->i][le->j]->cluster[le->k].quadList, le->i, le->j, le->k);
				le++;
			}
		}else{
			for(i=0;i<closedList.size;i++)
			{
				drawQuadList(m, &m->superCluster[le->i][le->j]->cluster[le->k].quadList, le->i, le->j, le->k);
				le++;
			}
		}
		le=closedList.elements;
		Game_FastBind(waterTexture);
		uvMapCur=uvMapWater;
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
		for(i=0;i<closedList.size;i++)
		{
			drawQuadList(m, &m->superCluster[le->i][le->j]->cluster[le->k].specialList, le->i, le->j, le->k);
			le++;
		}
		updateUVwater();
		uvMapCur=uvMap;
		Game_FastBind(blockSuperTexture);
		drawDrops(m);
		Game_FastBind(cursorTexture);
		if(cursorValid)drawCursor(m, testCursorI, testCursorJ, testCursorK);
		
		PROF_END(time);
		#ifdef DEBUGMODE
		iprintf("\ndrawing : %d   ",time);
		#endif
	}
	
	{
		if(m->transitioning[0])
		{
			m->transitioning[0]--;
			int j=m->transitioning[0], i;
			clusterColumn_struct* temp;
			switch(j)
			{
				case 33:
					if(m->offset.y < m->clusterSize.y-SUPERCLUSTERSIZE/*-SUPERCLUSTERSIZE/2*/)
					{
						temp=m->transitionCluster[31+32*3];
						for(i=32-1;i>0;i--)
						{
							m->transitionCluster[i+32*3]=m->transitionCluster[i-1+32*3];
						}
						m->transitionCluster[0+32*3]=temp;
						readClusterColumn(m, m->offset.x, 32+m->offset.y, m->transitionCluster[0+32*3], m->transitionStuff, m->fileHandle);
					}
					break;
				case 32:
					if(m->offset.y > 0)
					{
						temp=m->transitionCluster[31+32*2];
						for(i=32-1;i>0;i--)
						{
							m->transitionCluster[i+32*2]=m->transitionCluster[i-1+32*2];
						}
						m->transitionCluster[0+32*2]=temp;
						readClusterColumn(m, m->offset.x, m->offset.y-1, m->transitionCluster[0+32*2], m->transitionStuff, m->fileHandle);
					}
					break;
				default:
					if(!(m->offset.x > 0))m->transitioning[0]=0;
					else readClusterColumn(m, m->offset.x-1, j+m->offset.y, m->transitionCluster[j], m->transitionStuff, m->fileHandle);
					break;
			}
		}else if(m->transitioning[1])
		{
			m->transitioning[1]--;
			int j=m->transitioning[1], i;
			clusterColumn_struct* temp;
			switch(j)
			{
				case 33:
					if(m->offset.y < m->clusterSize.y-SUPERCLUSTERSIZE/*-SUPERCLUSTERSIZE/2*/)
					{
						temp=m->transitionCluster[0+32*3];
						for(i=0;i<32-1;i++)
						{
							m->transitionCluster[i+32*3]=m->transitionCluster[i+1+32*3];
						}
						m->transitionCluster[31+32*3]=temp;
						readClusterColumn(m, 31+m->offset.x, 32+m->offset.y, m->transitionCluster[31+32*3], m->transitionStuff, m->fileHandle);
					}
					break;
				case 32:
					if(m->offset.y > 0)
					{
						temp=m->transitionCluster[0+32*2];
						for(i=0;i<32-1;i++)
						{
							m->transitionCluster[i+32*2]=m->transitionCluster[i+1+32*2];
						}
						m->transitionCluster[31+32*2]=temp;
						readClusterColumn(m, 31+m->offset.x, m->offset.y-1, m->transitionCluster[31+32*2], m->transitionStuff, m->fileHandle);
					}
					break;
				default:
					if(!(m->offset.x < m->clusterSize.x-SUPERCLUSTERSIZE/*-SUPERCLUSTERSIZE/2*/))m->transitioning[1]=0;
					else readClusterColumn(m, 32+m->offset.x, j+m->offset.y, m->transitionCluster[j+32], m->transitionStuff, m->fileHandle);
					break;
			}
		}else if(m->transitioning[2])
		{
			m->transitioning[2]--;
			int j=m->transitioning[2], i;
			clusterColumn_struct* temp;
			switch(j)
			{
				case 33:
					if(m->offset.x < m->clusterSize.x-SUPERCLUSTERSIZE/*-SUPERCLUSTERSIZE/2*/)
					{
						temp=m->transitionCluster[31+32];
						for(i=32-1;i>0;i--)
						{
							m->transitionCluster[i+32*1]=m->transitionCluster[i-1+32*1];
						}
						m->transitionCluster[0+32*1]=temp;
						readClusterColumn(m, 32+m->offset.x, m->offset.y, m->transitionCluster[0+32*1], m->transitionStuff, m->fileHandle);
					}
					break;
				case 32:
					if(m->offset.x > 0)
					{
						temp=m->transitionCluster[31+0];
						for(i=32-1;i>0;i--)
						{
							m->transitionCluster[i+32*0]=m->transitionCluster[i-1+32*0];
						}
						m->transitionCluster[0+32*0]=temp;
						readClusterColumn(m, m->offset.x-1, m->offset.y, m->transitionCluster[0+32*0], m->transitionStuff, m->fileHandle);
					}
					break;
				default:
					if(!(m->offset.y > 0))m->transitioning[2]=0;
					else readClusterColumn(m, j+m->offset.x, m->offset.y-1, m->transitionCluster[j+32*2], m->transitionStuff, m->fileHandle);
					break;
			}	
		}else if(m->transitioning[3])
		{
			m->transitioning[3]--;
			int j=m->transitioning[3], i;
			clusterColumn_struct* temp;
			switch(j)
			{
				case 33:
					if(m->offset.x < m->clusterSize.x-SUPERCLUSTERSIZE/*-SUPERCLUSTERSIZE/2*/)
					{
						temp=m->transitionCluster[0+32*1];
						for(i=0;i<32-1;i++)
						{
							m->transitionCluster[i+32*1]=m->transitionCluster[i+1+32*1];
						}
						m->transitionCluster[31+32*1]=temp;
						readClusterColumn(m, 32+m->offset.x, m->offset.y+31, m->transitionCluster[31+32*1], m->transitionStuff, m->fileHandle);
					}
					break;
				case 32:
					if(m->offset.x > 0)
					{
						temp=m->transitionCluster[0+32*0];
						for(i=0;i<32-1;i++)
						{
							m->transitionCluster[i+32*0]=m->transitionCluster[i+1+32*0];
						}
						m->transitionCluster[31+32*0]=temp;
						readClusterColumn(m, m->offset.x-1, m->offset.y+31, m->transitionCluster[31+32*0], m->transitionStuff, m->fileHandle);
					}
					break;
				default:
					if(!(m->offset.y < m->clusterSize.y-SUPERCLUSTERSIZE/*-SUPERCLUSTERSIZE/2*/))m->transitioning[3]=0;
					else readClusterColumn(m, j+m->offset.x, 32+m->offset.y, m->transitionCluster[j+32*3], m->transitionStuff, m->fileHandle);
					break;
			}
		}
	}
	#ifdef DEBUGMODE
	iprintf("\nstreaming : %d (%d)         ",time,m->transitioning[1]);
	#endif
	
	glPopMatrix(1);
		glPopMatrix(1);
		glPushMatrix();
		if(!testBuffer && !gamePaused)updatePlayer(&Player);
		i=(Player.position.x+(tilesize<<6)*SCALEFACTOR+(SUPERCLUSTERSIZE*(bsize*SCALEFACTOR))/2)/(bsize*SCALEFACTOR);
		j=(Player.position.y+(tilesize<<6)*SCALEFACTOR+(SUPERCLUSTERSIZE*(bsize*SCALEFACTOR))/2)/(bsize*SCALEFACTOR);
		k=(Player.position.z+(tilesize<<6)*SCALEFACTOR+(m->clusterSize.z*(bsize*SCALEFACTOR))/2)/(bsize*SCALEFACTOR);
		// the culling indexes its tables with these: keep them inside the loaded area
		if(i<0)i=0; else if(i>=SUPERCLUSTERSIZE)i=SUPERCLUSTERSIZE-1;
		if(j<0)j=0; else if(j>=SUPERCLUSTERSIZE)j=SUPERCLUSTERSIZE-1;
		if(k<0)k=0; else if(k>=m->clusterSize.z)k=m->clusterSize.z-1;
		#ifdef DEBUGMODE
		iprintf("\ncluster : %d, %d, %d", i+m->offset.x, j+m->offset.y, k);
		#endif
		playerCamera(&Player, true);
		glPushMatrix();
		glScalef32(inttof32(SCALEFACTOR),inttof32(SCALEFACTOR),inttof32(SCALEFACTOR));
		glTranslatef32(-(SUPERCLUSTERSIZE*CLUSTERSIZE*(tilesize2<<6))/2, -(SUPERCLUSTERSIZE*CLUSTERSIZE*(tilesize2<<6))/2,-(m->size.z*(tilesize2<<6))/2);
	PROF_START();
	if(cull)cullClusters(m, &openList, &closedList, i, j, k);
	PROF_END(time);
		glPopMatrix(1);
	if(testBuffer)
	{
		rt1++;
		rt1%=2;
		if(rt1)maT1=time;
		else miT1=time;
		#ifdef DEBUGMODE
		iprintf("\ntesting : %d,%d (%d)         ",maT1,miT1,cull);
		#endif
		{
			toProcess_struct* q=lightProcess.first;
			void** pq=&lightProcess.first;
			while(q)
			{
				if(q->i-m->offset.x*CLUSTERSIZE>CLUSTERSIZE && q->i-m->offset.x*CLUSTERSIZE<(SUPERCLUSTERSIZE-2)*CLUSTERSIZE
				&& q->j-m->offset.y*CLUSTERSIZE>CLUSTERSIZE && q->j-m->offset.y*CLUSTERSIZE<(SUPERCLUSTERSIZE-2)*CLUSTERSIZE)
				{
					vect3D clusterCoord=getCluster(m,q->i,q->j,q->k);
					processLight(m,q->i,q->j,q->k,clusterCoord);
					*pq=q->next;
					free(q);
					break;
				}
				pq=&q->next;
				q=q->next;
			}
		}
	}else{
		rt2++;
		rt2%=2;
		if(rt2)maT2=time;
		else miT2=time;
		#ifdef DEBUGMODE
		iprintf("\ntesting : %d,%d (%d)         ",maT2,miT2,cull);
		#endif
		{
			int i3, ma;
			PROF_START();
				processWater(m);
				if(waterCursor2>waterCount2)ma=WATERNUMBER2;
				else ma=waterCount2;
				for(i3=waterCursor2;i3<ma;i3++)
				{
					water_struct* w=&waterToSpread[i3];
					const u16 i=w->pos&8191, j=(w->pos>>13)&8191;
						if(i-m->offset.x*CLUSTERSIZE<WATERMAX
						&& j-m->offset.y*CLUSTERSIZE<WATERMAX
						&& i-m->offset.x*CLUSTERSIZE>WATERMIN
						&& j-m->offset.y*CLUSTERSIZE>WATERMIN)
						{
							addWater2(m,*w);
							*w=waterToSpread[waterCursor2++];
						}else if(i-m->offset.x*CLUSTERSIZE<-CLUSTERSIZE
							|| j-m->offset.y*CLUSTERSIZE<-CLUSTERSIZE
							|| i-m->offset.x*CLUSTERSIZE>(SUPERCLUSTERSIZE+1)*CLUSTERSIZE
							|| j-m->offset.y*CLUSTERSIZE>(SUPERCLUSTERSIZE+1)*CLUSTERSIZE){
							*w=waterToSpread[waterCursor2++];
						}
					if(i3==ma-1 && ma==WATERNUMBER2){i3=0;ma=waterCount2;}
				}
			PROF_END(TESTVALUE);
		}
	}
	#ifdef DEBUGMODE
	iprintf("\nquads : %d (%d)  ",testquads,testBuffer);
	iprintf("\nFS : %d %d %d   ",fsFormat,m->headerMap[0],m->fileMap[0]);
	#endif
}

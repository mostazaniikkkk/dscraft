#include "game/game_main.h"
#include <math.h>

// Rules for leaves, saplings and trees, ported from Minecraft Beta 1.7
// (BlockLeaves, BlockSapling, WorldGenTrees, WorldGenBigTree). They see the
// world through a plantWorld_struct, so they run on the host tests too.
// Minecraft's y is up; here k is up, so Java (x, y, z) is (i, k, j).

static inline int at(const plantWorld_struct* w, int i, int j, int k)
{
	if(k<0 || k>=w->height)return 0;      // Minecraft: nothing above or below the world
	return w->get(w->ctx,i,j,k);
}

/* ---------------------------------------------------------------------------
 * Leaf decay (BlockLeaves.updateTick)
 * ------------------------------------------------------------------------- */

#define LD 4                               // leaves survive up to 4 steps from a log
#define LS (2*LD+1)

bool plantsLeafSupported(const plantWorld_struct* w, int i, int j, int k)
{
	s8 c[LS*LS*LS];
	int x, y, z, step;
	for(z=0;z<LS;z++)for(y=0;y<LS;y++)for(x=0;x<LS;x++)
	{
		int t=at(w,i+x-LD,j+y-LD,k+z-LD);
		c[x+y*LS+z*LS*LS]=(t>0 && itemIsLog(t))?0:((t>0 && isLeaves(t))?-2:-1);
	}
	for(step=1;step<=LD;step++)
	{
		for(z=0;z<LS;z++)for(y=0;y<LS;y++)for(x=0;x<LS;x++)
		{
			s8* p=&c[x+y*LS+z*LS*LS];
			if(*p!=step-1)continue;
			if(x>0 && p[-1]==-2)p[-1]=step;
			if(x<LS-1 && p[1]==-2)p[1]=step;
			if(y>0 && p[-LS]==-2)p[-LS]=step;
			if(y<LS-1 && p[LS]==-2)p[LS]=step;
			if(z>0 && p[-LS*LS]==-2)p[-LS*LS]=step;
			if(z<LS-1 && p[LS*LS]==-2)p[LS*LS]=step;
		}
	}
	return c[LD+LD*LS+LD*LS*LS]>=0;
}

int plantsLeafDrops(u8* items)
{
	int n=0;
	if(!(rand()%20))items[n++]=ITEM_SAPLING;   // BlockLeaves.quantityDropped
	if(!(rand()%200))items[n++]=ITEM_APPLE;    // oak leaves, Minecraft 1.0
	return n;
}

void plantsLeafDropsAt(int i, int j, int k)
{
	u8 items[2];
	int n, c=plantsLeafDrops(items);
	for(n=0;n<c;n++)dropsSpawn(items[n],i,j,k);
}

/* ---------------------------------------------------------------------------
 * Light (the engine lights faces, not blocks: Minecraft's levels are
 * estimated from the column above and the light sources around)
 * ------------------------------------------------------------------------- */

static int opacity(int t)
{
	if(t<0)return 15;
	if(!t || t==12 || t==13 || isSapling(t) || isLadder(t) || isDoor(t))return 0;
	if(isLeaves(t))return 1;
	if(t>=WATERTYPE)return 3;
	return 15;
}

// World.calculateSkylightSubtracted, with the sun angle as the time of day
// (sunX 0: sunrise, 8192: noon, 24576: midnight)
int plantsSkyDarkness(int sunX)
{
	float f=(sunX&32767)/32768.0f-0.25f, g, d;
	if(f<0)f+=1;
	g=f;
	f=1.0f-(cosf(f*3.14159265f)+1.0f)/2.0f;   // getCelestialAngle
	f=g+(f-g)/3.0f;
	d=1.0f-(cosf(f*3.14159265f*2.0f)*2.0f+0.5f);
	if(d<0)d=0;
	if(d>1)d=1;
	return (int)(d*11.0f);
}

static int skyLight(const plantWorld_struct* w, int i, int j, int k)
{
	int s=15, kk;
	for(kk=k+1;kk<w->height;kk++)
	{
		s-=opacity(at(w,i,j,kk));
		if(s<=0)return 0;
	}
	return s;
}

bool plantsSeesSky(const plantWorld_struct* w, int i, int j, int k)
{
	int kk;
	for(kk=k+1;kk<w->height;kk++)if(opacity(at(w,i,j,kk)))return false;
	return true;
}

static int blockLight(const plantWorld_struct* w, int i, int j, int k)
{
	int di, dj, dk, best=0;
	for(dk=-7;dk<=7;dk++)for(dj=-7;dj<=7;dj++)for(di=-7;di<=7;di++)
	{
		int t=at(w,i+di,j+dj,k+dk), l;
		if(t<=0)continue;
		l=(t==13)?14:(isLitFurnace(t)?FURNACE_LIGHT:0);
		l-=abs(di)+abs(dj)+abs(dk);
		if(l>best)best=l;
	}
	return best;
}

int plantsLight(const plantWorld_struct* w, int i, int j, int k, int darkness)
{
	int s=skyLight(w,i,j,k)-darkness, b=blockLight(w,i,j,k);
	if(s<0)s=0;
	return s>b?s:b;
}

// BlockFlower.canBlockStay: grass or dirt below, and light 8 or the sky
bool plantsSaplingStays(const plantWorld_struct* w, int i, int j, int k)
{
	int below=at(w,i,j,k-1);
	if(below!=1 && below!=2)return false;
	return plantsLight(w,i,j,k,0)>=8 || plantsSeesSky(w,i,j,k);
}

/* ---------------------------------------------------------------------------
 * Trees
 * ------------------------------------------------------------------------- */

typedef struct
{
	const plantWorld_struct* w;
	treeBox_struct* out;
	u32 s;
}gen_struct;

static u32 grNext(gen_struct* g){ g->s^=g->s<<13; g->s^=g->s>>17; g->s^=g->s<<5; return g->s; }
static int grInt(gen_struct* g, int n){ return grNext(g)%n; }
static float grFloat(gen_struct* g){ return (grNext(g)>>8)/16777216.0f; }

// Java coordinates: x, y (up), z. The sapling's own cell counts as air,
// since Minecraft removes the sapling before growing the tree.
static int jget(gen_struct* g, int x, int y, int z)
{
	if(x==g->out->i && z==g->out->j && y==g->out->k)return 0;
	return at(g->w,x,z,y);
}

static void jset(gen_struct* g, int x, int y, int z, u8 t)
{
	u8* c;
	if(y<0 || y>=g->w->height)return;
	c=treeBoxCell(g->out,x,z,y);
	if(c)*c=t;
}

static inline bool airOrLeaves(int t){ return !t || (t>0 && isLeaves(t)); }

static void startBox(gen_struct* g, const plantWorld_struct* w, u32 seed, int i, int j, int k, treeBox_struct* out)
{
	g->w=w;
	g->out=out;
	g->s=seed?seed:0x9E3779B9;
	memset(out->cells,0,sizeof(out->cells));
	out->i=i; out->j=j; out->k=k;
}

// WorldGenTrees: a trunk of 4..6 logs under four layers of leaves
bool plantsGrowSmallTree(const plantWorld_struct* w, u32 seed, int i, int j, int k, treeBox_struct* out)
{
	gen_struct g;
	int x=i, y=k, z=j, l, y1, x1, z1, b;
	startBox(&g,w,seed,i,j,k,out);
	l=grInt(&g,3)+4;
	if(y<1 || y+l+1>w->height)return false;
	for(y1=y;y1<=y+1+l;y1++)
	{
		b=1;
		if(y1==y)b=0;
		if(y1>=y+1+l-2)b=2;
		for(x1=x-b;x1<=x+b;x1++)for(z1=z-b;z1<=z+b;z1++)
		{
			if(y1<0 || y1>=w->height)return false;
			if(!airOrLeaves(jget(&g,x1,y1,z1)))return false;
		}
	}
	b=jget(&g,x,y-1,z);
	if((b!=1 && b!=2) || y>=w->height-l-1)return false;
	jset(&g,x,y-1,z,2);                                  // the grass under it becomes dirt
	for(y1=y-3+l;y1<=y+l;y1++)
	{
		int dy=y1-(y+l), r=1-dy/2;
		for(x1=x-r;x1<=x+r;x1++)
		{
			for(z1=z-r;z1<=z+r;z1++)
			{
				// corners: half of them, and none on the top layer
				if((abs(x1-x)!=r || abs(z1-z)!=r || (grInt(&g,2)!=0 && dy!=0)) && airOrLeaves(jget(&g,x1,y1,z1)))
					jset(&g,x1,y1,z1,LEAVES_BLOCK);
			}
		}
	}
	for(y1=0;y1<l;y1++)if(airOrLeaves(jget(&g,x,y+y1,z)))jset(&g,x,y+y1,z,ITEM_LOG);
	return true;
}

// WorldGenBigTree ------------------------------------------------------------

static const s8 otherCoord[6]={2,0,0,1,2,1};

typedef struct
{
	gen_struct g;
	int base[3];
	int heightLimit, height;
	int (*nodes)[4];
	int nodeCount;
}big_struct;

static int floorD(double d){ int i=(int)d; return (d<i)?i-1:i; }

static int bget(big_struct* t, const int* p){ return jget(&t->g,p[0],p[1],p[2]); }

// -1 when the line from a to b is clear (air or leaves), or the distance to the first obstacle
static int checkLine(big_struct* t, const int* a, const int* b)
{
	int d[3], i=0, n, j, k, p[3];
	for(n=0;n<3;n++)
	{
		d[n]=b[n]-a[n];
		if(abs(d[n])>abs(d[i]))i=n;
	}
	if(!d[i])return -1;
	int b1=otherCoord[i], b2=otherCoord[i+3], s=d[i]>0?1:-1;
	double r1=(double)d[b1]/d[i], r2=(double)d[b2]/d[i];
	j=0;
	k=d[i]+s;
	while(j!=k)
	{
		p[i]=a[i]+j;
		p[b1]=floorD(a[b1]+j*r1);
		p[b2]=floorD(a[b2]+j*r2);
		if(!airOrLeaves(bget(t,p)))break;
		j+=s;
	}
	return (j==k)?-1:abs(j);
}

static void placeLine(big_struct* t, const int* a, const int* b, u8 type)
{
	int d[3], i=0, n, j, p[3];
	for(n=0;n<3;n++)
	{
		d[n]=b[n]-a[n];
		if(abs(d[n])>abs(d[i]))i=n;
	}
	if(!d[i])return;
	int b1=otherCoord[i], b2=otherCoord[i+3], s=d[i]>0?1:-1;
	double r1=(double)d[b1]/d[i], r2=(double)d[b2]/d[i];
	for(j=0;j!=d[i]+s;j+=s)
	{
		p[i]=floorD(a[i]+j+0.5);
		p[b1]=floorD(a[b1]+j*r1+0.5);
		p[b2]=floorD(a[b2]+j*r2+0.5);
		jset(&t->g,p[0],p[1],p[2],type);
	}
}

static float layerSize(big_struct* t, int i)
{
	float f, f1, f2;
	if((double)i<(float)t->heightLimit*0.3)return -1.618f;
	f=t->heightLimit/2.0f;
	f1=t->heightLimit/2.0f-i;
	if(f1==0)f2=f;
	else if(fabsf(f1)>=f)f2=0;
	else f2=sqrtf(f*f-f1*f1);
	return f2*0.5f;
}

static float leafSize(int i)
{
	if(i<0 || i>=4)return -1;
	return (i!=0 && i!=3)?3.0f:2.0f;
}

static void treeLayer(big_struct* t, int x, int y, int z, float f)
{
	int r=(int)(f+0.618), a, b, p[3];
	for(a=-r;a<=r;a++)for(b=-r;b<=r;b++)
	{
		double d=sqrt(pow(abs(a)+0.5,2)+pow(abs(b)+0.5,2));
		if(d>f)continue;
		p[0]=x+a; p[1]=y; p[2]=z+b;
		if(airOrLeaves(bget(t,p)))jset(&t->g,p[0],p[1],p[2],LEAVES_BLOCK);
	}
}

static bool bigLeafNodes(big_struct* t)
{
	int per, j, k, l, i1, n;
	t->height=(int)(t->heightLimit*0.618);
	if(t->height>=t->heightLimit)t->height=t->heightLimit-1;
	per=(int)(1.382+pow(t->heightLimit/13.0,2));
	if(per<1)per=1;
	t->nodes=malloc(sizeof(*t->nodes)*per*t->heightLimit);
	if(!t->nodes)return false;
	j=t->base[1]+t->heightLimit-4;
	k=1;
	l=t->base[1]+t->height;
	i1=j-t->base[1];
	t->nodes[0][0]=t->base[0]; t->nodes[0][1]=j; t->nodes[0][2]=t->base[2]; t->nodes[0][3]=l;
	j--;
	while(i1>=0)
	{
		float f=layerSize(t,i1);
		if(f>=0)for(n=0;n<per;n++)
		{
			double d1=f*(grFloat(&t->g)+0.328);
			double d2=grFloat(&t->g)*2*3.14159;
			int x=floorD(d1*sin(d2)+t->base[0]+0.5), z=floorD(d1*cos(d2)+t->base[2]+0.5);
			int a1[3]={x,j,z}, a2[3]={x,j+4,z}, a3[3]={t->base[0],t->base[1],t->base[2]};
			if(checkLine(t,a1,a2)!=-1)continue;
			double d4=sqrt(pow(abs(t->base[0]-x),2)+pow(abs(t->base[2]-z),2))*0.381;
			a3[1]=(a1[1]-d4>l)?l:(int)(a1[1]-d4);
			if(checkLine(t,a3,a1)==-1)
			{
				t->nodes[k][0]=x; t->nodes[k][1]=j; t->nodes[k][2]=z; t->nodes[k][3]=a3[1];
				k++;
			}
		}
		j--;
		i1--;
	}
	t->nodeCount=k;
	return true;
}

bool plantsGrowBigTree(const plantWorld_struct* w, u32 seed, int i, int j, int k, treeBox_struct* out)
{
	big_struct t;
	int n, y, top[3], b;
	startBox(&t.g,w,seed,i,j,k,out);
	t.base[0]=i; t.base[1]=k; t.base[2]=j;
	t.heightLimit=5+grInt(&t.g,12);
	// validTreeLocation
	b=jget(&t.g,t.base[0],t.base[1]-1,t.base[2]);
	if(b!=1 && b!=2)return false;
	top[0]=t.base[0]; top[1]=t.base[1]+t.heightLimit-1; top[2]=t.base[2];
	n=checkLine(&t,t.base,top);
	if(n!=-1)
	{
		if(n<6)return false;
		t.heightLimit=n;
	}
	if(!bigLeafNodes(&t))return false;
	for(n=0;n<t.nodeCount;n++)
		for(y=t.nodes[n][1];y<t.nodes[n][1]+4;y++)treeLayer(&t,t.nodes[n][0],y,t.nodes[n][2],leafSize(y-t.nodes[n][1]));
	top[1]=t.base[1]+t.height;
	placeLine(&t,t.base,top,ITEM_LOG);                 // trunk
	for(n=0;n<t.nodeCount;n++)                           // branches
	{
		int a[3]={t.base[0],t.nodes[n][3],t.base[2]}, c[3]={t.nodes[n][0],t.nodes[n][1],t.nodes[n][2]};
		if((double)(a[1]-t.base[1])>=t.heightLimit*0.2)placeLine(&t,a,c,ITEM_LOG);
	}
	free(t.nodes);
	return true;
}

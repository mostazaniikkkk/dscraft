/* Host test: generate a world file with source/game/worldgen.c.
   Usage: test_worldgen <output dir> [seed]   (prints the generated path) */
#include "game/game_main.h"
#include <time.h>

/* stands in for survivalFormatHeader: proves the generator calls it with the spawn set */
static void markHeader(u8* header)
{
	header_struct* h=(header_struct*)header;
	header[600]=(u8)(h->spawnX^0x5A);
}

static int seedTests(void)
{
	u32 v; int fail=0;
	#define SEED(text,ok,val) do{ v=0; if(worldgenSeedFromText(text,&v)!=(ok) || ((ok) && v!=(u32)(val))){ printf("FAIL seed \"%s\" -> %lu\n",text,(unsigned long)v); fail=1; } }while(0)
	SEED("",false,0);
	SEED("   ",false,0);
	SEED("12345",true,12345);
	SEED("-1",true,0xFFFFFFFFu);
	SEED("0",true,0);
	SEED("abc",true,96354);              /* Java "abc".hashCode() */
	SEED("hello",true,99162322);         /* Java "hello".hashCode() */
	SEED("12a",true,48736);              /* not a number: hashed */
	SEED("9999999999",true,(u32)9999999999LL^(u32)(9999999999LL>>32));
	if(!fail)printf("seed parsing OK\n");
	return fail;
}

int main(int argc, char** argv)
{
	if(argc>1 && !strcmp(argv[1],"--seeds"))return seedTests();
	char path[256];
	const char* dir=argc>1?argv[1]:".";
	u32 seed=argc>2?(u32)strtoul(argv[2],NULL,10):12345;
	int p, last=-1, steps=0;
	clock_t t0=clock();

	if(!worldgenNewPath(dir,argc>4?argv[4]:"world",path,sizeof(path))){printf("FAIL no free name\n");return 1;}
	if(!worldgenStart(path,seed,argc>3 && !strcmp(argv[3],"flat"),argc>3 && !strcmp(argv[3],"mark")?markHeader:NULL)){printf("FAIL start\n");return 1;}
	while((p=worldgenStep())>=0 && p<100)
	{
		if(p<last){printf("FAIL progress went from %d to %d\n",last,p);return 1;}
		last=p; steps++;
	}
	if(p!=100){printf("FAIL step returned %d\n",p);return 1;}
	steps++;
	if(steps!=WORLDGEN_COLUMNS*(3*WORLDGEN_COLUMNS+1)){printf("FAIL %d steps\n",steps);return 1;}
	if(worldgenStep()!=-1){printf("FAIL step after finishing\n");return 1;}

	/* the next free name must differ now that the file exists */
	{
		char next[256];
		if(!worldgenNewPath(dir,argc>4?argv[4]:"world",next,sizeof(next)) || !strcmp(next,path)){printf("FAIL name reuse\n");return 1;}
	}
	/* cancelling removes the partial file */
	{
		char tmp[256];
		worldgenNewPath(dir,"tmp",tmp,sizeof(tmp));
		worldgenStart(tmp,1,false,NULL); worldgenStep(); worldgenCancel();
		FILE* f=fopen(tmp,"rb");
		if(f){fclose(f);printf("FAIL partial file left behind\n");return 1;}
	}
	printf("%s\n",path);
	fprintf(stderr,"generated in %.2f s on the host\n",(double)(clock()-t0)/CLOCKS_PER_SEC);
	return 0;
}

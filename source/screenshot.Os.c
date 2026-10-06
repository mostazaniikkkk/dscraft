#include "common/general.h"
#include "common/bmp.h"
#include <sys/stat.h>

int scrnum;


void write16(u16* address, u16 value) {

	u8* first=(u8*)address;
	u8* second=first+1;

	*first=value&0xff;
	*second=value>>8;
}

void write32(u32* address, u32 value) {

	u8* first=(u8*)address;
	u8* second=first+1;
	u8* third=first+2;
	u8* fourth=first+3;

	*first=value&0xff;
	*second=(value>>8)&0xff;
	*third=(value>>16)&0xff;
	*fourth=(value>>24)&0xff;
}

void screenshotbmp2(const char* filename, u16* vram1, u16* vram2)//, u16 bgcolor)
{

	FILE* file=fopen(filename, "wb");
	if(!file)return;
	
	u8* temp=(u8*)malloc(sizeof(INFOHEADER)+sizeof(HEADER));
	if(!temp){fclose(file);return;}

	HEADER* header=(HEADER*)temp;
	INFOHEADER* infoheader=(INFOHEADER*)(temp+sizeof(HEADER));

	write16(&header->type, 0x4D42);
	write32(&header->size, 256*192*3+sizeof(INFOHEADER)+sizeof(HEADER));
	write32(&header->offset, sizeof(INFOHEADER)+sizeof(HEADER));
	write16(&header->reserved1, 0);
	write16(&header->reserved2, 0);

	write16(&infoheader->bits, 24);
	write32(&infoheader->size, sizeof(INFOHEADER));
	write32(&infoheader->compression, 0);
	write32(&infoheader->width, 256);
	write32(&infoheader->height, 192);
	write16(&infoheader->planes, 1);
	write32(&infoheader->imagesize, 256*192*3);
	write32(&infoheader->xresolution, 0);
	write32(&infoheader->yresolution, 0);
	write32(&infoheader->importantcolours, 0);
	write32(&infoheader->ncolours, 0);
	fwrite(temp, 1, sizeof(INFOHEADER)+sizeof(HEADER), file);
	int y,x;
	u8 line[256*3];
	for(y=0;y<192;y++)
	{
		for(x=0;x<256;x++)
		{
			u16 color=vram2[256*192-y*256+x];
			if(!((color>>15)&1)){color=vram1[256*192-y*256+x];}

			line[x*3+2]=(color&31)<<3;
			line[x*3+1]=((color>>5)&31)<<3;
			line[x*3+0]=((color>>10)&31)<<3;
		}
		fwrite(line,1,sizeof(line),file);   // a row at a time
	}

	DC_FlushAll();
	fclose(file);
	free(temp);
}

char bmpname[40];

void Debug_TakeScreenshotBMP(u16* vram1, u16* vram2)//, u16 bgcolor)
{
	// the folder may not exist yet
	#ifdef FATONLY
		mkdir("screens",0777);
	#else
		sprintf(bmpname,"%s/%s/screens",basePath,ROOT);
		mkdir(bmpname,0777);
	#endif
	scrnum++;
	#ifdef FATONLY
		if(scrnum<10)sprintf(bmpname, "screens/SCR_0000%d.bmp",scrnum);
		else if(scrnum<100)sprintf(bmpname, "screens/SCR_000%d.bmp",scrnum);
		else if(scrnum<1000)sprintf(bmpname, "screens/SCR_00%d.bmp",scrnum);
		else if(scrnum<10000)sprintf(bmpname, "screens/SCR_0%d.bmp",scrnum);
		else sprintf(bmpname, "screens/SCR_%d.bmp",scrnum);
	#else
		if(scrnum<10)sprintf(bmpname, "%s/%s/screens/SCR_0000%d.bmp",basePath,ROOT,scrnum);
		else if(scrnum<100)sprintf(bmpname, "%s/%s/screens/SCR_000%d.bmp",basePath,ROOT,scrnum);
		else if(scrnum<1000)sprintf(bmpname, "%s/%s/screens/SCR_00%d.bmp",basePath,ROOT,scrnum);
		else if(scrnum<10000)sprintf(bmpname, "%s/%s/screens/SCR_0%d.bmp",basePath,ROOT,scrnum);
		else sprintf(bmpname, "%s/%s/screens/SCR_%d.bmp",basePath,ROOT,scrnum);
	#endif
	while(!access(bmpname,R_OK))
	{
		scrnum++;
		#ifdef FATONLY
			if(scrnum<10)sprintf(bmpname, "screens/SCR_0000%d.bmp",scrnum);
			else if(scrnum<100)sprintf(bmpname, "screens/SCR_000%d.bmp",scrnum);
			else if(scrnum<1000)sprintf(bmpname, "screens/SCR_00%d.bmp",scrnum);
			else if(scrnum<10000)sprintf(bmpname, "screens/SCR_0%d.bmp",scrnum);
			else sprintf(bmpname, "screens/SCR_%d.bmp",scrnum);
		#else
		if(scrnum<10)sprintf(bmpname, "%s/%s/screens/SCR_0000%d.bmp",basePath,ROOT,scrnum);
		else if(scrnum<100)sprintf(bmpname, "%s/%s/screens/SCR_000%d.bmp",basePath,ROOT,scrnum);
		else if(scrnum<1000)sprintf(bmpname, "%s/%s/screens/SCR_00%d.bmp",basePath,ROOT,scrnum);
		else if(scrnum<10000)sprintf(bmpname, "%s/%s/screens/SCR_0%d.bmp",basePath,ROOT,scrnum);
		else sprintf(bmpname, "%s/%s/screens/SCR_%d.bmp",basePath,ROOT,scrnum);
		#endif
	}
	screenshotbmp2(bmpname, vram1, vram2);
}


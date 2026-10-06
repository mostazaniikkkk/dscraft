#include "game/game_main.h"

// Thin wrappers around libfat's internal file API. The map streaming code
// needs the on-disc sector of each map column so it can read and write raw
// sectors directly, bypassing the FAT layer once the file has been laid out.

struct _reent r;

static FILE_STRUCT* lastSeekFile;

FILE_STRUCT* sOpen(const char *path)
{
	FILE_STRUCT* f=malloc(sizeof(FILE_STRUCT));
	_FAT_open_r(&r, f, path, O_RDONLY, 0);
	lastSeekFile=f;
	return f;
}

ssize_t sRead(FILE_STRUCT* f, char *ptr, size_t len)
{
	return _FAT_read_r(&r, f, ptr, len);
}

off_t sSeek(FILE_STRUCT* f, u32 pos, int dir)
{
	lastSeekFile=f;
	return _FAT_seek_r(&r, f, (off_t)pos, dir);
}

// Position (cluster/sector/byte) of the last file passed to sSeek.
// The original build relied on a patched libfat that exported this.
FILE_POSITION _FAT_getPosition(u32* pos)
{
	if(pos)*pos=lastSeekFile->currentPosition;
	return lastSeekFile->rwPosition;
}

bool readSectors(u32 sector, u32 number, u8* buffer)
{
	if(!lastSeekFile)return false;
	return _FAT_disc_readSectors(lastSeekFile->partition->disc, sector, number, buffer);
}

bool writeSectors(u32 sector, u32 number, u8* buffer)
{
	if(!lastSeekFile)return false;
	return _FAT_disc_writeSectors(lastSeekFile->partition->disc, sector, number, buffer);
}

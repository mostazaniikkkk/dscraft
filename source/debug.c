#include "common/general.h"

void DS_Debug(char* string, ...)
{
}

size_t DS_UsedMem(void)
{
	return getMemUsed();
}

size_t DS_FreeMem(void)
{
	return getMemFree();
}

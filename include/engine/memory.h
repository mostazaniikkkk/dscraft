#ifndef __MEMORY9__
#define __MEMORY9__

#define MAX_MALLOC 512

void *DS_malloc_list[MAX_MALLOC];

void* DS_mAlloc(size_t size, DS_state* state);
void DS_freeState(DS_state* state);
void DS_InitMalloc();

#endif

#include "game/game_main.h"

#define RESERVED_SIZE_DISPLAY_LISTS   (1*1024) /*4 KB*/
u32 dl_displayLists[RESERVED_SIZE_DISPLAY_LISTS];
u32 dl_displayLists_filled = 0;

u32 dl_curdisplayList_filled_start;

u8 dl_commands_buffer[4];
u32 dl_commands_buffer_filled = 0;

u32 dl_attributes_buffer[8];
u32 dl_attributes_buffer_filled = 0;


void dl_packCommandsForDisplayList() {
    u32 i;

    while (dl_commands_buffer_filled < 4) {
        dl_commands_buffer[dl_commands_buffer_filled] = FIFO_NOP;
        dl_commands_buffer_filled++;
    }
    dl_displayLists[dl_displayLists_filled] = FIFO_COMMAND_PACK(dl_commands_buffer[0], dl_commands_buffer[1], dl_commands_buffer[2], dl_commands_buffer[3]);
    dl_displayLists_filled++;
    for (i=0; i<dl_attributes_buffer_filled; i++) {
        dl_displayLists[dl_displayLists_filled] = dl_attributes_buffer[i];
        dl_displayLists_filled++;
    }

    dl_commands_buffer_filled = 0;
    dl_attributes_buffer_filled = 0;

    if (dl_displayLists_filled > RESERVED_SIZE_DISPLAY_LISTS){NOGBA("overflow in model loading!\n");iprintf("overflow in model loading!\n");}
}


u32 glVertexPackedDL(u32 packed) {
    if (dl_attributes_buffer_filled > 2)
        dl_packCommandsForDisplayList();
    dl_commands_buffer[dl_commands_buffer_filled] = FIFO_VERTEX10;
    dl_commands_buffer_filled++;
    dl_attributes_buffer[dl_attributes_buffer_filled] = (u32) (packed);
    dl_attributes_buffer_filled++;
	u32 temp=dl_displayLists_filled+dl_commands_buffer_filled;
    if (dl_commands_buffer_filled == 4 || dl_attributes_buffer_filled == 4) dl_packCommandsForDisplayList();
	return temp;
}

u32 glBeginDL(u32 type) {
	dl_packCommandsForDisplayList();
	u32 r=(dl_displayLists_filled - dl_curdisplayList_filled_start) - 1;
    dl_commands_buffer[dl_commands_buffer_filled] = FIFO_BEGIN;
    dl_commands_buffer_filled++;
    dl_attributes_buffer[dl_attributes_buffer_filled] = type;
    dl_attributes_buffer_filled++;
    if (dl_commands_buffer_filled == 4 || dl_attributes_buffer_filled == 4) dl_packCommandsForDisplayList();
	return r;
}

u32* glBeginListDL() {
    dl_displayLists_filled = 0;
	dl_attributes_buffer_filled = 0;
	dl_commands_buffer_filled = 0;
    dl_curdisplayList_filled_start = dl_displayLists_filled;
    dl_displayLists_filled++; // current position will need to contain how much commands there are in the end, so we skip that one for now
    return (dl_displayLists + dl_curdisplayList_filled_start);
}

void glEndListDL() {
    dl_packCommandsForDisplayList();
    // start of the display list now needs to be updated as to how much commands it holds
    dl_displayLists[dl_curdisplayList_filled_start] = (dl_displayLists_filled - dl_curdisplayList_filled_start) - 1;
}

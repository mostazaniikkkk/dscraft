#ifndef _DISPLAYLISTLIB_H_
#define _DISPLAYLISTLIB_H_


u32 glVertexPackedDL(u32 packed);


u32 glBeginDL(u32 type);

u32* glBeginListDL();
void glEndListDL();

#endif

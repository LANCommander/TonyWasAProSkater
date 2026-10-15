/* Decompiled data: the retail executable's initialised globals that src/gamefmv/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: ushort*[11] */
PSX_DATA(0x800a6748) unsigned int *decodeSliceBuffer[6];

PSX_DATA(0x800a6760) int DAT_800a6760[5] = {
	5, 0x10a0005, 5, 0x1000005, 5,
};

PSX_DATA(0x800a6774) int D_800a6774[7] = {
	0x1020005, 10, 0xffff,
};

/* THPS2 demo: int */
PSX_DATA(0x800d13c8) int MIDILEVEL[1] = {
	0x4000,
};

/* THPS2 demo: null */
PSX_DATA(0x800d162c) int GameFMV_IsMovie[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1630) int GameFMV_RewindSwitch[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1638) int GameFMV_FrameBufferCopyActive[1];

/* THPS2 demo: int */
PSX_DATA(0x800d163c) int currentDecodeBuffer[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1640) int currentVlcBuffer[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1644) int sendVLC[1];

/* THPS2 demo: int */
PSX_DATA(0x800d164c) int alreadyHandledPause[1];

/* THPS2 demo: int */
PSX_DATA(0x800d165c) int GameFMV_StartSector[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1660) int GameFMV_EndFrame[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d1664) unsigned int GameFMV_VLCBufferSize[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d1668) unsigned int GameFMV_RingBufferSectors[1];

/* THPS2 demo: uchar */
PSX_DATA(0x800d166c) unsigned char GameFMV_SliceBuffers[2];

/* THPS2 demo: short */
PSX_DATA(0x800d166e) short GameFMV_StartY[1];

/* THPS2 demo: ushort */
PSX_DATA(0x800d1670) unsigned short GameFMV_EndX[1];

/* THPS2 demo: ushort */
PSX_DATA(0x800d1672) unsigned short GameFMV_Width[1];

/* THPS2 demo: ushort */
PSX_DATA(0x800d1674) unsigned short GameFMV_Height[2];

/* THPS2 demo: uchar */
PSX_DATA(0x800d167c) unsigned char GameFMV_Is24Bit[1];

/* THPS2 demo: uchar */
PSX_DATA(0x800d167d) unsigned char GameFMV_CurrentTrack[3];

/* RECT (THPS2 demo: RECT) */
PSX_DATA(0x800d1684) int decodeSlice[2];

/* THPS2 demo: uint*[2] */
PSX_DATA(0x800d168c) unsigned int *vlcBuffer[2];

/* Decompiled data: the retail executable's initialised globals that src/gamefmv/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"

/* THPS2 demo: ushort*[11] */
unsigned int *decodeSliceBuffer[6];

int DAT_800a6760[5] = {
	5, 0x10a0005, 5, 0x1000005, 5,
};

int D_800a6774[7] = {
	0x1020005, 10, 0xffff,
};

/* THPS2 demo: int */
int MIDILEVEL[1] = {
	0x4000,
};

/* THPS2 demo: null */
int GameFMV_IsMovie[1];

/* THPS2 demo: int */
int GameFMV_RewindSwitch[1];

/* THPS2 demo: int */
int GameFMV_FrameBufferCopyActive[1];

/* THPS2 demo: int */
int currentDecodeBuffer[1];

/* THPS2 demo: int */
int currentVlcBuffer[1];

/* THPS2 demo: int */
int sendVLC[1];

/* THPS2 demo: int */
int alreadyHandledPause[1];

/* THPS2 demo: int */
int GameFMV_StartSector[1];

/* THPS2 demo: int */
int GameFMV_EndFrame[1];

/* THPS2 demo: uint */
unsigned int GameFMV_VLCBufferSize[1];

/* THPS2 demo: uint */
unsigned int GameFMV_RingBufferSectors[1];

/* THPS2 demo: uchar */
unsigned char GameFMV_SliceBuffers[2];

/* THPS2 demo: short */
short GameFMV_StartY[1];

/* THPS2 demo: ushort */
unsigned short GameFMV_EndX[1];

/* THPS2 demo: ushort */
unsigned short GameFMV_Width[1];

/* THPS2 demo: ushort */
unsigned short GameFMV_Height[2];

/* THPS2 demo: uchar */
unsigned char GameFMV_Is24Bit[1];

/* THPS2 demo: uchar */
unsigned char GameFMV_CurrentTrack[3];

/* RECT (THPS2 demo: RECT) */
int decodeSlice[2];

/* THPS2 demo: uint*[2] */
unsigned int *vlcBuffer[2];

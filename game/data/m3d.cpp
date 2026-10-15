/* Decompiled data: the retail executable's initialised globals that src/m3d/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"

struct SCamera;

/* SVECTOR[10], then FrustumMatrix1 and FrustumMatrix2 (MATRIX): one SLights to the renderer
 * (M3d_Render, src/m3d.cpp) (THPS2 demo: SVECTOR[10], MATRIX, MATRIX) */
int FrustumNormals[36];

/* THPS2 demo: int */
int CurrentWaterLevel[1] = {
	0x7fffffff,
};

/* THPS2 demo: uchar */
unsigned char CurrentObjFileRegion[1];

/* THPS2 demo: short */
short M3d_OTPushback2[1];

/* THPS2 demo: uint */
unsigned int FrameNo[1];

/* THPS2 demo: uint */
unsigned int XblanksThen[1];

/* THPS2 demo: int */
int M3d_WireframeMode[1];

/* THPS2 demo: int */
int DpqMax[1];

/* THPS2 demo: int */
int DpqMin[1];

/* THPS2 demo: int */
int DpqShift[1];

/* THPS2 demo: uint */
unsigned int M3d_FadeColour[1];

/* THPS2 demo: SCamera* */
SCamera *pCurrentCamera;

/* SVECTOR (THPS2 demo: SVECTOR) */
int WaterNormal[2];

/* THPS2 demo: uint */
unsigned int Xres[1];

/* THPS2 demo: uint */
unsigned int Yres[1];

/* THPS2 demo: int */
int PixelAspectX[1];

/* THPS2 demo: int */
int PixelAspectY[1];

/* THPS2 demo: uint* */
unsigned int *pColourTable;

/* THPS2 demo: uint */
unsigned int RefMapClut[1];

/* THPS2 demo: uint */
unsigned int RefMapTpage[1];

/* THPS2 demo: uint */
unsigned int RefMapU0V0[1];

/* THPS2 demo: uint */
unsigned int XblanksNow[1];

/* THPS2 demo: int */
int SubObjectMasked[1];

/* THPS2 demo: int */
int Fast[1];

/* THPS2 demo: int */
int Dewibble[1];

/* THPS2 demo: int */
int BasePhase[1];

int FUN_80071fa4[37] = {
	0x6d6d6f63, 0x6e6f, 2048, 2048, 0x71800, 0xa800, 0x3000, 0, -1, 0x74616b73, 0x745f65, 0x5b000,
	0xa800, 0x3000, 0x32000, 0x1800, -1, 0x63736b73, 0x745f6c68, 0, 0x5b000, 0xa800, 0x3000,
	0x47800, 0x7000, 0x7c800, -1, 0x61766b73, 0x745f736e, 0, 0x5b000, 0xa800, 0x3000, 0, 0x5f800,
	-1,
};

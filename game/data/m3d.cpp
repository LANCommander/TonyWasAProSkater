/* Decompiled data: the retail executable's initialised globals that src/m3d/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

struct SCamera;

/* SVECTOR[10], then FrustumMatrix1 and FrustumMatrix2 (MATRIX): one SLights to the renderer
 * (src/m3d/render.cpp) (THPS2 demo: SVECTOR[10], MATRIX, MATRIX) */
PSX_DATA(0x800c7c00) int FrustumNormals[36];

/* THPS2 demo: int */
PSX_DATA(0x800d16f0) int CurrentWaterLevel[1] = {
	0x7fffffff,
};

/* THPS2 demo: uchar */
PSX_DATA(0x800d1a14) unsigned char CurrentObjFileRegion[1];

/* THPS2 demo: short */
PSX_DATA(0x800d214e) short M3d_OTPushback2[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d2150) unsigned int FrameNo[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d2154) unsigned int XblanksThen[1];

/* THPS2 demo: int */
PSX_DATA(0x800d215c) int M3d_WireframeMode[1];

/* THPS2 demo: int */
PSX_DATA(0x800d2168) int DpqMax[1];

/* THPS2 demo: int */
PSX_DATA(0x800d216c) int DpqMin[1];

/* THPS2 demo: int */
PSX_DATA(0x800d2170) int DpqShift[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d2174) unsigned int M3d_FadeColour[1];

/* THPS2 demo: SCamera* */
PSX_DATA(0x800d217c) SCamera *pCurrentCamera;

/* SVECTOR (THPS2 demo: SVECTOR) */
PSX_DATA(0x800d2180) int WaterNormal[2];

/* THPS2 demo: uint */
PSX_DATA(0x800d2188) unsigned int Xres[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d218c) unsigned int Yres[1];

/* THPS2 demo: int */
PSX_DATA(0x800d2190) int PixelAspectX[1];

/* THPS2 demo: int */
PSX_DATA(0x800d2194) int PixelAspectY[1];

/* THPS2 demo: uint* */
PSX_DATA(0x800d219c) unsigned int *pColourTable;

/* THPS2 demo: uint */
PSX_DATA(0x800d21a4) unsigned int RefMapClut[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d21a8) unsigned int RefMapTpage[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d21ac) unsigned int RefMapU0V0[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d21b0) unsigned int XblanksNow[1];

/* THPS2 demo: int */
PSX_DATA(0x800d21b4) int SubObjectMasked[1];

/* THPS2 demo: int */
PSX_DATA(0x800d21bc) int Fast[1];

/* THPS2 demo: int */
PSX_DATA(0x800d21c0) int Dewibble[1];

/* THPS2 demo: int */
PSX_DATA(0x800d21c4) int BasePhase[1];

PSX_DATA(0x80071fa4) int FUN_80071fa4[37] = {
	0x6d6d6f63, 0x6e6f, 2048, 2048, 0x71800, 0xa800, 0x3000, 0, -1, 0x74616b73, 0x745f65, 0x5b000,
	0xa800, 0x3000, 0x32000, 0x1800, -1, 0x63736b73, 0x745f6c68, 0, 0x5b000, 0xa800, 0x3000,
	0x47800, 0x7000, 0x7c800, -1, 0x61766b73, 0x745f736e, 0, 0x5b000, 0xa800, 0x3000, 0, 0x5f800,
	-1,
};

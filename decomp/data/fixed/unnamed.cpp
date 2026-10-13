/* Decompiled data that src/unnamed/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char AI__4CCar[];
extern char Die__6CBaddy[];
extern char Display__15CTexturedRibbon[];
extern char FUN_8001664c[];
extern char FUN_8001f718[];
extern char FUN_8001f794[];
extern char FUN_8002a448[];
extern char FUN_8002a77c[];
extern char FUN_8002e290[];
extern char FUN_8002ee00[];
extern char FUN_8002ee64[];
extern char FUN_80043584[];
extern char FUN_80043cfc[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_8004eba0[];
extern char FUN_8004fdf4[];
extern char FUN_800620f0[];
extern char FUN_80066524[];
extern char FUN_80066558[];
extern char FUN_8006b630[];
extern char FUN_8006c470[];
extern char FUN_80077130[];
extern char FUN_800771c4[];
extern char FUN_800771f8[];
extern char FUN_8007d20c[];
extern char GetVariable__6CBaddyUs[];
extern char Move__11CFlatRipple[];
extern char Move__11CMotionBlur[];
extern char Move__11CSimpleAnim[];
extern char Move__11CSmokeTrail[];
extern char Move__11CWobblyGlow[];
extern char Move__15CSmokeGenerator[];
extern char Move__19CMotionBlurImmortal[];
extern char Move__5CFrag[];
extern char Move__9CGlassBit[];
extern char SetVariable__6CBaddyUs[];
extern char dtor_10CGPolyLine[] PSX_NAME("_._10CGPolyLine");
extern char dtor_11CMotionBlur[] PSX_NAME("_._11CMotionBlur");
extern char dtor_11CSimpleAnim[] PSX_NAME("_._11CSimpleAnim");
extern char dtor_11EraseSprite[] PSX_NAME("_._11EraseSprite");
extern char dtor_14VerticalDialog[] PSX_NAME("_._14VerticalDialog");
extern char dtor_15CNonRenderedBit[] PSX_NAME("_._15CNonRenderedBit");
extern char dtor_15CSmokeGenerator[] PSX_NAME("_._15CSmokeGenerator");
extern char dtor_16CLinked2EndedBit[] PSX_NAME("_._16CLinked2EndedBit");
extern char dtor_19CMotionBlurImmortal[] PSX_NAME("_._19CMotionBlurImmortal");
extern char dtor_4CBit[] PSX_NAME("_._4CBit");
extern char dtor_5CFrag[] PSX_NAME("_._5CFrag");
extern char dtor_5CRail[] PSX_NAME("_._5CRail");
extern char dtor_6CPixel[] PSX_NAME("_._6CPixel");
extern char dtor_6Sprite[] PSX_NAME("_._6Sprite");
extern char dtor_7CFT4Bit[] PSX_NAME("_._7CFT4Bit");
extern char dtor_8CFlatBit[] PSX_NAME("_._8CFlatBit");
extern char dtor_8CGenPoly[] PSX_NAME("_._8CGenPoly");
extern char dtor_8CQuadBit[] PSX_NAME("_._8CQuadBit");
extern char dtor_9CPolyLine[] PSX_NAME("_._9CPolyLine");
extern char draw__14VerticalDialog[];

PSX_FIXED(0x8009e538) int D_8009e538[4] = {
	0, 0, 0, (int)FUN_8001664c,
};

PSX_FIXED(0x8009f0b0) int __vt_6CLight[6] = {
	0, 0, 0, (int)FUN_8001f718, 0, (int)FUN_8001f794,
};

PSX_FIXED(0x8009f640) int D_8009f640[6] = {
	0, 0, 0, (int)dtor_15CSmokeGenerator, 0, (int)Move__15CSmokeGenerator,
};

PSX_FIXED(0x8009f658) int D_8009f658[6] = {
	0, 0, 0, (int)dtor_10CGPolyLine, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f670) int __vt_9CPolyLine[6] = {
	0, 0, 0, (int)dtor_9CPolyLine, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f688) int __vt_9CGlassBit[6] = {
	0, 0, 0, (int)FUN_8002a77c, 0, (int)Move__9CGlassBit,
};

PSX_FIXED(0x8009f6a0) int __vt_11CSmokeTrail[6] = {
	0, 0, 0, (int)FUN_8002a448, 0, (int)Move__11CSmokeTrail,
};

PSX_FIXED(0x8009f6e8) int __vt_15CTexturedRibbon[8] = {
	0, 0, 0, (int)FUN_8002e290, 0, (int)FUN_8002ee64, 0, (int)Display__15CTexturedRibbon,
};

PSX_FIXED(0x8009f728) int __vt_5CFrag[6] = {
	0, 0, 0, (int)dtor_5CFrag, 0, (int)Move__5CFrag,
};

PSX_FIXED(0x8009f740) int D_8009f740[6] = {
	0, 0, 0, (int)dtor_19CMotionBlurImmortal, 0, (int)Move__19CMotionBlurImmortal,
};

PSX_FIXED(0x8009f758) int D_8009f758[6] = {
	0, 0, 0, (int)dtor_11CMotionBlur, 0, (int)Move__11CMotionBlur,
};

PSX_FIXED(0x8009f770) int __vt_11CSimpleAnim[6] = {
	0, 0, 0, (int)dtor_11CSimpleAnim, 0, (int)Move__11CSimpleAnim,
};

PSX_FIXED(0x8009f7a0) int __vt_6CPixel[6] = {
	0, 0, 0, (int)dtor_6CPixel, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f7b8) int __vt_16CLinked2EndedBit[6] = {
	0, 0, 0, (int)dtor_16CLinked2EndedBit, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f7d0) int __vt_8CFlatBit[6] = {
	0, 0, 0, (int)dtor_8CFlatBit, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f7e8) int __vt_11CWobblyGlow[6] = {
	0, 0, 0, (int)FUN_8002ee00, 0, (int)Move__11CWobblyGlow,
};

PSX_FIXED(0x8009f830) int D_8009f830[6] = {
	0, 0, 0, (int)dtor_8CGenPoly, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f848) int __vt_8CQuadBit[6] = {
	0, 0, 0, (int)dtor_8CQuadBit, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f860) int __vt_7CFT4Bit[6] = {
	0, 0, 0, (int)dtor_7CFT4Bit, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f878) int __vt_15CNonRenderedBit[6] = {
	0, 0, 0, (int)dtor_15CNonRenderedBit, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f8b0) int __vt_4CBit[6] = {
	0, 0, 0, (int)dtor_4CBit, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x800a05f4) int __vt_5CItem[4] = {
	0, 0, 0, (int)FUN_80043584,
};

PSX_FIXED(0x800a1308) int __vt_11CFlatRipple[6] = {
	0, 0, 0, (int)FUN_8004eba0, 0, (int)Move__11CFlatRipple,
};

PSX_FIXED(0x800a1340) int __vt_5CRail[4] = {
	0, 0, 0, (int)dtor_5CRail,
};

PSX_FIXED(0x800a1350) int DAT_800a1350[4] = {
	0, 0, 0, (int)FUN_8004fdf4,
};

PSX_FIXED(0x800a2140) int __vt_14VerticalDialog[6] = {
	0, 0, 0, (int)dtor_14VerticalDialog, 0, (int)draw__14VerticalDialog,
};

PSX_FIXED(0x800a2170) int D_800a2170[4] = {
	0, 0, 0, (int)FUN_80066524,
};

PSX_FIXED(0x800a2180) int D_800a2180[4] = {
	0, 0, 0, (int)dtor_11EraseSprite,
};

PSX_FIXED(0x800a2190) int __vt_6Sprite[4] = {
	0, 0, 0, (int)dtor_6Sprite,
};

PSX_FIXED(0x800a21a0) int __vt_11SlicedImage[4] = {
	0, 0, 0, (int)FUN_800620f0,
};

PSX_FIXED(0x800a21b0) int DAT_800a21b0[4] = {
	0, 0, 0, (int)FUN_80066558,
};

PSX_FIXED(0x800a233c) int __vt_4CCar[22] = {
	0, 0, 0, (int)FUN_8006b630, 0, (int)Die__6CBaddy, 0, (int)AI__4CCar, 0, (int)FUN_800447e4, 0,
	(int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)FUN_800447d4, 0, (int)FUN_8006c470, 0,
	(int)SetVariable__6CBaddyUs, 0, (int)GetVariable__6CBaddyUs,
};

PSX_FIXED(0x800a3444) int D_800a3444[4] = {
	0, 0, 0, (int)FUN_80077130,
};

PSX_FIXED(0x800a3454) int D_800a3454[4] = {
	0, 0, 0, (int)FUN_800771c4,
};

PSX_FIXED(0x800a3464) int DAT_800a3464[4] = {
	0, 0, 0, (int)FUN_800771f8,
};

PSX_FIXED(0x800a35a4) int DAT_800a35a4[4] = {
	0, 0, 0, (int)FUN_8007d20c,
};

/* SDoubleBuffer[2] (THPS2 demo: SDoubleBuffer[2]) */
PSX_FIXED(0x800a891c) int DoubleBuffer[16470];

/* Decompiled data that src/bit/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char FUN_8002ee54[];
extern char FUN_8002ee5c[];
extern char FUN_8002ee64[];
extern char Move__10CRibbonBit[];
extern char Move__11CGrindSpark[];
extern char Move__6CSpark[];
extern char dtor_10CRibbonBit[] PSX_NAME("_._10CRibbonBit");
extern char dtor_11CGrindSpark[] PSX_NAME("_._11CGrindSpark");
extern char dtor_15CSpecialDisplay[] PSX_NAME("_._15CSpecialDisplay");
extern char dtor_5CGlow[] PSX_NAME("_._5CGlow");
extern char dtor_6CGLine[] PSX_NAME("_._6CGLine");
extern char dtor_6CSpark[] PSX_NAME("_._6CSpark");
extern char dtor_7CRibbon[] PSX_NAME("_._7CRibbon");
extern char dtor_9CChunkBit[] PSX_NAME("_._9CChunkBit");

PSX_FIXED(0x8009f610) int __vt_11CGrindSpark[6] = {
	0, 0, 0, (int)dtor_11CGrindSpark, 0, (int)Move__11CGrindSpark,
};

PSX_FIXED(0x8009f628) int D_8009f628[6] = {
	0, 0, 0, (int)dtor_6CGLine, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f6b8) int D_8009f6b8[6] = {
	0, 0, 0, (int)dtor_7CRibbon, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f6d0) int D_8009f6d0[6] = {
	0, 0, 0, (int)dtor_10CRibbonBit, 0, (int)Move__10CRibbonBit,
};

PSX_FIXED(0x8009f788) int __vt_6CSpark[6] = {
	0, 0, 0, (int)dtor_6CSpark, 0, (int)Move__6CSpark,
};

PSX_FIXED(0x8009f800) int D_8009f800[6] = {
	0, 0, 0, (int)dtor_5CGlow, 0, (int)FUN_8002ee54,
};

PSX_FIXED(0x8009f818) int D_8009f818[6] = {
	0, 0, 0, (int)dtor_9CChunkBit, 0, (int)FUN_8002ee64,
};

PSX_FIXED(0x8009f890) int D_8009f890[8] = {
	0, 0, 0, (int)dtor_15CSpecialDisplay, 0, (int)FUN_8002ee64, 0, (int)FUN_8002ee5c,
};

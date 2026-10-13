/* Decompiled data that src/platform/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char Die__6CBaddy[];
extern char ExecuteCommand__9CPlatformUs[];
extern char FUN_8003e390[];
extern char FUN_8003e934[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char GetVariable__9CPlatformUs[];
extern char SetVariable__9CPlatformUs[];
extern char dtor_9CPlatform[] PSX_NAME("_._9CPlatform");

PSX_FIXED(0x800a013c) int __vt_9CPlatform[22] = {
	0, 0, 0, (int)dtor_9CPlatform, 0, (int)Die__6CBaddy, 0, (int)FUN_8003e390, 0, (int)FUN_800447e4,
	0, (int)FUN_800447dc, 0, (int)FUN_8003e934, 0, (int)FUN_800447d4, 0,
	(int)ExecuteCommand__9CPlatformUs, 0, (int)SetVariable__9CPlatformUs, 0,
	(int)GetVariable__9CPlatformUs,
};

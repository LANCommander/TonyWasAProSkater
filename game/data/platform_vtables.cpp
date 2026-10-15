/* The vtables of the classes src/platform/ defines (native data, psxdata.h). */
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

PSX_DATA(0x800a013c) PsxVtEntry __vt_9CPlatform[11] = {
	{0, 0, 0},
	{0, 0, dtor_9CPlatform},
	{0, 0, Die__6CBaddy},
	{0, 0, FUN_8003e390},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_8003e934},
	{0, 0, FUN_800447d4},
	{0, 0, ExecuteCommand__9CPlatformUs},
	{0, 0, SetVariable__9CPlatformUs},
	{0, 0, GetVariable__9CPlatformUs},
};

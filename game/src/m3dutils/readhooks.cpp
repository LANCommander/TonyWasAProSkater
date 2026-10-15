/* M3dUtils_ReadHooksPacket__FP6CSuperPv (0x8008123c) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/m3d.h"
#include "game/ob.h"

extern SPSXRegion PSXRegion[];

void M3dUtils_ReadHooksPacket(CSuper *s, void *p) {
	PSXRegion[s->mRegion].pHooks = (SHook *)((char *)p + 4);
}

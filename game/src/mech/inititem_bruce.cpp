/* InitItem__6CBrucePc (0x80042988) */
/* C++ (MECH.CPP). CItem::InitItem, then reads this skater's hook points packet. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/m3d.h"
#include "game/bruce.h"

struct Message;

extern SHooksPacket SkaterHooks[];

void M3dUtils_ReadHooksPacket(CSuper *, void *);

void CBruce::InitItem(char *pName) {
	CItem::InitItem(pName);
	M3dUtils_ReadHooksPacket(this, &SkaterHooks[mSkaterIndex]);
}

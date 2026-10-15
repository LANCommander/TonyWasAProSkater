/* DetachOldEnviro__FUc (0x8006fbe4) */
/* C++ (SPOOL.CPP). Takes environment idx's items out of EnviroList, the chain of both
   environments' item lists (AttachNewEnviro puts the newer one first): the list then starts at
   the other environment, or the other's list ends where this one began. */
#include "game/m3d.h"

extern Item *EnviroList;
extern int EnvRegions[2];
extern uint *pTexWibPacket[2];

void DetachOldEnviro(uchar idx) {
	Item *other = 0;
	Item *cur;
	int o;

	pTexWibPacket[idx] = 0;
	cur = PSXRegionItems[EnvRegions[idx]];
	o = EnvRegions[idx ^ 1];

	if (o != -1)
		other = PSXRegionItems[o];

	if (!other) {
		EnviroList = 0;
		return;
	}

	if (cur == EnviroList) {
		EnviroList = other;
		return;
	}

	{
		Item *p = other;

		while (p->NextItem != cur)
			p = p->NextItem;

		p->NextItem = 0;
	}
}

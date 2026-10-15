/* GetCommandPoint__Fi (0x80044920) */
#include "game/trig.h"

short **OffsetList;
SCommandPoint *CommandPoints;

SCommandPoint *GetCommandPoint(int n) {
	SCommandPoint *p;

	if (n != 0xffff && *OffsetList[n] == 6) {
		for (p = CommandPoints; p; p = p->pNext)
			if (p->NodeIndex == n)
				return p;
	}

	return 0;
}

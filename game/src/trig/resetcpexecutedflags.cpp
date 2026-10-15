/* Trig_ResetCPExecutedFlags__Fi (0x800449cc) */
#include "game/trig.h"

SCommandPoint *CommandPoints;

void Trig_ResetCPExecutedFlags(int m) {
	SCommandPoint *p;

	for (p = CommandPoints; p; p = p->pNext)
		if ((p->Executed & m) && !(p->Collision & m))
			p->Executed &= ~m;
}

/* Trig_ResetCPCollisionFlags__Fi (0x80044994) */
#include "game/trig.h"

SCommandPoint *CommandPoints;

void Trig_ResetCPCollisionFlags(int m) {
	SCommandPoint *p;

	for (p = CommandPoints; p; p = p->pNext)
		p->Collision &= ~m;
}

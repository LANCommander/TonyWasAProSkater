/* Utils_LineOfSight__FRC7CVectorT0 (0x80048170) */
#include "game/colij.h"

extern int LineOfSightCheck;

void M3dColij_InitLineInfo(SLineInfo *li);

int Utils_LineOfSight(const CVector &a, const CVector &b) {
	SLineInfo li;

	li.StartCoords.vx = a.vx;
	li.StartCoords.vy = a.vy;
	li.StartCoords.vz = a.vz;
	li.EndCoords.vx = b.vx;
	li.EndCoords.vy = b.vy;
	li.EndCoords.vz = b.vz;
	LineOfSightCheck = 1;
	M3dColij_InitLineInfo(&li);
	M3dZone_LineToItemNotSCached(&li, 1);

	LineOfSightCheck = 0;
	return li.pItem == 0;
}

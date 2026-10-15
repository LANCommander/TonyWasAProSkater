/* Utils_GetGroundHeight__FRC7CVectorii (0x800481e8) */
#include "game/colij.h"

void M3dColij_InitLineInfo(SLineInfo *li);

int Utils_GetGroundHeight(const CVector &p, int up, int down) {
	SLineInfo li;

	li.StartCoords.vx = p.vx;
	li.StartCoords.vy = p.vy - (up << 12);
	li.StartCoords.vz = p.vz;
	li.EndCoords.vx = p.vx;
	li.EndCoords.vy = p.vy + (down << 12);
	li.EndCoords.vz = p.vz;
	M3dColij_InitLineInfo(&li);

	li.RecordTriggerZoneHits = 0;
	M3dZone_LineToItemNotSCached(&li, 1);

	if (li.pItem && li.Normal.vy < -3071)
		return li.Position.vy;

	return -1;
}

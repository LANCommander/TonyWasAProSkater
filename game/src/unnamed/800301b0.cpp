/* UberFrig__FP6CBruceG7CVectorPc (0x800301b0) */
#include "game/bruce.h"
#include "game/colij.h"

void M3dColij_InitLineInfo(SLineInfo *);

void UberFrig(CBruce *p, CVector a, char *msg) {
	SLineInfo li;

	if (p->m6DC == 0) {
		li.StartCoords.vx = a.vx;
		li.StartCoords.vy = a.vy;
		li.StartCoords.vz = a.vz;
		li.EndCoords = li.StartCoords;
		li.EndCoords.vy = li.EndCoords.vy + 0x1f40000;
		M3dColij_InitLineInfo(&li);
		M3dZone_LineToItemNotSCached(&li, 1);

		if (li.pItem) {
			p->CalculateDynamicLighting(li.pFace, li.pItem);
			return;
		}

		li.StartCoords.vx = a.vx;
		li.StartCoords.vy = a.vy;
		li.StartCoords.vz = a.vz;
		li.StartCoords.vy = a.vy + -0x12c000;
		li.EndCoords = li.StartCoords;
		li.EndCoords.vy = li.EndCoords.vy + 0x190000;
		M3dColij_InitLineInfo(&li);
		M3dZone_LineToItemNotSCached(&li, 1);

		if (li.pItem) {
			p->mPos.vx = li.Position.vx;
			p->mPos.vy = li.Position.vy;
			p->mPos.vz = li.Position.vz;
			p->mPos.vy = li.Position.vy + -0x1e000;
			p->mOldPos = p->mPos;
			return;
		}

		p->mPos = p->mSafePos;
		p->mOldPos = p->mPos;
		p->mVel.vx = -p->mVel.vz / 2;
		p->mVel.vz = p->mVel.vx / 2;
	}
}

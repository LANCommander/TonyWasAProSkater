/* Utils_SetVisibilityInBox__FRC7CVectorT0bT2 (0x80048484) */
#include "game/ob.h"
#include "game/bit.h"
#include "game/bruce.h"
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];
extern CItem *EnviroList;
extern CBody *PowerUpList;
extern CBody *SuspendedList;
extern CBody *EnvironmentalObjectList;
extern CBit *NonRenderedBitList;
extern CBruce *pCurrentPlayer;
extern CBruce *MechList[2];
extern int GNumberOfPlayers, GGame;

#define INSIDE(v, w, u)  (v >= t8 && t5 >= v && w >= t6 && t3 >= w && u >= t7 && t4 >= u)
#define OUTSIDE(v, w, u) ((v < t8 || t5 < v) && (w < t6 || t3 < w) && (u < t7 || t4 < u))
#define SET(o)           o->mFlags = (o->mFlags & t2) | t1

void Utils_SetVisibilityInBox(const CVector &a, const CVector &b, bool arg2, bool arg3) {
	int t8 = a.vx, t7 = a.vy, t6 = a.vz, t5 = b.vx, t4 = b.vy, t3 = b.vz;
	int t1, t2;
	CItem *o;
	CBit *q;

	if (pCurrentPlayer == 0 || GNumberOfPlayers == 1 || GGame == 7)
		t1 = 0x41;
	else {
		t1 = 1;

		if (pCurrentPlayer == MechList[0])
			t1 = 0x40;
	}

	t2 = ~t1;

	if (arg2)
		t1 = 0;
	else
		t2 = -1;

	o = EnviroList;

	if (o) {
		do {
			SPSXRegion *e = &PSXRegion[o->mRegion];

			if (e->Usable != 0) {
				int x = o->mPos.vx, y = o->mPos.vy, z = o->mPos.vz;

				if (arg3) {
					if (INSIDE(x, z, y))
						SET(o);
				} else {
					if (OUTSIDE(x, z, y))
						SET(o);
				}
			}

			o = o->mNextItem;
		} while (o);
	}

	o = PowerUpList;

	if (o) {
		do {
			if (arg3) {
				if (INSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			} else {
				if (OUTSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			}

			o = o->mNextItem;
		} while (o);
	}

	o = SuspendedList;

	if (o) {
		do {
			if (arg3) {
				if (INSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			} else {
				if (OUTSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			}

			o = o->mNextItem;
		} while (o);
	}

	o = EnvironmentalObjectList;

	if (o) {
		do {
			if (arg3) {
				if (INSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			} else {
				if (OUTSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			}

			o = o->mNextItem;
		} while (o);
	}

	q = NonRenderedBitList;

	if (q) {
		do {
			if (q->mType == 2) {
				if (arg3) {
					if (INSIDE(q->mPos.vx, q->mPos.vz, q->mPos.vy)) {
						if (arg2)
							((CLight *)q)->mOn = 1;
						else if (GNumberOfPlayers == 1)
							((CLight *)q)->mOn = 0;
					}
				} else {
					if (OUTSIDE(q->mPos.vx, q->mPos.vz, q->mPos.vy)) {
						if (arg2)
							((CLight *)q)->mOn = 1;
						else if (GNumberOfPlayers == 1)
							((CLight *)q)->mOn = 0;
					}
				}
			}

			q = q->mNext;
		} while (q);
	}
}

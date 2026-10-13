/* CheckSide__6CBrucei (0x800343f0) */
/* C++ (PHYSICS.CPP). Side collision probe: casts a line from 30 units up the skater along mRight
   (`dir` = -1/+1 picks the side; 5 units per step, 35 in the air). If it hits a wall-like face
   (|normal . mUp| < 1023), the skater is put back at mOldPos; a fast hit roughly head-on (normal .
   mFront >= 2897, on the ground, no trick) plays the wall-bump anim and grunts. The heading and
   velocity are reflected off the face, then a second probe along the normal moves the skater to
   its end if clear (returns 1) or reports 0. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/colij.h"
#include "game/m3d.h"
#include "game/sfx.h"
#include "game/vector.h"

void M3dColij_InitLineInfo(SLineInfo *);
int GetFaceFlags(SLineInfo *);

#define ST(d, b, q, t)                                                                             \
	q = (t);                                                                                       \
	d = b + (mInAir ? q * 35 : q * 5) * 2;

int CBruce::CheckSide(int a) {
	SLineInfo li;
	CVector d;
	CVector c;
	int s1, q0, q1, q2, q3, q4, q5;

	li.StartCoords.vx = mPos.vx + mUp.vx * 30;
	li.StartCoords.vy = mPos.vy + mUp.vy * 30;
	li.StartCoords.vz = mPos.vz + mUp.vz * 30;
	ST(li.EndCoords.vx, li.StartCoords.vx, q0, a * mRight.vx)
	ST(li.EndCoords.vy, li.StartCoords.vy, q1, a * mRight.vy)
	ST(li.EndCoords.vz, li.StartCoords.vz, q2, a * mRight.vz)
	M3dColij_InitLineInfo(&li);
	M3dZone_LineToItemNotSCached(&li, 1);
	GetFaceFlags(&li);

	if (li.pItem) {
		d.vx = li.Normal.vx;
		d.vy = li.Normal.vy;
		d.vz = li.Normal.vz;

		if ((unsigned)(Mips_CVecDotProd(&d, &mUp) + 1023) < 2047) {
			s1 = 0;
			mPos = mOldPos;
			c = mVel;

			if (mAnim != 12) {
				if (c.Length() >= 7) {
					if (!mInAir && !m368) {
						CVector e;

						e.vx = li.Normal.vx;
						e.vy = li.Normal.vy;
						e.vz = li.Normal.vz;

						if (Mips_CVecDotProd(&e, &mFront) >= 2897) {
							mAnimSpeed = 0x10000;
							Debug_RunAnim(12, 3787);

							s1 = 200;
							SFX_Grunt(this);
						}
					}
				}
			}

			ReflectHeadingVelocityInPlane(li.Normal, s1);

			li.StartCoords.vx = mPos.vx;
			li.StartCoords.vy = mPos.vy;
			li.StartCoords.vz = mPos.vz;
			ST(li.EndCoords.vx, li.StartCoords.vx, q3, li.Normal.vx)
			ST(li.EndCoords.vy, li.StartCoords.vy, q4, li.Normal.vy)
			ST(li.EndCoords.vz, li.StartCoords.vz, q5, li.Normal.vz)
			M3dColij_InitLineInfo(&li);
			M3dZone_LineToItemNotSCached(&li, 1);
			GetFaceFlags(&li);

			if (li.pItem)
				return 0;

			mPos.vx = li.EndCoords.vx;
			mPos.vy = li.EndCoords.vy;
			mPos.vz = li.EndCoords.vz;
			return 1;
		}
	}

	return 0;
}

/* M3dColij_LineToSphere__FRC7CVectorT0R7CVectorP5CBodyT3i (0x80080cf8) */
/* C++ (M3DCOLIJ.CPP). Fuzzy demo twin (0.72); locals from its MAIN.SYM. Finds the first body in
   the list whose bounding sphere the segment OldPos->NewPos passes through: an AABB reject, then
   the perpendicular distance (cross product with the unit direction) against the radius, then the
   distance along the line, with the end caps checked against the end points. The nearest hit's
   distance is kept in NearestHit's best field, and ColijPos is set to the hit point. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}
#include "game/ob.h"
#include "game/colij.h"

extern SHit NearestHit;

CBody *pBodyHit;

static inline void SetSVector(SVECTOR *v, int _x, int _y, int _z) {
	v->vx = _x;
	v->vy = _y;
	v->vz = _z;
}

CBody *M3dColij_LineToSphere(const CVector &OldPos, const CVector &NewPos, CVector &ColijPos,
                             CBody *pBody, CBody *pIgnore, int ScaleFactor) {
	int Length;
	int DotProd;
	CVector DirectionVec;
	CVector MinCoords;
	CVector MaxCoords;
	CVector SquareVec;
	VECTOR OldToCentre;
	VECTOR NewToCentre;
	SVECTOR DirVecShort;

	pBodyHit = 0;
	NearestHit.Dist = 0x7fffffff;
	DirectionVec = (NewPos - OldPos) >> 12;
	gte_ldlvl(&DirectionVec);
	gte_sqr0();
	gte_stlvnl(&SquareVec);

	Length = SquareRoot0(SquareVec.vx + SquareVec.vy + SquareVec.vz);

	if (!Length)
		return 0;

	DirectionVec = (DirectionVec << 12) / Length;
	gte_ldopv1(&DirectionVec);
	SetSVector(&DirVecShort, DirectionVec.vx, DirectionVec.vy, DirectionVec.vz);
	gte_ldsvllrow0(&DirVecShort);

	if (OldPos.vx < NewPos.vx) {
		MinCoords.vx = OldPos.vx;
		MaxCoords.vx = NewPos.vx;
	} else {
		MinCoords.vx = NewPos.vx;
		MaxCoords.vx = OldPos.vx;
	}

	if (OldPos.vy < NewPos.vy) {
		MinCoords.vy = OldPos.vy;
		MaxCoords.vy = NewPos.vy;
	} else {
		MinCoords.vy = NewPos.vy;
		MaxCoords.vy = OldPos.vy;
	}

	if (OldPos.vz < NewPos.vz) {
		MinCoords.vz = OldPos.vz;
		MaxCoords.vz = NewPos.vz;
	} else {
		MinCoords.vz = NewPos.vz;
		MaxCoords.vz = OldPos.vz;
	}

	for (; pBody; pBody = (CBody *)pBody->mNextItem) {
		int Radius;
		int RadiusSquared;

		if (pBody->mCBodyFlags & 0x40)
			continue;

		if (pBody == pIgnore)
			continue;

		Radius = pBody->mRMinor << 12;

		if (pBody->mPos.vx + Radius < MinCoords.vx || MaxCoords.vx < pBody->mPos.vx - Radius)
			continue;

		if (pBody->mPos.vy + Radius < MinCoords.vy || MaxCoords.vy < pBody->mPos.vy - Radius)
			continue;

		if (pBody->mPos.vz + Radius < MinCoords.vz || MaxCoords.vz < pBody->mPos.vz - Radius)
			continue;

		*(CVector *)&OldToCentre = (pBody->mPos - OldPos) >> 12;
		gte_ldopv2(&OldToCentre);
		gte_op12();
		gte_sqr0();
		gte_stlvnl(&SquareVec);

		RadiusSquared = pBody->mRMinor * pBody->mRMinor;

		if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
			continue;

		gte_ldlv0(&OldToCentre);
		gte_llv0();
		gte_stlvnl0(&DotProd);

		if (DotProd >= NearestHit.Dist)
			continue;

		if (DotProd < 0) {
			gte_ldlvl(&OldToCentre);
			gte_sqr0();
			gte_stlvnl(&SquareVec);

			if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
				continue;
		} else if (DotProd > Length) {
			*(CVector *)&NewToCentre = (pBody->mPos - NewPos) >> 12;
			gte_ldlvl(&NewToCentre);
			gte_sqr0();
			gte_stlvnl(&SquareVec);

			if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
				continue;
		}

		pBodyHit = pBody;
		NearestHit.Dist = DotProd;
	}

	if (pBodyHit) {
		gte_ldlvl(&DirectionVec);
		gte_lddp(NearestHit.Dist);
		gte_gpf12();
		gte_stlvnl(&ColijPos);
		ColijPos <<= 12;
		ColijPos += OldPos;
	}

	return pBodyHit;
}

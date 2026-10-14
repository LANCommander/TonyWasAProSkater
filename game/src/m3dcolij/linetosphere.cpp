/* M3dColij_LineToSphere__FRC7CVectorT0R7CVectorP5CBodyT3i (0x80080cf8) */
/* C++ (M3DCOLIJ.CPP). Fuzzy demo twin (0.72); locals from its MAIN.SYM. Finds the first body in
   the list whose bounding sphere the segment OldPos->NewPos passes through: an AABB reject, then
   the perpendicular distance (cross product with the unit direction) against the radius, then the
   distance along the line, with the end caps checked against the end points. The nearest hit's
   distance is kept in NearestHit's best field, and ColijPos is set to the hit point. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
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
	geom_square(&DirectionVec, &SquareVec);

	Length = SquareRoot0(SquareVec.vx + SquareVec.vy + SquareVec.vz);

	if (!Length)
		return 0;

	DirectionVec = (DirectionVec << 12) / Length;
	/* the direction stays loaded for the loop, as in retail: on the rotation's diagonal (for the
	 * cross products) and as the light matrix's first row (the dot products) */
	geom_load_op_diag(&pc_geom, DirectionVec.vx, DirectionVec.vy, DirectionVec.vz);
	SetSVector(&DirVecShort, DirectionVec.vx, DirectionVec.vy, DirectionVec.vz);
	pc_geom.llm[0][0] = DirVecShort.vx;
	pc_geom.llm[0][1] = DirVecShort.vy;
	pc_geom.llm[0][2] = DirVecShort.vz;
	pc_geom.llm[1][0] = DirVecShort.pad;

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
		{
			/* the squares of direction x OldToCentre (16-bit components) */
			int16_t d[3], v[3];
			GeomAcc r;

			geom_op_diag(&pc_geom, d);
			geom_ld_lv(v, &OldToCentre);
			r = geom_op(d, v, 12, 0, 0);
			r = geom_sqr(r.ir, 0, 1, 0);
			geom_st_lv(&SquareVec, r.mac);
		}

		RadiusSquared = pBody->mRMinor * pBody->mRMinor;

		if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
			continue;

		{
			/* direction . OldToCentre */
			int16_t v[3];

			geom_ld_lv(v, &OldToCentre);
			DotProd = geom_mvmva(pc_geom.llm, 0, v[0], v[1], v[2], 12, 0, 0).mac[0];
		}

		if (DotProd >= NearestHit.Dist)
			continue;

		if (DotProd < 0) {
			geom_square(&OldToCentre, &SquareVec);

			if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
				continue;
		} else if (DotProd > Length) {
			*(CVector *)&NewToCentre = (pBody->mPos - NewPos) >> 12;
			geom_square(&NewToCentre, &SquareVec);

			if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
				continue;
		}

		pBodyHit = pBody;
		NearestHit.Dist = DotProd;
	}

	if (pBodyHit) {
		{
			/* direction * distance (16 bits each) */
			int16_t v[3];
			GeomAcc r;

			geom_ld_lv(v, &DirectionVec);
			r = geom_gpf((int16_t)NearestHit.Dist, v, 12, 0, 0, 0, 0);
			geom_st_lv(&ColijPos, r.mac);
		}
		ColijPos <<= 12;
		ColijPos += OldPos;
	}

	return pBodyHit;
}

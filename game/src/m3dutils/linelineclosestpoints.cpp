/* M3dUtils_LineLineClosestPoints__FP7CVectorN40PiT0T5 (0x80081778) */
/* C++ (M3DUTILS.CPP): build with CC1=D:/Tools/psyq4.5/CC1PLPSX.EXE. Locals and their order come from the demo's MAIN.SYM. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/m3dasm.h"
#include "game/vector.h"

/* a . b, unshifted (16 bits of each component) */
static int Dot0(const SVECTOR *a, const CVector *b) {
	int16_t m[3][3] = { { a->vx, a->vy, a->vz } }, v[3];

	geom_ld_lv(v, b);
	return geom_mvmva(m, 0, v[0], v[1], v[2], 0, 0, 0).mac[0];
}

/* start + d * t >> 12 (16 bits of d and t), into out */
static void Lerp(const CVector *start, const CVector *d, int t, CVector *out) {
	int32_t base[3];
	int16_t v[3];
	GeomAcc r;

	geom_ld_lv32(base, start);
	geom_ld_lv(v, d);
	r = geom_gpl((int16_t)t, v, base, 12, 0, 0, 0, 0);
	geom_st_lv(out, r.mac);
}

int M3dUtils_LineLineClosestPoints(CVector *L1Start, CVector *L1End, CVector *L2Start,
                                   CVector *L2End, CVector *PointL1, int *t1, CVector *PointL2,
                                   int *t2) {
	CVector DL1, DL2, DStart, L1StartInt, L2StartInt, V, SquaredVec, CrossProd, ApproachVec;
	SVECTOR VShifted;
	int Numerator, Denominator, Shift, ExtraShift;

	DL1 = (*L1End - *L1Start) >> 12;
	DL2 = (*L2End - *L2Start) >> 12;
	DStart = (*L1Start - *L2Start) >> 12;
	L1StartInt = *L1Start >> 12;
	L2StartInt = *L2Start >> 12;

	geom_cross(&DL1, &DL2, &V, 0);

	Shift = 0;

	while (V.vx > 26754 || V.vx < -26754 || V.vy > 26754 || V.vy < -26754 || V.vz > 26754 ||
	       V.vz < -26754) {
		V >>= 1;
		Shift++;
	}

	geom_square(&V, &SquaredVec);

	geom_cross(&DL2, &DStart, &CrossProd, 0);
	CrossProd >>= Shift;
	ExtraShift = 0;

	while (CrossProd.vx > 26754 || CrossProd.vx < -26754 || CrossProd.vy > 26754 ||
	       CrossProd.vy < -26754 || CrossProd.vz > 26754 || CrossProd.vz < -26754) {
		CrossProd >>= 1;
		ExtraShift++;
	}

	Denominator = (SquaredVec.vx + SquaredVec.vy + SquaredVec.vz) >> (ExtraShift * 2);
	VShifted.vx = V.vx >> ExtraShift;
	VShifted.vy = V.vy >> ExtraShift;
	VShifted.vz = V.vz >> ExtraShift;
	Numerator = Dot0(&VShifted, &CrossProd);

	if (Numerator < 0) {
		*t1 = 0;
		*PointL1 = *L1Start;
	} else if (Numerator >= Denominator) {
		*t1 = 4096;
		*PointL1 = *L1End;
	} else {
		if (Denominator > 0xfffff)
			*t1 = Numerator / (Denominator >> 12);
		else
			*t1 = (Numerator << 12) / Denominator;

		Lerp(&L1StartInt, &DL1, *t1, PointL1);
		*PointL1 <<= 12;
	}

	geom_cross(&DL1, &DStart, &CrossProd, 0);
	CrossProd >>= Shift;
	ExtraShift = 0;

	while (CrossProd.vx > 26754 || CrossProd.vx < -26754 || CrossProd.vy > 26754 ||
	       CrossProd.vy < -26754 || CrossProd.vz > 26754 || CrossProd.vz < -26754) {
		CrossProd >>= 1;
		ExtraShift++;
	}

	Denominator = (SquaredVec.vx + SquaredVec.vy + SquaredVec.vz) >> (ExtraShift * 2);
	VShifted.vx = V.vx >> ExtraShift;
	VShifted.vy = V.vy >> ExtraShift;
	VShifted.vz = V.vz >> ExtraShift;
	Numerator = Dot0(&VShifted, &CrossProd);

	if (Numerator < 0) {
		*t2 = 0;
		*PointL2 = *L2Start;
	} else if (Numerator >= Denominator) {
		*t2 = 4096;
		*PointL2 = *L2End;
	} else {
		if (Denominator > 0xfffff)
			*t2 = Numerator / (Denominator >> 12);
		else
			*t2 = (Numerator << 12) / Denominator;

		Lerp(&L2StartInt, &DL2, *t2, PointL2);
		*PointL2 <<= 12;
	}

	ApproachVec = (*PointL1 - *PointL2) >> 12;
	geom_square(&ApproachVec, &SquaredVec);
	return M3dMaths_SquareRoot0(SquaredVec.vx + SquaredVec.vy + SquaredVec.vz);
}

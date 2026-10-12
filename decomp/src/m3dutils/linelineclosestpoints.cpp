/* M3dUtils_LineLineClosestPoints__FP7CVectorN40PiT0T5 (0x80081778) */
/* C++ (M3DUTILS.CPP): build with CC1=D:/Tools/psyq4.5/CC1PLPSX.EXE. Locals and their order come from the demo's MAIN.SYM. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
#include <inline_c.h>
}
#include "game/vector.h"

extern "C" int FUN_8008b5dc(int);

/* MAC1-3 <- r0 (no INLINE_C.H macro for this) */
#define gte_ldlvnl(r0)                                                                             \
	__asm__ volatile("lwc2 $25, 0( %0 ); lwc2 $26, 4( %0 ); lwc2 $27, 8( %0 )" : : "r"(r0))

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

	gte_ldopv1(&DL1);
	gte_ldopv2(&DL2);
	gte_op0();
	gte_stlvnl(&V);

	Shift = 0;

	while (V.vx > 26754 || V.vx < -26754 || V.vy > 26754 || V.vy < -26754 || V.vz > 26754 ||
	       V.vz < -26754) {
		V >>= 1;
		Shift++;
	}

	gte_ldlvl(&V);
	gte_sqr0();
	gte_stlvnl(&SquaredVec);

	gte_ldopv1(&DL2);
	gte_ldopv2(&DStart);
	gte_op0();
	gte_stlvnl(&CrossProd);
	CrossProd >>= Shift;
	ExtraShift = 0;

	while (CrossProd.vx > 26754 || CrossProd.vx < -26754 || CrossProd.vy > 26754 ||
	       CrossProd.vy < -26754 || CrossProd.vz > 26754 || CrossProd.vz < -26754) {
		CrossProd >>= 1;
		ExtraShift++;
	}

	gte_ldlvl(&CrossProd);

	Denominator = (SquaredVec.vx + SquaredVec.vy + SquaredVec.vz) >> (ExtraShift * 2);
	VShifted.vx = V.vx >> ExtraShift;
	VShifted.vy = V.vy >> ExtraShift;
	VShifted.vz = V.vz >> ExtraShift;
	gte_ldsvrtrow0(&VShifted);
	gte_rtir_sf0();
	gte_stlvnl0(&Numerator);

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

		gte_ldlvnl(&L1StartInt);
		gte_ldlvl(&DL1);
		gte_lddp(*t1);
		gte_gpl12();
		gte_stlvnl(PointL1);
		*PointL1 <<= 12;
	}

	gte_ldopv1(&DL1);
	gte_ldopv2(&DStart);
	gte_op0();
	gte_stlvnl(&CrossProd);
	CrossProd >>= Shift;
	ExtraShift = 0;

	while (CrossProd.vx > 26754 || CrossProd.vx < -26754 || CrossProd.vy > 26754 ||
	       CrossProd.vy < -26754 || CrossProd.vz > 26754 || CrossProd.vz < -26754) {
		CrossProd >>= 1;
		ExtraShift++;
	}

	gte_ldlvl(&CrossProd);

	Denominator = (SquaredVec.vx + SquaredVec.vy + SquaredVec.vz) >> (ExtraShift * 2);
	VShifted.vx = V.vx >> ExtraShift;
	VShifted.vy = V.vy >> ExtraShift;
	VShifted.vz = V.vz >> ExtraShift;
	gte_ldsvrtrow0(&VShifted);
	gte_rtir_sf0();
	gte_stlvnl0(&Numerator);

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

		gte_ldlvnl(&L2StartInt);
		gte_ldlvl(&DL2);
		gte_lddp(*t2);
		gte_gpl12();
		gte_stlvnl(PointL2);
		*PointL2 <<= 12;
	}

	ApproachVec = (*PointL1 - *PointL2) >> 12;
	gte_ldlvl(&ApproachVec);
	gte_sqr0();
	gte_stlvnl(&SquaredVec);
	return FUN_8008b5dc(SquaredVec.vx + SquaredVec.vy + SquaredVec.vz);
}

/* M3DUTILS.CPP: M3D helpers: hook positions on animated super items, animation in-betweening,
   angles from a matrix, closest points of two lines and vector normalising. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/m3dutils.h"
#include "game/m3d.h"
#include "game/ob.h"
#include "m3dasm/lastvec.h"
#include "game/m3dasm.h"
#include "game/vector.h"
#include "game/decomp.h"
#include "game/globals.h"
#include "game/m3dinit.h"
#include "game/psyq.h"

void M3dUtils_InterpolateVectors(int, int, uint *, Item *, int, int);
void M3dAsm_TransformTwice(SVector *, SMatrix *, MATRIX *);

/* M3dUtils_ReadHooksPacket__FP6CSuperPv (0x8008123c) */
void M3dUtils_ReadHooksPacket(CSuper *s, void *p) {
	PSXRegion[s->mRegion].pHooks = (SHook *)((char *)p + 4);
}

/* M3dUtils_GetHookPosition__FP6VECTORP6CSuperi (0x8008125c) */
void M3dUtils_GetHookPosition(VECTOR *pResult, CSuper *pSuper, int Hook) {
	SMatrix *pAnimTransform;
	int Anim;
	int Part;
	uint *pAnimFile;
	SHook *pHook;

	pHook = PSXRegion[pSuper->mRegion].pHooks + Hook;
	Anim = pSuper->mAnim;
	Part = pHook->Part;
	pAnimFile = PSXRegion[pSuper->mRegion].pAnimFile;

	if (pSuper->mExtraFlags & 2) {
		pSuper->mTransform.m[0][0] = -pSuper->mTransform.m[0][0];
		pSuper->mTransform.m[1][0] = -pSuper->mTransform.m[1][0];
		pSuper->mTransform.m[2][0] = -pSuper->mTransform.m[2][0];
	}

	if (pSuper->mFlags & 4) {
		pAnimTransform = pSuper->mpPoseBuffer + Part;
	} else {
		ushort Interval = ((ushort *)pAnimFile)[Anim * 4 + 5];

		if (Interval) {
			M3dUtils_InterpolateVectors(4, Interval + 1, pAnimFile, (Item *)pSuper, Part,
			                            PSXRegion[pSuper->mRegion].NumParts);

			pAnimTransform = TweenBuffer + Part;
		} else {
			pAnimTransform = Decomp_GetAnimTransform(pSuper) + Part;
		}
	}

	M3dAsm_TransformTwice((SVector *)pHook, pAnimTransform, &pSuper->mTransform);
	M3dMaths_GetLastVecSums(pResult); /* TransformTwice leaves the point there */
	pResult->vx <<= 12;
	pResult->vy <<= 12;
	pResult->vz <<= 12;
	pResult->vx = pSuper->mPos.vx + ((pResult->vx - pSuper->mPos.vx) >> 4);
	pResult->vy = pSuper->mPos.vy + ((pResult->vy - pSuper->mPos.vy) >> 4);
	pResult->vz = pSuper->mPos.vz + ((pResult->vz - pSuper->mPos.vz) >> 4);

	if (pSuper->mExtraFlags & 2) {
		pSuper->mTransform.m[0][0] = -pSuper->mTransform.m[0][0];
		pSuper->mTransform.m[1][0] = -pSuper->mTransform.m[1][0];
		pSuper->mTransform.m[2][0] = -pSuper->mTransform.m[2][0];
	}
}

/* M3dUtils_InBetween__FP6CSuper (0x80081448) */
/* C++. Blends a super item's parts between two animation frames when its frame says it has
   in-betweens (the count at +0xa of the frame's 8-byte entry in the region's animation). */
void M3dUtils_InBetween(CSuper *s) {
	int i = s->mRegion;
	uint *f = PSXRegion[i].pAnimFile;
	int c = ((ushort *)(f + 2 * s->mAnim))[5];
	uint _t5683 = c + 1;

	if (c) {
		int n = PSXRegion[i].NumParts;

		M3dUtils_InterpolateVectors(4 * n, _t5683, f, (Item *)s, 0, n);
	}
}

/* M3dUtils_InterpolateVectors__FiiPUiP4Itemii (0x800814b0) */
void M3dUtils_InterpolateVectors(int NumVectors, int Interval, uint *pAnimFile, Item *pItem,
                                 int Part, int NumParts) {
	CSVector dAnim;
	SMatrix *pAnims;
	int Alpha;
	int Frame0;
	int Frame1;
	int Frame;
	int NumFrames;
	int i;
	int Anim;
	CSVector *pTween;
	CSVector *pAnim0;

	Frame = ((CSuper *)pItem)->mFrame;
	Frame0 = Frame - (Frame % Interval);
	Frame1 = Frame0 + Interval;
	Anim = ((CSuper *)pItem)->mAnim;
	pAnims = (SMatrix *)((char *)pAnimFile + pAnimFile[Anim * 2 + 1]);
	NumFrames = ((ushort *)pAnimFile)[Anim * 4 + 4];

	if (Frame1 >= NumFrames) {
		if (((CSuper *)pItem)->mAnimMode == 1) {
			Frame1 = 0;
			Alpha = ((Frame - Frame0) << 12) / (NumFrames - Frame0);
		} else {
			Frame0 -= Interval;

			if (Frame0 < 0)
				Frame0 = 0;

			Frame1 -= Interval;

			if (Frame1 == 0)
				Alpha = 0;
			else
				Alpha = ((Frame - Frame0) << 12) / (Frame1 - Frame0);
		}
	} else {
		Alpha = ((Frame - Frame0) << 12) / (Frame1 - Frame0);
	}

	pTween = (CSVector *)((char *)TweenBuffer + Part * 24);
	pAnim0 = (CSVector *)((char *)pAnims + (NumParts * Frame0 / Interval) * 24 + Part * 24);

	if (Alpha == 0) {
		for (i = 0; i < NumVectors; i++)
			*pTween++ = *pAnim0++;
	} else {
		CSVector *pAnim1 =
		    (CSVector *)((char *)pAnims + (NumParts * Frame1 / Interval) * 24 + Part * 24);
		for (i = 0; i < NumVectors; i++) {
			/* p0 + (p1 - p0) * Alpha (16-bit differences and weight), the sums' low halves */
			int32_t base[3];
			int16_t d[3];
			GeomAcc r;

			geom_load_vector(base, pAnim0->vx, pAnim0->vy, pAnim0->vz);
			d[0] = (int16_t)(pAnim1->vx - pAnim0->vx);
			d[1] = (int16_t)(pAnim1->vy - pAnim0->vy);
			d[2] = (int16_t)(pAnim1->vz - pAnim0->vz);
			r = geom_gpl((int16_t)Alpha, d, base, 12, 0, 0, 0, 0);

			pAnim0++;
			pAnim1++;
			pTween->vx = (short)r.mac[0];
			pTween->vy = (short)r.mac[1];
			pTween->vz = (short)r.mac[2];
			pTween++;
		}
	}
}

/* M3dUtils_AnglesFromMatrix__FP7SVECTORP6MATRIX (0x800816cc) */
void M3dUtils_AnglesFromMatrix(SVECTOR *out, MATRIX *m) {
	int y = m->m[0][2];
	int x = m->m[2][2];
	short s1 = m->m[1][0];
	short s2 = m->m[1][1];
	short s3 = m->m[1][2];
	int r = M3dMaths_SquareRoot0(y * y + x * x);

	out->vx = (short)ratan2(-(int)s3, r);
	out->vy = (short)ratan2(y, x);
	out->vz = (short)ratan2((int)s1, (int)s2);
}

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

/* M3dUtils_LineLineClosestPoints__FP7CVectorN40PiT0T5 (0x80081778) */
/* : build with CC1=D:/Tools/psyq4.5/CC1PLPSX.EXE. Locals and their order come from the demo's MAIN.SYM. */
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

/* M3dUtils_NormaliseVector__FP7CVector (0x80081f24) */
/* Shrink into GTE range, SQR+SquareRoot0, scale to 4096. */
void M3dUtils_NormaliseVector(CVector *v) {
	VECTOR t;
	int len;

	while (v->vx > 26754 || v->vx < -26754 || v->vy > 26754 || v->vy < -26754 || v->vz > 26754 ||
	       v->vz < -26754) {
		int one = 1;

		*v >>= one;
	}

	geom_square(v, &t);

	len = SquareRoot0(t.vx + t.vy + t.vz);

	if (len) {
		int x = (v->vx << 12) / len, y = (v->vy << 12) / len, z = (v->vz << 12) / len;

		v->vx = x;
		v->vy = y;
		v->vz = z;
	} else {
		v->vx = 4096;
		v->vy = 0;
		v->vz = 0;
	}
}

/* FUN_80082054 (0x80082054) */
void FUN_80082054(void) {
	int i, j;
	short *out;

	M3dInit_SetResolution(512, 240);
	M3dInit_SetFoggingParams(10, 6000, 2048);

	for (i = 0, out = WibbleTables[0]; i < 16; i++)
		for (j = 0; j < 4096; j += 64)
			*out++ = (rcossin_tbl[j].s * i) >> 4;

	M3d_FadeColour = 0x80000000;
}

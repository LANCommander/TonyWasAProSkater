/* EXP.CPP: hits on environment items and CFlatRipple, the expanding ring on water.
   Declarations in game/exp.h. */
#include "game/exp.h"
#include "game/m3d.h"
#include "game/bit.h"
#include "game/trig.h"
#include "game/bit2.h"
#include "game/globals.h"
#include "game/psyq.h"
#include "game/shatter.h"
#include "game/spool.h"
#include "game/utils.h"

extern "C" char __vt_11CFlatRipple[];
extern "C" int BASEF(CFlatRipple *p, int a);
extern "C" void __dl__4CBitPv(void *p);
struct VT {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short pad2;
	void (*fn)(char *, int);
};
struct O {
	struct VT *vt;
};

/* Exp_HitEnvItem__FP4ItemPUii (0x8004e968) */
void Exp_HitEnvItem(Item *pItem, uint *pFace, int Damage) {
	Item *pScan;
	SModel *pModel;

	if (!pItem)
		return;

	pScan = EnviroList;

	while (pScan) {
		if (pScan == pItem)
			break;

		pScan = pScan->NextItem;
	}

	if (!pScan)
		return;

	pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];

	if (pModel->Flags & 2) {
		uchar Life =
		    pModel->Flags >> 16; /* always 0: a life count read from bits ushort Flags lacks */

		if (Life || Damage == 0xffff)
			if (Damage >= Life)
				Shatter_Item(pItem, 1, 1);
	} else if (pFace) {
		Shatter_Face(pItem, pFace, 1, 1, 1, 1, 1);
	}
}

/* __11CFlatRippleRC7CVectoriUcUcUcii (0x8004ea54) */
/* A ripple: a semi-transparent poly-line ring of NumSegs segments, all at Pos in
   colour (r, g, b), that will grow at GrowRate up to MaxRadius from a random angle. */
CFlatRipple::CFlatRipple(const CVector &Pos, int MaxRadius, uchar r, uchar g, uchar b, int NumSegs,
                         int GrowRate)
    : CPolyLine(NumSegs) {
	mPos = Pos;
	mMaxRadius = MaxRadius;
	mGrowRate = GrowRate;
	mStart = Pos;
	mR = r;
	mG = g;
	mB = b;

	for (int i = 0; i < mNumSegs; i++) {
		mSegs[i].r = r;
		mSegs[i].g = g;
		mSegs[i].b = b;
		mSegs[i].End = Pos;
	}

	mAngle = Rnd(4096);
	SetSemiTransparent();
}

/* FUN_8004eba0 (0x8004eba0) */
/* CFlatRipple's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags). */
extern "C" void FUN_8004eba0(CFlatRipple *p, int f) {
	p->__vptr = __vt_11CFlatRipple;
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* Move__11CFlatRipple (0x8004ebf4) */
void CFlatRipple::Move() {
	int intensity;
	int a;
	int step;

	mRadius += mGrowRate;

	if (mRadius >= mMaxRadius) {
		mRadius = mMaxRadius;
		Die();
	}

	intensity = ((mMaxRadius - mRadius) << 8) / mMaxRadius;
	a = mAngle;
	step = 4096 / mNumSegs;
	SSinCos *e = &rcossin_tbl[a & 4095];

	mStart.vx = mPos.vx + (((mRadius * e->c) / 4096) << 12);
	mStart.vz = mPos.vz + (((mRadius * e->s) / 4096) << 12);

	for (int i = 0; i < mNumSegs; i++) {
		mSegs[i].r = (mR * intensity) >> 8;
		mSegs[i].g = (mG * intensity) >> 8;
		mSegs[i].b = (mB * intensity) >> 8;
		a += step;

		if (i == mNumSegs - 1) {
			mSegs[i].End = mStart;
		} else {
			SSinCos *f = &rcossin_tbl[a & 4095];

			mSegs[i].End.vx = mPos.vx + (((mRadius * f->c) / 4096) << 12);
			mSegs[i].End.vz = mPos.vz + (((mRadius * f->s) / 4096) << 12);
		}
	}
}

/* FUN_8004ee24 (0x8004ee24) */
void FUN_8004ee24(void) {
	struct O *p = (struct O *)TrickObList;
	struct O *q;

	if (p) {
		for (;;) {
			p->vt->fn((char *)p + p->vt->off, 3);

			q = (struct O *)TrickObList;

			if (!q)
				break;

			p = q;
		}
	}
}

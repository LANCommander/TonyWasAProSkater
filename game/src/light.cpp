/* LIGHT.CPP: CLight, a dynamic light that shows a wobbly glow while the skater is near it
   (declarations in game/light.h). */

#include "game/light.h"
#include "game/bruce.h"
#include "game/globals.h"

/* an egcs vtable entry (game/data/psxdata.h PsxVtEntry) */
struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};
extern "C" char __vt_6CLight[];
/* egcs-ABI calls, as retail makes them */
extern "C" void FUN_80027fa4(CBit *p, int a);
extern "C" void __dl__4CBitPv(void *p);
/* an egcs vtable: {delta, index, function} entries after an all-zero one */
struct VT {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short p2;
	void (*fn)(char *, int);
};
/* egcs-ABI calls, as retail makes them: CBit::operator new, CWobblyGlow::CWobblyGlow */
extern "C" char *__nw__4CBitUi(uint);
extern "C" CWobblyGlow *__11CWobblyGlowRC7CVectoriiiUcUcUcUcUcUc(char *, const CVector *, int, int,
                                                                  int, uchar, uchar, uchar, uchar,
                                                                  uchar, uchar);

/* __6CLightRC7CVectoriiiiiUcUcUcUcUcUc (0x8001f614) */
/* A dynamic light: a non-rendered bit at Pos with an inner/fringe radius, two colours
   and an amplitude; it starts switched on. */
CLight::CLight(const CVector &Pos, int Node, int ClipDist, int InnerRadius, int FringeRadius,
               int Amp, uchar r0, uchar g0, uchar b0, uchar r1, uchar g1, uchar b1) {
	mPos = Pos;
	mClipDist = ClipDist;
	mType = BITTYPE_LIGHT;
	mLifetime = Node; /* a light keeps its trigger node in mLifetime */
	mOn = 1;
	mInnerRadius = InnerRadius;
	mFringeRadius = FringeRadius;
	mr0 = r0;
	mg0 = g0;
	mb0 = b0;
	mr1 = r1;
	mg1 = g1;
	mb1 = b1;
	mAmp = Amp;
}

/* FUN_8001f718 (0x8001f718) */
/* CLight's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail has:
   deletes the light's glow through its vtable, then CNonRenderedBit's destructor. */
extern "C" void FUN_8001f718(CLight *p, int f) {
	CWobblyGlow *o = p->mpGlow;

	p->__vptr = __vt_6CLight;

	if (o) {
		struct VtEntry *vt = (struct VtEntry *)((CBit *)o)->__vptr;

		vt[1].fn((char *)o + vt[1].delta, 3);
	}

	FUN_80027fa4(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* FUN_8001f794 (0x8001f794) */
/* C++: CLight::Move (vtable slot 1), kept as the egcs-ABI function retail has. Within mClipDist
   of the first skater a switched-on light has a wobbly glow (made on demand, its angle following
   the distance); otherwise the glow is deleted through its vtable. */
extern "C" void FUN_8001f794(CLight *t) {
	int dx, dz, d;
	char *o;
	CWobblyGlow *ob;
	CBruce *cam;

	if (t->mOn) {
		cam = MechList;
		dx = (t->mPos.vx - cam->mPos.vx) >> 12;

		if (dx < 0)
			dx = -dx;

		dz = (t->mPos.vz - cam->mPos.vz) >> 12;

		if (dz < 0)
			dz = -dz;

		if (dx < dz)
			d = dz + dx / 2;
		else
			d = dx + dz / 2;

		if (d < t->mClipDist) {
			if (t->mpGlow == 0) {
				UseCacheForBits = 0;
				o = __nw__4CBitUi(sizeof(CWobblyGlow));

				if (o)
					o = (char *)__11CWobblyGlowRC7CVectoriiiUcUcUcUcUcUc(
					    o, &t->mPos, t->mInnerRadius, t->mFringeRadius, t->mAmp, t->mr0, t->mg0,
					    t->mb0, t->mr1, t->mg1, t->mb1);

				t->mpGlow = (CWobblyGlow *)o;
				UseCacheForBits = 1;
				((CWobblyGlow *)o)->mProtected = 1;
			}

			t->mpGlow->mAngle = (d * 5 * 2) / 16;
			return;
		}
	}

	ob = t->mpGlow;

	if (ob) {
		((struct VT *)ob->__vptr)->fn((char *)ob + ((struct VT *)ob->__vptr)->off, 3);

		t->mpGlow = 0;
	}
}

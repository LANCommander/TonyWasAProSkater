/* FUN_8001f794 (0x8001f794) */
/* C++: CLight::Move (vtable slot 1), kept as the egcs-ABI function retail has. Within mClipDist
   of the first skater a switched-on light has a wobbly glow (made on demand, its angle following
   the distance); otherwise the glow is deleted through its vtable. */
#include "game/bit.h"
#include "game/bruce.h"

/* an egcs vtable: {delta, index, function} entries after an all-zero one */
struct VT {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short p2;
	void (*fn)(char *, int);
};

extern CBruce *MechList;
extern int UseCacheForBits;

/* egcs-ABI calls, as retail makes them: CBit::operator new, CWobblyGlow::CWobblyGlow */
extern "C" char *__nw__4CBitUi(uint);
extern "C" CWobblyGlow *__11CWobblyGlowRC7CVectoriiiUcUcUcUcUcUc(char *, const CVector *, int, int,
                                                                  int, uchar, uchar, uchar, uchar,
                                                                  uchar, uchar);

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

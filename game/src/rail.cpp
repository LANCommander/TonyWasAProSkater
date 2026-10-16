/* RAIL.CPP: the grind rails, CRail, node runs with bounding boxes on RailList, and the skaters'
   rail pickup (declarations in game/rail.h). */
#include "game/rail.h"
#include "game/trickob.h"
#include "game/bruce.h"
#include "game/globals.h"
#include "game/m3dutils.h"
#include "game/trig.h"

/* the egcs vtable as RailList's deleting-destructor calls use it: slot 0's this-offset and function */
struct SRailVtable {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short pad2;
	void (*fn)(char *, int);
};
extern char __vt_5CRail[];
extern CRail *RailList;
/* CVector's out-of-line operators (game/vector.h), called as retail's egcs code calls them */
extern "C" {
int __eq__7CVectorRC7CVector(CVector *, CVector *);
CVector __mi__C7CVector(const CVector *);
void __mi__FRC7CVectorT0(CVector *, CVector *, CVector *);
}
extern char DAT_800a1350[];
void __dl__6CClassPv(void *p);

/* Rail_NextNode__FPUs (0x8004f578) */
int Rail_NextNode(unsigned short *p) {
	unsigned a;

	if (p[-1] == 11) {
		a = p[1];

		if (*OffsetList[a] == 10)
			return a;

		return p[2];
	}

	return p[1];
}

/* FUN_8004f5d0 (0x8004f5d0) */
void FUN_8004f5d0(void) {
	CRail *p = RailList;
	CRail *q;
	struct SRailVtable *vt;

	if (p) {
		for (;;) {
			vt = *(struct SRailVtable **)p;
			vt->fn((char *)p + vt->off, 3);

			q = RailList;

			if (!q)
				break;

			p = q;
		}
	}

	RailList = 0;
}

/* Rail_DeleteAll__Fv (0x8004f624) */
void Rail_DeleteAll(void) {
	CRail *p = RailList;
	CRail *n;
	struct SRailVtable *vt;

	while (p) {
		n = p->mpNext;

		if (p) {
			vt = *(struct SRailVtable **)p;
			vt->fn((char *)p + vt->off, 3);
		}

		p = n;
	}
}

/* FUN_8004f684 (0x8004f684) */
CRail *FUN_8004f684(CRail *p, int idx) {
	CVector a, b;
	short s;
	unsigned short *l;

	*(char **)p = __vt_5CRail;

	if (RailList)
		RailList->mpPrevious = p;

	p->mpNext = RailList;
	p->mpPrevious = 0;
	RailList = p;
	Trig_GetLinksPointer(idx);

	p->mStartNode = idx;
	/* the short at byte 6 of the node data past the position */
	s = ((short *)Trig_GetPosition(a, idx))[3];

	if (s == -1)
		p->mType = RAIL_TYPE_DEFAULT;
	else
		p->mType = (ERailType)s;

	idx = p->mStartNode;
	p->mBBoxMin.vz = 0x7fffffff;
	p->mBBoxMin.vy = 0x7fffffff;
	p->mBBoxMin.vx = 0x7fffffff;
	p->mBBoxMax.vz = 0x80000000;
	p->mBBoxMax.vy = 0x80000000;
	p->mBBoxMax.vx = 0x80000000;

	while (1) {
		Trig_GetPosition(b, idx);

		if (b.vx < p->mBBoxMin.vx)
			p->mBBoxMin.vx = b.vx;

		if (b.vy < p->mBBoxMin.vy)
			p->mBBoxMin.vy = b.vy;

		if (b.vz < p->mBBoxMin.vz)
			p->mBBoxMin.vz = b.vz;

		if (b.vx > p->mBBoxMax.vx)
			p->mBBoxMax.vx = b.vx;

		if (b.vy > p->mBBoxMax.vy)
			p->mBBoxMax.vy = b.vy;

		if (b.vz > p->mBBoxMax.vz)
			p->mBBoxMax.vz = b.vz;

		l = Trig_GetLinksPointer(idx);

		if (*l == 0)
			break;

		idx = Rail_NextNode(l);
	}

	return p;
}

/* _._5CRail (0x8004f814) */
/* Unlinks the rail from RailList. */
CRail::~CRail() {
	if (RailList == this)
		RailList = mpNext;

	if (mpPrevious)
		mpPrevious->mpNext = mpNext;

	if (mpNext)
		mpNext->mpPrevious = mpPrevious;
}

/* Rail_Update__Fv (0x8004f8a0) */
/* C++: the skaters (MechList) are CBruce objects and the rails CRail ones, whose fields move in a
   64-bit build. */
void Rail_Update() {
	CBruce *o;
	CRail *b;
	CVector cmin, cmax, pa, pb, v60, v70, v80, v90;
	int d, t;
	int ax, ay, az, bx, by, bz;
	CBruce *o2;
	int R;
	int s7, s6, flag, nxt;
	ushort *lp;

	o = MechList;

	if (o) {
		R = 0x64000;

		do {
			o2 = o;

			if (o->mpPad->Grind.Pressed == 0)
				o->m314 = 0;
			else if (o->m6DC == 0 && o->mInAir == 0 && o->m314 == 0) {
				ax = o->mOldPos.vx;
				ay = o->mOldPos.vy;
				az = o->mOldPos.vz;
				bx = o->mPos.vx;
				by = o->mPos.vy;
				bz = o->mPos.vz;

				if (bx < ax) {
					t = ax;
					ax = bx;
					bx = t;
				}

				if (by < ay) {
					t = ay;
					ay = by;
					by = t;
				}

				if (bz < az) {
					t = az;
					az = bz;
					bz = t;
				}

				ax -= 0x64000;
				bx += R;
				ay -= R;
				by += R;
				az -= R;
				bz += R;
				b = RailList;

				if (b) {
					do {
						if (bx >= b->mBBoxMin.vx && b->mBBoxMax.vx >= ax && by >= b->mBBoxMin.vy &&
						    b->mBBoxMax.vy >= ay && bz >= b->mBBoxMin.vz && b->mBBoxMax.vz >= az &&
						    (o->m70C <= 0 || o->m710 != b->mStartNode)) {
							s7 = b->mStartNode;

							while (*(lp = Trig_GetLinksPointer(s7))) {
								Trig_GetPosition(pa, s7);

								nxt = Rail_NextNode(lp);
								Trig_GetPosition(pb, nxt);

								cmin = pa;
								cmax = pb;

								if (cmin.vx > cmax.vx) {
									t = cmin.vx;
									cmin.vx = cmax.vx;
									cmax.vx = t;
								}

								if (cmin.vy > cmax.vy) {
									t = cmin.vy;
									cmin.vy = cmax.vy;
									cmax.vy = t;
								}

								if (cmin.vz > cmax.vz) {
									t = cmin.vz;
									cmin.vz = cmax.vz;
									cmax.vz = t;
								}

								if (bx >= cmin.vx && cmax.vx >= ax && by >= cmin.vy && cmax.vy >= ay &&
								    bz >= cmin.vz && cmax.vz >= az) {
									s6 = M3dUtils_LineLineClosestPoints(&o->mOldPos, &o->mPos, &pa, &pb,
									                                    &v60, &d, &v70, &t);

									if (s6 < o->m6F0 &&
									    ((o->mPhysicsState != 0 && o->mPhysicsState != 2) || v70.vy >= o->mPos.vy)) {
										__mi__FRC7CVectorT0(&v80, &pb, &pa);
										M3dUtils_NormaliseVector(&v80);

										flag = 0;

										if (Mips_CVecDotProd(&v80, &o->mVel) >= 0) {
											o2->mRailDir = v80;
											flag = __eq__7CVectorRC7CVector(&v70, &pb) != 0;
										} else {
											v90 = __mi__C7CVector(&v80);

											o2->mRailDir = v90;

											if (__eq__7CVectorRC7CVector(&v70, &pa) != 0)
												flag = 1;
										}

										if (flag == 0) {
											o->m6F0 = s6;
											o->mStickToRail = 1;
											o->mRailStart = b->mStartNode;
											o->mRailNode = s7;
											o2->mRailPos = v70;
											o->mRailType = b->mType;
										}
									}
								}

								s7 = nxt;
							}
						}

						b = b->mpNext;
					} while (b);
				}
			}

			o = (CBruce *)o->mNextItem;
		} while (o);
	}
}

/* FUN_8004fdf4 (0x8004fdf4) */
void FUN_8004fdf4(char *p, int f) {
	*(char **)(p + 0) = DAT_800a1350;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* Rail_Update__Fv (0x8004f8a0) */
/* C++: the skaters (MechList) are CBruce objects and the rails CRail ones, whose fields move in a
   64-bit build. */
#include "game/bruce.h"
#include "game/trickob.h"

extern CBruce *MechList;

CRail *RailList;

ushort *Trig_GetLinksPointer(int);
void Trig_GetPosition(CVector &, int);
int M3dUtils_LineLineClosestPoints(CVector *, CVector *, CVector *, CVector *, CVector *, int *,
                                   CVector *, int *);
void M3dUtils_NormaliseVector(CVector *);

/* CVector's out-of-line operators (game/vector.h), called as retail's egcs code calls them */
extern "C" {
int __eq__7CVectorRC7CVector(CVector *, CVector *);
CVector __mi__C7CVector(const CVector *);
void __mi__FRC7CVectorT0(CVector *, CVector *, CVector *);
}

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
											o->m210 = b->mNodeParam;
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

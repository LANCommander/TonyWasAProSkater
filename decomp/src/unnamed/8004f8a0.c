/* FUN_8004f8a0 (0x8004f8a0) */
#include "names.h"
typedef struct {
	int x, y, z;
} V;

typedef struct Obj {
	char p0[4];
	V bb;
	char p1[0x1c - 0x10];
	struct Obj *next;
	char p2[0x6c - 0x20];
	V f6c;
	char p3[0xf0 - 0x78];
	V ff0;
	char p4[0x210 - 0xfc];
	int f210;
	char p5[0x314 - 0x214];
	int f314;
	char p6[0x358 - 0x318];
	int f358;
	char p7[0x6dc - 0x35c];
	int f6dc;
	int f6e0;
	char p8[0x6e8 - 0x6e4];
	int f6e8;
	int f6ec;
	int f6f0;
	V f6f4;
	V f700;
	int f70c;
	int f710;
	char p9[0x734 - 0x714];
	int f734;
	char p10[0x86c - 0x738];
	char *f86c;
} Obj;

typedef struct Box {
	char p0[4];
	int id;
	V mn;
	V mx;
	char p1[0x24 - 0x20];
	struct Box *next;
	int f28;
} Box;

extern Obj *MechList;

Box *RailList;

unsigned short *Trig_GetLinksPointer(int);
void Trig_GetPosition(V *, int);
int FUN_8004f578(unsigned short *);
int Mips_CVecDotProd(V *, V *);
int M3dUtils_LineLineClosestPoints(V *, V *, V *, V *, V *, int *, V *, int *);
void M3dUtils_NormaliseVector(V *);
int __eq__7CVectorRC7CVector(V *, V *);
V __mi__C7CVector(const V *);
void __mi__FRC7CVectorT0(V *, V *, V *);

void FUN_8004f8a0(void) {
	Obj *o;
	Box *b;
	V cmin, cmax, pa, pb, v60, v70, v80, v90;
	int d, t;
	int ax, ay, az, bx, by, bz;
	Obj *o2;
	int R;
	int s7, s6, flag, nxt;
	unsigned short *lp;

	o = MechList;

	if (o) {
		R = 0x64000;

		do {
			o2 = o;

			if (*o->f86c == 0)
				o->f314 = 0;
			else if (o->f6dc == 0 && o->f358 == 0 && o->f314 == 0) {
				ax = o->ff0.x;
				ay = o->ff0.y;
				az = o->ff0.z;
				bx = o->bb.x;
				by = o->bb.y;
				bz = o->bb.z;

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
						if (bx >= b->mn.x && b->mx.x >= ax && by >= b->mn.y && b->mx.y >= ay &&
						    bz >= b->mn.z && b->mx.z >= az && (o->f70c <= 0 || o->f710 != b->id)) {
							s7 = b->id;

							while (*(lp = Trig_GetLinksPointer(s7))) {
								Trig_GetPosition(&pa, s7);

								nxt = FUN_8004f578(lp);
								Trig_GetPosition(&pb, nxt);

								cmin = pa;
								cmax = pb;

								if (cmin.x > cmax.x) {
									t = cmin.x;
									cmin.x = cmax.x;
									cmax.x = t;
								}

								if (cmin.y > cmax.y) {
									t = cmin.y;
									cmin.y = cmax.y;
									cmax.y = t;
								}

								if (cmin.z > cmax.z) {
									t = cmin.z;
									cmin.z = cmax.z;
									cmax.z = t;
								}

								if (bx >= cmin.x && cmax.x >= ax && by >= cmin.y && cmax.y >= ay &&
								    bz >= cmin.z && cmax.z >= az) {
									s6 = M3dUtils_LineLineClosestPoints(
									    &o->ff0, &o->bb, &pa, &pb, &v60, &d, &v70, &t);

									if (s6 < o->f6f0 &&
									    ((o->f734 != 0 && o->f734 != 2) || v70.y >= o->bb.y)) {
										__mi__FRC7CVectorT0(&v80, &pb, &pa);
										M3dUtils_NormaliseVector(&v80);

										flag = 0;

										if (Mips_CVecDotProd(&v80, &o->f6c) >= 0) {
											o2->f700 = v80;
											flag = __eq__7CVectorRC7CVector(&v70, &pb) != 0;
										} else {
											v90 = __mi__C7CVector(&v80);

											o2->f700 = v90;

											if (__eq__7CVectorRC7CVector(&v70, &pa) != 0)
												flag = 1;
										}

										if (flag == 0) {
											o->f6f0 = s6;
											o->f6e0 = 1;
											o->f6e8 = b->id;
											o->f6ec = s7;
											o2->f6f4 = v70;
											o->f210 = b->f28;
										}
									}
								}

								s7 = nxt;
							}
						}

						b = b->next;
					} while (b);
				}
			}

			o = o->next;
		} while (o);
	}
}

/* GameFMV_RetargetScreenFaces__FUsP7Texture (0x800237b4) */
/* C++: points the faces of the environment regions' and environmental objects' models that are
   textured from inside pTex (same tpage/clut, uvs within its box) at the FMV screen's tpage, with
   their uvs halved. The faces (model file data, 32-bit words) are walked by raw offsets: the
   textured face's uv/clut/tpage words at +16 (SFace::TexWord on), its size in words in bits 18+. */
#include "game/m3d.h"
#include "game/ob.h"

typedef unsigned char U8;

typedef struct {
	unsigned short f0, f2, f4, f6;
} H;

extern int EnvRegions[2];
extern SPSXRegion PSXRegion[];
extern CBody *EnvironmentalObjectList;

void GameFMV_RetargetScreenFaces(ushort arg0, Texture *a1) {
	unsigned short t9 = arg0;
	int i;
	int m, m2;
	CBody *o;

	i = 0;

	do {

		m = EnvRegions[i];

		if (m != -1) {
			SModel **lst;
			unsigned t8, t6;

			/* the port's model array has no count before it (retail's in-file table did) */
			lst = PSXRegion[m].ppModels;
			t8 = PSXRegion[m].NumModels;
			t6 = 0;

			if (t8 != 0) {
				SModel **t5;

				t5 = lst;

				do {
					char *t2, *t0;
					unsigned t3;
					unsigned short t4;

					t2 = (char *)*t5;
					t3 = 0;
					t0 = t2 + (((H *)t2)->f2 * 8 + 28) + ((H *)t2)->f4 * 8;
					t4 = ((H *)t2)->f6;

					if (t4 != 0) {
						do {
							unsigned t1;
							char *a2;

							t1 = *(unsigned *)t0;

							if ((t1 & 1) && (a2 = t0 + 16, (t1 & 2))) {
								if (*(unsigned short *)(a2 + 6) == a1->tpage &&
								    *(unsigned short *)(a2 + 2) == a1->clut &&
								    *(U8 *)(t0 + 16) >= a1->u0 &&
								    a1->u1 >= *(U8 *)(t0 + 16) &&
								    *(U8 *)(a2 + 4) >= a1->u0 &&
								    a1->u1 >= *(U8 *)(a2 + 4) &&
								    *(U8 *)(a2 + 8) >= a1->u0 &&
								    a1->u1 >= *(U8 *)(a2 + 8) &&
								    *(U8 *)(a2 + 1) >= a1->v0 &&
								    a1->v2 >= *(U8 *)(a2 + 1) &&
								    *(U8 *)(a2 + 5) >= a1->v0 &&
								    a1->v2 >= *(U8 *)(a2 + 5) &&
								    *(U8 *)(a2 + 9) >= a1->v0 &&
								    a1->v2 >= *(U8 *)(a2 + 9)) {
									*(unsigned short *)t2 |= 0x200;
									*(unsigned short *)(a2 + 6) = t9;
									*(U8 *)(t0 + 16) = *(U8 *)(t0 + 16) >> 1;
									*(U8 *)(a2 + 4) = *(U8 *)(a2 + 4) >> 1;
									*(U8 *)(a2 + 8) = *(U8 *)(a2 + 8) >> 1;
									*(U8 *)(a2 + 10) = *(U8 *)(a2 + 10) >> 1;
								}
							}

							t0 += (*(unsigned *)t0 >> 18) * 4;
							t3++;
						} while (t3 < t4);
					}

					t6++;
					t5++;
				} while (t6 < t8);
			}
		}

		i++;
	} while (i < 2);

	o = EnvironmentalObjectList;

	if (o) {
		do {

			m2 = o->mRegion;

			if (m2 != -1) {
				SModel **lst;
				unsigned t8, t6;

				lst = PSXRegion[m2].ppModels;
				t8 = PSXRegion[m2].NumModels;
				t6 = 0;

				if (t8 != 0) {
					SModel **t5;

					t5 = lst;

					do {
						char *t2, *t0;
						unsigned t3;
						unsigned short t4;

						t2 = (char *)*t5;
						t3 = 0;
						t0 = t2 + (((H *)t2)->f2 * 8 + 28) + ((H *)t2)->f4 * 8;
						t4 = ((H *)t2)->f6;

						if (t4 != 0) {
							do {
								unsigned t1;
								char *a2;

								t1 = *(unsigned *)t0;

								if ((t1 & 1) && (a2 = t0 + 16, (t1 & 2))) {
									if (*(unsigned short *)(a2 + 6) == a1->tpage &&
									    *(unsigned short *)(a2 + 2) == a1->clut &&
									    *(U8 *)(t0 + 16) >= a1->u0 &&
									    a1->u1 >= *(U8 *)(t0 + 16) &&
									    *(U8 *)(a2 + 4) >= a1->u0 &&
									    a1->u1 >= *(U8 *)(a2 + 4) &&
									    *(U8 *)(a2 + 8) >= a1->u0 &&
									    a1->u1 >= *(U8 *)(a2 + 8) &&
									    *(U8 *)(a2 + 10) >= a1->u0 &&
									    a1->u1 >= *(U8 *)(a2 + 10) &&
									    *(U8 *)(a2 + 1) >= a1->v0 &&
									    a1->v2 >= *(U8 *)(a2 + 1) &&
									    *(U8 *)(a2 + 5) >= a1->v0 &&
									    a1->v2 >= *(U8 *)(a2 + 5) &&
									    *(U8 *)(a2 + 9) >= a1->v0 &&
									    a1->v2 >= *(U8 *)(a2 + 9) &&
									    *(U8 *)(a2 + 11) >= a1->v0 &&
									    a1->v2 >= *(U8 *)(a2 + 11)) {
										*(unsigned short *)t2 |= 0x200;
										*(unsigned short *)(a2 + 6) = t9;
										*(U8 *)(t0 + 16) = *(U8 *)(t0 + 16) >> 1;
										*(U8 *)(a2 + 4) = *(U8 *)(a2 + 4) >> 1;
										*(U8 *)(a2 + 8) = *(U8 *)(a2 + 8) >> 1;
										*(U8 *)(a2 + 10) = *(U8 *)(a2 + 10) >> 1;
									}
								}

								t0 += (*(unsigned *)t0 >> 18) * 4;
								t3++;
							} while (t3 < t4);
						}

						t6++;
						t5++;
					} while (t6 < t8);
				}
			}

			o = (CBody *)o->mNextItem;
		} while (o);
	}
}

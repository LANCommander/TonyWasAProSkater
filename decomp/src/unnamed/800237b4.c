/* GameFMV_RetargetScreenFaces__FUsP7Texture (0x800237b4) */
#include "names.h"

typedef unsigned char U8;

typedef struct {
	unsigned short f0, f2, f4, f6;
} H;

extern int EnvRegions[2];
extern char *PSXRegion_10[][16];
extern char *EnvironmentalObjectList;

void GameFMV_RetargetScreenFaces(int arg0, char *a1) {
	unsigned short t9 = arg0;
	int i;
	int m, m2;
	char *o;

	i = 0;

	do {

		m = EnvRegions[i];

		if (m != -1) {
			char **lst;
			unsigned t8, t6;

			lst = (char **)PSXRegion_10[m][0];
			t8 = ((unsigned *)lst)[-1];
			t6 = 0;

			if (t8 != 0) {
				char **t5;

				t5 = lst;

				do {
					char *t2, *t0;
					unsigned t3;
					unsigned short t4;

					t2 = *t5;
					t3 = 0;
					t0 = t2 + (((H *)t2)->f2 * 8 + 28) + ((H *)t2)->f4 * 8;
					t4 = ((H *)t2)->f6;

					if (t4 != 0) {
						do {
							unsigned t1;
							char *a2;

							t1 = *(unsigned *)t0;

							if ((t1 & 1) && (a2 = t0 + 16, (t1 & 2))) {
								if (*(unsigned short *)(a2 + 6) == *(unsigned short *)(a1 + 6) &&
								    *(unsigned short *)(a2 + 2) == *(unsigned short *)(a1 + 2) &&
								    *(U8 *)(t0 + 16) >= *(U8 *)a1 &&
								    *(U8 *)(a1 + 4) >= *(U8 *)(t0 + 16) &&
								    *(U8 *)(a2 + 4) >= *(U8 *)a1 &&
								    *(U8 *)(a1 + 4) >= *(U8 *)(a2 + 4) &&
								    *(U8 *)(a2 + 8) >= *(U8 *)a1 &&
								    *(U8 *)(a1 + 4) >= *(U8 *)(a2 + 8) &&
								    *(U8 *)(a2 + 1) >= *(U8 *)(a1 + 1) &&
								    *(U8 *)(a1 + 9) >= *(U8 *)(a2 + 1) &&
								    *(U8 *)(a2 + 5) >= *(U8 *)(a1 + 1) &&
								    *(U8 *)(a1 + 9) >= *(U8 *)(a2 + 5) &&
								    *(U8 *)(a2 + 9) >= *(U8 *)(a1 + 1) &&
								    *(U8 *)(a1 + 9) >= *(U8 *)(a2 + 9)) {
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

			m2 = *(U8 *)(o + 27);

			if (m2 != -1) {
				char **lst;
				unsigned t8, t6;

				lst = (char **)PSXRegion_10[m2][0];
				t8 = ((unsigned *)lst)[-1];
				t6 = 0;

				if (t8 != 0) {
					char **t5;

					t5 = lst;

					do {
						char *t2, *t0;
						unsigned t3;
						unsigned short t4;

						t2 = *t5;
						t3 = 0;
						t0 = t2 + (((H *)t2)->f2 * 8 + 28) + ((H *)t2)->f4 * 8;
						t4 = ((H *)t2)->f6;

						if (t4 != 0) {
							do {
								unsigned t1;
								char *a2;

								t1 = *(unsigned *)t0;

								if ((t1 & 1) && (a2 = t0 + 16, (t1 & 2))) {
									if (*(unsigned short *)(a2 + 6) ==
									        *(unsigned short *)(a1 + 6) &&
									    *(unsigned short *)(a2 + 2) ==
									        *(unsigned short *)(a1 + 2) &&
									    *(U8 *)(t0 + 16) >= *(U8 *)a1 &&
									    *(U8 *)(a1 + 4) >= *(U8 *)(t0 + 16) &&
									    *(U8 *)(a2 + 4) >= *(U8 *)a1 &&
									    *(U8 *)(a1 + 4) >= *(U8 *)(a2 + 4) &&
									    *(U8 *)(a2 + 8) >= *(U8 *)a1 &&
									    *(U8 *)(a1 + 4) >= *(U8 *)(a2 + 8) &&
									    *(U8 *)(a2 + 10) >= *(U8 *)a1 &&
									    *(U8 *)(a1 + 4) >= *(U8 *)(a2 + 10) &&
									    *(U8 *)(a2 + 1) >= *(U8 *)(a1 + 1) &&
									    *(U8 *)(a1 + 9) >= *(U8 *)(a2 + 1) &&
									    *(U8 *)(a2 + 5) >= *(U8 *)(a1 + 1) &&
									    *(U8 *)(a1 + 9) >= *(U8 *)(a2 + 5) &&
									    *(U8 *)(a2 + 9) >= *(U8 *)(a1 + 1) &&
									    *(U8 *)(a1 + 9) >= *(U8 *)(a2 + 9) &&
									    *(U8 *)(a2 + 11) >= *(U8 *)(a1 + 1) &&
									    *(U8 *)(a1 + 9) >= *(U8 *)(a2 + 11)) {
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

			o = *(char **)(o + 28);
		} while (o);
	}
}

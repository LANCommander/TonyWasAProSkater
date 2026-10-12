/* FUN_8007b108 (0x8007b108) */
#include "names.h"
#include "game/padbuttons.h"
typedef struct Q {
	int *vp;
	int x, y, z, w;
} Q;

extern char Pad[];

unsigned short DAT_800d2068, DAT_800d206a;

extern char s_Pos_A[];
extern char s_Pos_B[];
extern int DAT_800a35c4[];

void CamPos_DisplayLabels(int, int, int, int, int);
void CamPos_Display(char *, int, int, int, int, int);
void Font_String(int, int, char *, int, int, int, int);
void QFromXRot(Q *, int);
void QFromYRot(Q *, int);
void QFromZRot(Q *, int);
void __ml__FRC5CQuatT0(Q *, Q *, Q *);

#define P Pad

void FUN_8007b108(char *a) {
	struct {
		int sv;
		int pad;
		Q r;
	} z;
	Q q1;
	Q q2;
	char *s1;
	char *s2;
	char *s4;

	CamPos_DisplayLabels(100, 50, 200, 200, 200);
	Font_String(200, 40, s_Pos_A, 200, 200, 200, 0);

	s4 = a + 0x138;
	CamPos_Display(s4, 200, 50, 80, 0, 0);
	Font_String(300, 40, s_Pos_B, 200, 200, 200, 0);

	s1 = a + 0x360;
	CamPos_Display(s1, 300, 50, 80, 0, 0);

	if (P[PAD_SIZE + PAD_RIGHT + PAD_TRIGGERED]) {
		switch (DAT_800d206a) {
			case 0:
				DAT_800d206a = 1;
				break;
			case 1:
				DAT_800d206a = 1;
				break;
		}

		P[PAD_SIZE + PAD_RIGHT + PAD_TRIGGERED] = 0;
	}

	if (P[PAD_SIZE + PAD_LEFT + PAD_TRIGGERED]) {
		switch (DAT_800d206a) {
			case 0:
				DAT_800d206a = 0;
				break;
			case 1:
				DAT_800d206a = 0;
				break;
		}

		P[PAD_SIZE + PAD_LEFT + PAD_TRIGGERED] = 0;
	}

	if (P[PAD_SIZE + PAD_DOWN + PAD_TRIGGERED]) {
		switch (DAT_800d2068) {
			case 0:
				DAT_800d2068 = 1;
				break;
			case 1:
				DAT_800d2068 = 2;
				break;
			case 2:
				DAT_800d2068 = 3;
				break;
			case 3:
				DAT_800d2068 = 5;
				break;
			case 5:
				DAT_800d2068 = 6;
				break;
			case 6:
				DAT_800d2068 = 8;
				break;
			case 8:
				DAT_800d2068 = 9;
				break;
			case 9:
				DAT_800d2068 = 10;
				break;
			case 10:
				DAT_800d2068 = 0;
				break;
		}

		P[PAD_SIZE + PAD_DOWN + PAD_TRIGGERED] = 0;
	}

	if (P[PAD_SIZE + PAD_UP + PAD_TRIGGERED]) {
		switch (DAT_800d2068) {
			case 0:
				DAT_800d2068 = 10;
				break;
			case 1:
				DAT_800d2068 = 0;
				break;
			case 2:
				DAT_800d2068 = 1;
				break;
			case 3:
				DAT_800d2068 = 2;
				break;
			case 5:
				DAT_800d2068 = 3;
				break;
			case 6:
				DAT_800d2068 = 5;
				break;
			case 8:
				DAT_800d2068 = 6;
				break;
			case 9:
				DAT_800d2068 = 8;
				break;
			case 10:
				DAT_800d2068 = 9;
				break;
		}

		P[PAD_SIZE + PAD_UP + PAD_TRIGGERED] = 0;
	}

	switch (DAT_800d206a) {
		case 0:
			s2 = s4;
			break;
		case 1:
			s2 = s1;
			break;
	}

	if (P[PAD_SIZE + PAD_GRIND + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5: {
				Q *pr;
				Q *pq;

				pq = &q1;
				z.sv = (int)((Q *)(s2 + 0x1c))->vp;
				pr = &z.r;
				QFromXRot(pq, 10);
				__ml__FRC5CQuatT0(pr, (Q *)(s2 + 0x1c), pq);

				*(Q *)(s2 + 0x1c) = z.r;
				((Q *)(s2 + 0x1c))->vp = (int *)z.sv;
				z.r.vp = DAT_800a35c4;
				q1.vp = DAT_800a35c4;
			} break;
			case 6:
				*(unsigned short *)(s2 + 0x3c) = *(unsigned short *)(s2 + 0x3c) - 10;
				break;
		}
	}

	if (P[PAD_SIZE + PAD_OLLIE + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5: {
				Q *pr;
				Q *pq;

				pq = &q2;
				z.sv = (int)((Q *)(s2 + 0x1c))->vp;
				pr = &z.r;
				QFromXRot(pq, -10);
				__ml__FRC5CQuatT0(pr, (Q *)(s2 + 0x1c), pq);

				*(Q *)(s2 + 0x1c) = z.r;
				((Q *)(s2 + 0x1c))->vp = (int *)z.sv;
				z.r.vp = DAT_800a35c4;
				q2.vp = DAT_800a35c4;
			} break;
			case 6:
				*(unsigned short *)(s2 + 0x3c) = *(unsigned short *)(s2 + 0x3c) + 10;
				break;
		}
	}

	if (P[PAD_SIZE + PAD_FLIP + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5: {
				Q *pr;
				Q *pq;

				pq = &q1;
				z.sv = (int)((Q *)(s2 + 0x1c))->vp;
				pr = &z.r;
				QFromYRot(pq, -10);
				__ml__FRC5CQuatT0(pr, (Q *)(s2 + 0x1c), pq);

				*(Q *)(s2 + 0x1c) = z.r;
				((Q *)(s2 + 0x1c))->vp = (int *)z.sv;
				z.r.vp = DAT_800a35c4;
				q1.vp = DAT_800a35c4;
			}
		}
	}

	if (P[PAD_SIZE + PAD_GRAB + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5: {
				Q *pr;
				Q *pq;

				pq = &q1;
				z.sv = (int)((Q *)(s2 + 0x1c))->vp;
				pr = &z.r;
				QFromYRot(pq, 10);
				__ml__FRC5CQuatT0(pr, (Q *)(s2 + 0x1c), pq);

				*(Q *)(s2 + 0x1c) = z.r;
				((Q *)(s2 + 0x1c))->vp = (int *)z.sv;
				z.r.vp = DAT_800a35c4;
				q1.vp = DAT_800a35c4;
			}
		}
	}

	if (P[PAD_SIZE + PAD_SPIN_RIGHT + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5: {
				Q *pr;
				Q *pq;

				pq = &q1;
				z.sv = (int)((Q *)(s2 + 0x1c))->vp;
				pr = &z.r;
				QFromZRot(pq, 10);
				__ml__FRC5CQuatT0(pr, (Q *)(s2 + 0x1c), pq);

				*(Q *)(s2 + 0x1c) = z.r;
				((Q *)(s2 + 0x1c))->vp = (int *)z.sv;
				z.r.vp = DAT_800a35c4;
				q1.vp = DAT_800a35c4;
			}
		}
	}

	if (P[PAD_SIZE + PAD_TAP_SPIN_RIGHT + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5: {
				Q *pr;
				Q *pq;

				pq = &q1;
				z.sv = (int)((Q *)(s2 + 0x1c))->vp;
				pr = &z.r;
				QFromZRot(pq, -10);
				__ml__FRC5CQuatT0(pr, (Q *)(s2 + 0x1c), pq);

				*(Q *)(s2 + 0x1c) = z.r;
				((Q *)(s2 + 0x1c))->vp = (int *)z.sv;
				z.r.vp = DAT_800a35c4;
				q1.vp = DAT_800a35c4;
			}
		}
	}
}

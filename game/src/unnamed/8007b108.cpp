/* FUN_8007b108 (0x8007b108) */
#include "game/padbuttons.h"
#include "game/camera.h"
#include "game/vector.h"

extern char Pad[];

extern unsigned short DAT_800d2068, DAT_800d206a;

void Font_String(ushort, ushort, char *, uchar, uchar, uchar, uchar);
CQuat QFromXRot(short);
CQuat QFromYRot(short);
CQuat QFromZRot(short);

/* the assignment as compiled here: x/y/z/w only, the destination keeps its vptr */
static inline void SetQuat(CQuat &d, const CQuat &q) {
	d.x = q.x;
	d.y = q.y;
	d.z = q.z;
	d.w = q.w;
}

#define P Pad

extern "C" void FUN_8007b108(CCamera *a) {
	CamPos *s1;
	CamPos *s2;
	CamPos *s4;

	CamPos::DisplayLabels(100, 50, 200, 200, 200);
	Font_String(200, 40, "Pos A", 200, 200, 200, 0);

	s4 = &a->mPosA;
	s4->Display(200, 50, 80, 0, 0);
	Font_String(300, 40, "Pos B", 200, 200, 200, 0);

	s1 = &a->mPosB;
	s1->Display(300, 50, 80, 0, 0);

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
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromXRot(10));
				break;
			case 6:
				s2->mStickLength -= 10;
				break;
		}
	}

	if (P[PAD_SIZE + PAD_OLLIE + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromXRot(-10));
				break;
			case 6:
				s2->mStickLength += 10;
				break;
		}
	}

	if (P[PAD_SIZE + PAD_FLIP + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromYRot(-10));
		}
	}

	if (P[PAD_SIZE + PAD_GRAB + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromYRot(10));
		}
	}

	if (P[PAD_SIZE + PAD_SPIN_RIGHT + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromZRot(10));
		}
	}

	if (P[PAD_SIZE + PAD_TAP_SPIN_RIGHT + PAD_PRESSED]) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromZRot(-10));
		}
	}
}

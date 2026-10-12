/* FUN_80070ff4 (0x80070ff4) */
#include "names.h"
typedef struct {
	char b[16];
} SButton;

typedef struct {
	SButton btn[16];
	char pad[0x48];
	signed char a148, a149, a14a, a14b;
	int f14c, f150;
	int pad154;
	unsigned short t158, t15a;
	char pad2[0x16c - 0x15c];
} SControl;

typedef struct {
	unsigned char b[32];
} Raw;

extern SControl Pad[];
extern Raw theControllers[];

int DAT_800d1f34, DAT_800d1f38, DAT_800d1f3c, DAT_800d1f40, DAT_800d1f44, DAT_800d1f48,
    DAT_800d1f4c, DAT_800d1f50;
int DAT_800d1f54, DAT_800d1f58, DAT_800d1f5c, DAT_800d1f60, Pad_IdleTime;
int DAT_800d23e4, DAT_800d23e8, DAT_800d23ec, DAT_800d23f0, DAT_800d23f4, DAT_800d23f8,
    DAT_800d23fc, DAT_800d2400;

void Pad_ActuatorOff(int, int);
void Pad_Button(SButton *, int);
void ReadControllers(void);
int FUN_80070f18(int, int, int, int);

#ifdef PC_PORT
/* Pad_Update on PC: every button slot of SControl is one action, and each frame it is fed straight
   from that action's bindings (platform/pc/src/pc_input.c). There is no PS1 button word and no
   analog stick: the pads report as digital (Type 0x41), or 0 when nothing is connected. */
#include "pc_input.h"

/* SControl's 20 button slots in order (game/padbuttons.h), and the action filling each */
static const signed char s_slotAction[20] = {
	PC_ACT_GRIND,     PC_ACT_FLIP,       PC_ACT_GRAB,        PC_ACT_OLLIE,
	PC_ACT_SPIN_LEFT, PC_ACT_TAP_SPIN_LEFT, PC_ACT_SPIN_RIGHT, PC_ACT_TAP_SPIN_RIGHT,
	PC_ACT_LEFT,      PC_ACT_RIGHT,      PC_ACT_UP,          PC_ACT_DOWN,
	PC_ACT_MENU_UP,   PC_ACT_MENU_DOWN,  PC_ACT_PAUSE,       PC_ACT_CAMERA,
	PC_ACT_MENU_LEFT, PC_ACT_MENU_RIGHT, PC_ACT_MENU_ACCEPT, PC_ACT_MENU_BACK,
};

int FUN_80070ff4(void) {
	int i, j, any = 0;

	for (i = 0; i < 2; i++) {
		if (Pad[i].t158 != 0) {
			if (--Pad[i].t158 == 0)
				Pad_ActuatorOff(i & 255, 0);
		}

		if (Pad[i].t15a != 0) {
			if (--Pad[i].t15a == 0)
				Pad_ActuatorOff(i & 255, 1);
		}
	}

	PC_InputUpdate();

	for (j = 0; j < 2; j++) {
		SButton *q = (SButton *)&Pad[j];

		Pad[j].f14c = PC_InputPlayerConnected(j) ? 0x41 : 0;
		Pad[j].a148 = Pad[j].a149 = Pad[j].a14a = Pad[j].a14b = 0;

		for (i = 0; i < 20; i++) {
			int down = PC_InputActionDown(j, s_slotAction[i]);

			any |= down;
			Pad_Button(q + i, down);
		}
	}

	if (!any)
		Pad_IdleTime = Pad_IdleTime + 1;

	return 0;
}
#else
#define B Pad
#define R theControllers

int FUN_80070ff4(void) {
	int a;
	int b;
	unsigned s2;
	int t2, t3, t0, t1, a3, a2, a1, a0;

	{
		int i;

		for (i = 0; i < 2; i++) {
			if (B[i].t158 != 0) {
				if (--B[i].t158 == 0)
					Pad_ActuatorOff(i & 255, 0);
			}

			if (B[i].t15a != 0) {
				if (--B[i].t15a == 0)
					Pad_ActuatorOff(i & 255, 1);
			}
		}
	}

	ReadControllers();

	if (R[0].b[1] == 0x41 || R[0].b[1] == 0x73) {
		a = ~(R[0].b[3] | (R[0].b[2] << 8)) & 0xffff;
	} else {
		Pad_ActuatorOff(0, 0);
		Pad_ActuatorOff(0, 1);

		a = 0;
	}

	if (R[1].b[1] == 0x41 || R[1].b[1] == 0x73) {
		b = ~(R[1].b[3] | (R[1].b[2] << 8)) & 0xffff;
	} else {
		Pad_ActuatorOff(1, 0);
		Pad_ActuatorOff(1, 1);

		b = 0;
	}

	s2 = (b << 16) + a;
	B[0].f14c = R[0].b[1];
	B[1].f14c = R[1].b[1];
	t2 = R[0].b[4] & ~3;
	t3 = R[0].b[5] & ~3;
	t0 = R[0].b[6] & ~3;
	t1 = R[0].b[7] & ~3;
	a3 = R[1].b[4] & ~3;
	a2 = R[1].b[5] & ~3;
	a1 = R[1].b[6] & ~3;
	a0 = R[1].b[7] & ~3;

	if (s2 != 0 || t2 != DAT_800d23e4 || t3 != DAT_800d23e8 || t0 != DAT_800d23ec ||
	    t1 != DAT_800d23f0 || a3 != DAT_800d23f4 || a2 != DAT_800d23f8 || a1 != DAT_800d23fc ||
	    a0 != DAT_800d2400) {
		DAT_800d23e4 = t2;
		DAT_800d23e8 = t3;
		DAT_800d23ec = t0;
		DAT_800d23f0 = t1;
		DAT_800d23f4 = a3;
		DAT_800d23f8 = a2;
		DAT_800d23fc = a1;
		DAT_800d2400 = a0;
	} else {
		Pad_IdleTime = Pad_IdleTime + 1;
	}

	if (((s2 >> 16) & 0x900) != 0x900)
		B[1].f150 = 0;
	else
		B[1].f150 = B[1].f150 + 1;

	if (B[0].f150 == 120 || B[1].f150 == 120)
		return 1;

	{
		int k;

		for (k = 0; k < 2; k++) {
			int ty = B[k].f14c;

			if (ty == 0x73 || ty == 0x53) {
				B[k].a14b = FUN_80070f18(R[k].b[4], DAT_800d1f44, DAT_800d1f48, DAT_800d1f5c);
				B[k].a14a = FUN_80070f18(R[k].b[5], DAT_800d1f4c, DAT_800d1f50, DAT_800d1f60);
				B[k].a149 = FUN_80070f18(R[k].b[6], DAT_800d1f34, DAT_800d1f38, DAT_800d1f54);
				B[k].a148 = FUN_80070f18(R[k].b[7], DAT_800d1f3c, DAT_800d1f40, DAT_800d1f58);
			} else {
				B[k].a14b = 0;
				B[k].a14a = 0;
				B[k].a149 = 0;
				B[k].a148 = 0;
			}
		}
	}

	{
		int j;

		for (j = 0; j < 2; j++) {
			SButton *q = B[j].btn;

			Pad_Button(q++, s2 & 0x10);
			Pad_Button(q++, s2 & 0x80);
			Pad_Button(q++, s2 & 0x20);
			Pad_Button(q++, s2 & 0x40);
			Pad_Button(q++, s2 & 4);
			Pad_Button(q++, s2 & 1);
			Pad_Button(q++, s2 & 8);
			Pad_Button(q++, s2 & 2);
			Pad_Button(q++, (s2 & 0x8000) | (B[j].a149 < -40));
			Pad_Button(q++, (s2 & 0x2000) | (B[j].a149 >= 41));
			Pad_Button(q++, (s2 & 0x1000) | (B[j].a148 < -40));
			Pad_Button(q++, (s2 & 0x4000) | (B[j].a148 >= 41));
			Pad_Button(q++, s2 & 0x200);
			Pad_Button(q++, s2 & 0x400);
			Pad_Button(q++, s2 & 0x800);
			Pad_Button(q++, s2 & 0x100);

			s2 = s2 >> 16;
		}
	}

	return 0;
}
#endif

/* Pad_Update__Fv (0x80070ff4) */
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

/* Pad_Update on PC: every button slot of SControl is one action, and each frame it is fed straight
   from that action's bindings (platform/pc/src/pc_input.c). There is no PS1 button word and no
   analog stick: the pads report as digital (Type 0x41), or 0 when nothing is connected. */
#include "pc_input.h"

/* SControl's 20 button slots in order (game/padbuttons.h), and the action filling each */
static const signed char s_slotAction[20] = {
	PC_ACT_GRIND,     PC_ACT_FLIP,          PC_ACT_GRAB,        PC_ACT_OLLIE,
	PC_ACT_SPIN_LEFT, PC_ACT_TAP_SPIN_LEFT, PC_ACT_SPIN_RIGHT,  PC_ACT_TAP_SPIN_RIGHT,
	PC_ACT_LEFT,      PC_ACT_RIGHT,         PC_ACT_UP,          PC_ACT_DOWN,
	PC_ACT_MENU_UP,   PC_ACT_MENU_DOWN,     PC_ACT_PAUSE,       PC_ACT_CAMERA,
	PC_ACT_MENU_LEFT, PC_ACT_MENU_RIGHT,    PC_ACT_MENU_ACCEPT, PC_ACT_MENU_BACK,
};

int Pad_Update(void) {
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

		/* the motors Pad_ActuatorOn/Off set (Motor0, Motor1) */
		PC_InputRumble(j, ((unsigned char *)&Pad[j].pad154)[0], ((unsigned char *)&Pad[j].pad154)[1]);
	}

	if (!any)
		Pad_IdleTime = Pad_IdleTime + 1;

	return 0;
}

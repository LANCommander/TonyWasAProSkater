/* PAD.CPP: controller input - the per-player button state SControl, reading the controllers
   (on PC through pc_input.h) and the actuators (declarations in game/pad.h). */

#include "game/pad.h"
#include "game/padbuttons.h"
#include "game/class.h"
#include "game/main.h"
#include "game/mem.h"
#include "game/globals.h"
#include "pc_input.h"

extern ControllerBuffer controllers[2];
extern ControllerData theControllers[8];
extern int Pad_IdleTime, ControlSystemInitialised;
void FUN_80096070(void);
void FUN_80096050(void);
int FUN_80070f18(int, int, int, int);

/* SControl's 20 button slots in order (game/padbuttons.h), and the action filling each */
extern "C" { /* (C name, as before) */
static const signed char s_slotAction[20] = {
	PC_ACT_GRIND,     PC_ACT_FLIP,          PC_ACT_GRAB,        PC_ACT_OLLIE,
	PC_ACT_SPIN_LEFT, PC_ACT_TAP_SPIN_LEFT, PC_ACT_SPIN_RIGHT,  PC_ACT_TAP_SPIN_RIGHT,
	PC_ACT_LEFT,      PC_ACT_RIGHT,         PC_ACT_UP,          PC_ACT_DOWN,
	PC_ACT_MENU_UP,   PC_ACT_MENU_DOWN,     PC_ACT_PAUSE,       PC_ACT_CAMERA,
	PC_ACT_MENU_LEFT, PC_ACT_MENU_RIGHT,    PC_ACT_MENU_ACCEPT, PC_ACT_MENU_BACK,
};
}

/* the object FUN_80071a14 destroys: a CClass with two heap blocks, on a list headed by
   DAT_800d1f78 */
struct SPadListObject {
	char *vt;
	char pad[40];
	int *m1;
	int *m2;
	char pad2[16];
	SPadListObject *prev;
	SPadListObject *next;
};
extern char DAT_800a2450[];
extern char DAT_800a2460[];
extern SPadListObject *DAT_800d1f78;
void __dl__6CClassPv(void *p);

/* Pad_InitAtStart__Fv (0x80070878) */
void Pad_InitAtStart(void) {
	int i;

	for (i = 0; i < 2; i++) {
		Pad[i].pGrind = &Pad[i].Grind;
		Pad[i].pGrab = &Pad[i].Grab;
		Pad[i].pFlip = &Pad[i].Flip;
		Pad[i].pOllie = &Pad[i].Ollie;
	}

	Pad_ClearAll();

	Pad_IdleTime = 0;
	ResetMainDemoTimer();

	ControlSystemInitialised = 1;
}

/* Pad_LockAnalog__Fv (0x80070928) */
void Pad_LockAnalog() {
}

/* Pad_ClearAllOne__Fi (0x80070960) */
/* (the counter is stored through a plain word pointer, as the C source did: a typed store
   compiles differently) */
void Pad_ClearAllOne(int i) {
	int o = i * PAD_SIZE;
	char *p = (char *)Pad + o;

	Pad_Clear((SControl *)p);

	*(uint *)((char *)Pad + o + __builtin_offsetof(SControl, ResetCounter)) = 0;
	((SControl *)p)->AnalogueAimLeftRight = 0;
	((SControl *)p)->AnalogueAimForwardsBackwards = 0;
	((SControl *)p)->AnalogueMoveLeftRight = 0;
	((SControl *)p)->AnalogueMoveForwardsBackwards = 0;
}

/* Pad_ClearAll__Fv (0x800709d4) */
void Pad_ClearAll() {
	int i = 0;

	do {
		Pad_ClearAllOne(i);

		i++;
	} while (i < 2);
}

/* Pad_Clear__FP8SControl (0x80070a0c) */
/* (the analogue axes it clears are always pad 0's) */
void Pad_Clear(SControl *pControl) {
	SButton *p = (SButton *)pControl;
	int i = 0;

	do {
		p->Pressed = 0;
		p->Triggered = 0;
		p->PressedTime = 0;
		p->ReleasedTime = 0;
		p->TriggeredTime = 0;
		p++;
		i++;
	} while (i < 0x14);

	Pad[0].AnalogueMoveForwardsBackwards = 0;
	Pad[0].AnalogueMoveLeftRight = 0;
	Pad[0].AnalogueAimForwardsBackwards = 0;
	Pad[0].AnalogueAimLeftRight = 0;
}

/* Pad_ClearTriggers__FP8SControl (0x80070a60) */
void Pad_ClearTriggers(SControl *pControl) {
	SButton *p = (SButton *)pControl;
	int i = 0;

	do {
		p->Triggered = 0;
		i++;
		p++;
	} while (i < 0x14);
}

/* Pad_ActuatorOn__FUcUsUcUc (0x80070a80) */
void Pad_ActuatorOn(uchar index, ushort time, uchar motor, uchar motor_value) {
	int checkIndex = 0;
	int state;

	if (GGame != 7)
		checkIndex = index;

	/* PC: Pad_Update hands the motor values to the player's gamepad (pc_input.h) */
	state = PC_InputGamepadConnected(index) ? 2 : 0;
	if (state != 0) {
		Pad[checkIndex].AlignCalled = 1;

		{
			uchar *p_motor_value = &(&Pad[index].Motor0)[motor];
			ushort *p_motor_timer;
			ushort *q;

			*p_motor_value = motor_value;
			q = &Pad[index].Motor0Timer;
			p_motor_timer = (ushort *)((char *)q + (motor << 1));
			*p_motor_timer = time;
		}
	}
}

/* Pad_ActuatorOff__FUcUc (0x80070c24) */
void Pad_ActuatorOff(unsigned char a, unsigned char b) {
	int o = a * sizeof(SControl);
	char *p = (char *)&Pad[0].Motor0;
	char *q = (char *)&Pad[0].Motor0Timer;

	*(p + o + b) = 0;

	{
		short *s = (short *)(q + o);
		short *t = s + b;

		*t = 0;
	}
}

/* Pad_Button__FP7SButtoni (0x80070c74) */
void Pad_Button(SButton *b, int pressed) {
	b->TriggeredTime++;

	if (!b->Pressed && pressed) {
		b->Triggered = 1;
		b->TriggeredTime = 0;
	}

	if (pressed) {
		Pad_IdleTime = 0;
		ResetMainDemoTimer();

		b->Pressed = 1;
	} else
		b->Pressed = 0;

	if (b->Pressed) {
		b->ReleasedTime = 0;
		b->PressedTime++;
	} else {
		b->PressedTime = 0;
		b->ReleasedTime++;
	}
}

/* ReadControllers__Fv (0x80070d10) */
/* Demo twin. Multitap ports (type 0x80) read four controllers; otherwise read one and mark the other three absent. */
void ReadControllers() {
	int controller_port;

	for (controller_port = 0; controller_port < 2; controller_port++) {
		if (controllers[controller_port].pad[1] == 0x80) {
			ReadController(&theControllers[controller_port * 4 + 0],
			               &controllers[controller_port].pad[2]);
			ReadController(&theControllers[controller_port * 4 + 1],
			               &controllers[controller_port].pad[10]);
			ReadController(&theControllers[controller_port * 4 + 2],
			               &controllers[controller_port].pad[18]);
			ReadController(&theControllers[controller_port * 4 + 3],
			               &controllers[controller_port].pad[26]);
		} else {
			ReadController(&theControllers[controller_port * 4], controllers[controller_port].pad);

			theControllers[controller_port * 4 + 1].type = 0;
			theControllers[controller_port * 4 + 2].type = 0;
			theControllers[controller_port * 4 + 3].type = 0;
		}
	}
}

/* ReadController__FP14ControllerDataPUc (0x80070e5c) */
void ReadController(ControllerData *d, unsigned char *s) {
	unsigned char old = d->type;

	d->status = s[0];
	d->type = s[1];
	d->button1 = s[2];
	d->button2 = s[3];
	d->analog0 = s[4];
	d->analog1 = s[5];
	d->analog2 = s[6];
	d->analog3 = s[7];

	if (s[0] == 0xff) {
		d->type = 0;
	} else if (old != d->type) {
		FUN_80096070();
		FUN_80096050();

		Pad[0].AlignCalled = 0;
		Pad[1].AlignCalled = 0;
	}
}

/* FUN_80070f18 (0x80070f18) */
/* Maps a raw analog axis reading to -127..127 given its calibrated min/max and centre, with a
   dead zone of 32 either side of the centre. Movement outside it marks the pad active. */
int FUN_80070f18(int v, int min, int max, int centre) {
	if (v < min)
		return -127;

	if (max < v)
		return 127;

	if (v < centre - 32) {
		int a, n, b;

		Pad_IdleTime = 0;
		ResetMainDemoTimer();

		a = v + 32;
		n = (centre - a) * -127;
		b = min + 32;
		return n / (centre - b);
	}

	if (centre + 32 < v) {
		int a, n, b;

		Pad_IdleTime = 0;
		ResetMainDemoTimer();

		a = v - 32;
		n = (a - centre) * 127;
		b = max - 32;
		return n / (b - centre);
	}

	return 0;
}

/* Pad_Update__Fv (0x80070ff4) */
int Pad_Update(void) {
	int i, j, any = 0;

	for (i = 0; i < 2; i++) {
		if (Pad[i].Motor0Timer != 0) {
			if (--Pad[i].Motor0Timer == 0)
				((void (*)(int, int))Pad_ActuatorOff)(i & 255, 0); /* (as the C source declared it) */
		}

		if (Pad[i].Motor1Timer != 0) {
			if (--Pad[i].Motor1Timer == 0)
				((void (*)(int, int))Pad_ActuatorOff)(i & 255, 1);
		}
	}

	PC_InputUpdate();

	for (j = 0; j < 2; j++) {
		SButton *q = &Pad[j].Grind;

		Pad[j].Type = PC_InputPlayerConnected(j) ? 0x41 : 0;
		Pad[j].AnalogueMoveForwardsBackwards = Pad[j].AnalogueMoveLeftRight =
			Pad[j].AnalogueAimForwardsBackwards = Pad[j].AnalogueAimLeftRight = 0;

		for (i = 0; i < 20; i++) {
			int down = PC_InputActionDown(j, s_slotAction[i]);

			any |= down;
			Pad_Button(q + i, down);
		}

		/* the motors Pad_ActuatorOn/Off set (Motor0, Motor1) */
		PC_InputRumble(j, Pad[j].Motor0, Pad[j].Motor1);
	}

	if (!any)
		Pad_IdleTime = Pad_IdleTime + 1;

	return 0;
}

/* Pad_Remap__Fv (0x80071588) */
/* Demo twin Pad_Remap (its extra swap block is gone in THPS1): copies each pad's
   face buttons out to the slots their remap pointers name. Inverse of Pad_Restore. */
void Pad_Remap(void) {
}

/* Pad_Restore__Fv (0x8007171c) */
void Pad_Restore(void) {
}

/* (a button's Triggered, read through a byte view of Pad as the C source did: that keeps its
   fully unrolled code) */
#define TRIG(i, button) ((uchar *)Pad)[(i) * PAD_SIZE + __builtin_offsetof(SControl, button) + PAD_TRIGGERED]

/* FUN_800718d0 (0x800718d0) */
/* Any button other than X and Start triggered on either pad (the loading screen takes it as X). */
int FUN_800718d0(void) {
	int i;

	for (i = 0; i < 2; i++) {
		if (TRIG(i, Grind) || TRIG(i, Flip) || TRIG(i, SpinLeft) ||
		    TRIG(i, TapSpinLeft) || TRIG(i, SpinRight) || TRIG(i, TapSpinRight) ||
		    TRIG(i, Left) || TRIG(i, Right) || TRIG(i, Up) || TRIG(i, Down) ||
		    TRIG(i, Grab) || TRIG(i, Camera)
		    || TRIG(i, MenuBack) || TRIG(i, MenuUp) || TRIG(i, MenuDown) ||
		    TRIG(i, MenuLeft) || TRIG(i, MenuRight)
		)
			return 1;
	}

	return i < 2;
}

/* FUN_80071a14 (0x80071a14) */
void FUN_80071a14(SPadListObject *p, int f) {
	p->vt = DAT_800a2450;

	if (p->prev)
		p->prev->next = p->next;

	if (p->next)
		p->next->prev = p->prev;

	if (DAT_800d1f78 == p)
		DAT_800d1f78 = p->prev;

	Mem_Delete(p->m1);
	Mem_Delete(p->m2);

	p->vt = DAT_800a2460;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_80071acc (0x80071acc) */
void FUN_80071acc(char *p, int f) {
	*(char **)(p + 0) = DAT_800a2460;

	if (f & 1)
		__dl__6CClassPv(p);
}

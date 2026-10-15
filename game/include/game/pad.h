/* Controller input (PAD.CPP): the per-player button state SControl (= the demo's PAD.H layout,
   unchanged in retail), the raw libetc/multitap read buffers, and the button-remap table.

   Local stand-ins: front/menuupdate's `SPad { uchar b[364]; }` is SControl viewed as bytes. */
#ifndef GAME_PAD_H
#define GAME_PAD_H

#include "game/types.h"
#include "game/padbuttons.h"

/* one button: level + edge, with the frame times of the last press/release/trigger */
struct SButton {
	uchar Pressed;   /* 0x00 */
	uchar Triggered; /* 0x01 */
	char pad2[0x4 - 0x2];
	int PressedTime;   /* 0x04 */
	int ReleasedTime;  /* 0x08 */
	int TriggeredTime; /* 0x0c */
	                   /* sizeof(SButton) == 0x10 */
};

/* one player's pad (the game keeps `SControl Pad[2]`). The buttons are named after what they do
   while skating; the comments give the PS1 button behind each one. The menus read the Menu*
   names: on the PS1 those are the same buttons (the aliases below), on PC they are bound on their
   own and live in the slots the PS1 never fills (L3/R3 and the unused Crouch..SelectWeapon). */
struct SControl {
	SButton Grind;        /* 0x000 Triangle */
	SButton Flip;         /* 0x010 Square */
	SButton Grab;         /* 0x020 Circle */
	SButton Ollie;        /* 0x030 X */
	SButton SpinLeft;     /* 0x040 L1: held, rotate left */
	SButton TapSpinLeft;  /* 0x050 L2: each tap queues a 180 to the left */
	SButton SpinRight;    /* 0x060 R1 */
	SButton TapSpinRight; /* 0x070 R2 */
	SButton Left;         /* 0x080 */
	SButton Right;        /* 0x090 */
	SButton Up;           /* 0x0a0 */
	SButton Down;         /* 0x0b0 */
	SButton MenuUp;   /* 0x0c0 */
	SButton MenuDown; /* 0x0d0 */
	SButton Pause;  /* 0x0e0 Start */
	SButton Camera; /* 0x0f0 Select: close camera / 2P viewports */
	SButton MenuLeft;   /* 0x100 */
	SButton MenuRight;  /* 0x110 */
	SButton MenuAccept; /* 0x120 */
	SButton MenuBack;   /* 0x130 */
	uchar AnaloguePotMapping[4];               /* 0x140 */
	uchar RawAnalogueMoveForwardsBackwards;    /* 0x144 */
	uchar RawAnalogueMoveLeftRight;            /* 0x145 */
	uchar RawAnalogueAimForwardsBackwards;     /* 0x146 */
	uchar RawAnalogueAimLeftRight;             /* 0x147 */
	signed char AnalogueMoveForwardsBackwards; /* 0x148 */
	signed char AnalogueMoveLeftRight;         /* 0x149 */
	signed char AnalogueAimForwardsBackwards;  /* 0x14a */
	signed char AnalogueAimLeftRight;          /* 0x14b */
	int Type;                                  /* 0x14c */
	uint ResetCounter;                         /* 0x150 */
	uchar Motor0;                              /* 0x154 */
	uchar Motor1;                              /* 0x155 */
	uchar Lock;                                /* 0x156 */
	uchar AlignCalled;                         /* 0x157 */
	ushort Motor0Timer;                        /* 0x158 */
	ushort Motor1Timer;                        /* 0x15a */
	SButton *pGrind;                           /* 0x15c */
	SButton *pFlip;                            /* 0x160 */
	SButton *pGrab;                            /* 0x164 */
	SButton *pOllie;                           /* 0x168 */
	                                           /* sizeof(SControl) == 0x16c */
};
typedef char SControl_is_PAD_SIZE[sizeof(SControl) == PAD_SIZE ? 1 : -1];

/* The front end's Start: its "press Start" prompts, and the "Start or X" tests. On PC that is
   MenuAccept; Pause is a binding of its own there (often the Back key), so it doesn't accept. */
#define MenuStart MenuAccept

/* the controller-options remap: which game function each of the four face buttons does (not used
   on PC, where the bindings say what every key does) */
struct SPadMap {
	int Function[4]; /* 0x00 */
};

/* one decoded controller (ReadController fills it from the raw buffer) */
struct ControllerData {
	uchar status;  /* 0x0 */
	uchar type;    /* 0x1 */
	uchar button1; /* 0x2 */
	uchar button2; /* 0x3 */
	uchar analog0; /* 0x4 */
	uchar analog1; /* 0x5 */
	uchar analog2; /* 0x6 */
	uchar analog3; /* 0x7 */
};

/* one port's raw receive buffer (multitap: 4 x 8 bytes from +2) */
struct ControllerBuffer {
	uchar pad[34]; /* 0x00 */
};

void Pad_ClearAllOne(int i);
void Pad_Clear(SControl *pControl);
void Pad_ActuatorOn(uchar index, ushort time, uchar motor, uchar motor_value);
void Pad_ActuatorOff(unsigned char a, unsigned char b);
void Pad_Button(SButton *b, int pressed);
void ReadControllers();
void ReadController(ControllerData *d, unsigned char *s);
void Pad_Remap();
void Pad_Restore();

/* functions */
void Pad_InitAtStart();
void Pad_LockAnalog();
void Pad_ClearAll();
void Pad_ClearTriggers(SControl *pControl);
int Pad_Update();
int FUN_800718d0();

#endif

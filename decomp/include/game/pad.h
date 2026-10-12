/* Controller input (PAD.CPP): the per-player button state SControl (= the demo's PAD.H layout,
   unchanged in retail), the raw libetc/multitap read buffers, and the button-remap table.

   Local stand-ins: front/menuupdate's `SPad { uchar b[364]; }` is SControl viewed as bytes. */
#ifndef GAME_PAD_H
#define GAME_PAD_H

#include "game/types.h"

/* one button: level + edge, with the frame times of the last press/release/trigger */
struct SButton {
	uchar Pressed;                 /* 0x00 */
	uchar Triggered;               /* 0x01 */
	char pad2[0x4 - 0x2];
	int PressedTime;               /* 0x04 */
	int ReleasedTime;              /* 0x08 */
	int TriggeredTime;             /* 0x0c */
	/* sizeof(SButton) == 0x10 */
};

/* one player's pad (the game keeps `SControl Pad[2]`) */
struct SControl {
	SButton Triangle;              /* 0x000 */
	SButton Square;                /* 0x010 */
	SButton Circle;                /* 0x020 */
	SButton X;                     /* 0x030 */
	SButton LeftOne;               /* 0x040 */
	SButton LeftTwo;               /* 0x050 */
	SButton RightOne;              /* 0x060 */
	SButton RightTwo;              /* 0x070 */
	SButton Left;                  /* 0x080 */
	SButton Right;                 /* 0x090 */
	SButton Up;                    /* 0x0a0 */
	SButton Down;                  /* 0x0b0 */
	SButton AnalogueLeft;          /* 0x0c0 */
	SButton AnalogueRight;         /* 0x0d0 */
	SButton Start;                 /* 0x0e0 */
	SButton Select;                /* 0x0f0 */
	SButton Crouch;                /* 0x100 */
	SButton Jump;                  /* 0x110 */
	SButton SmartBomb;             /* 0x120 */
	SButton SelectWeapon;          /* 0x130 */
	uchar AnaloguePotMapping[4];   /* 0x140 */
	uchar RawAnalogueMoveForwardsBackwards; /* 0x144 */
	uchar RawAnalogueMoveLeftRight;         /* 0x145 */
	uchar RawAnalogueAimForwardsBackwards;  /* 0x146 */
	uchar RawAnalogueAimLeftRight;          /* 0x147 */
	signed char AnalogueMoveForwardsBackwards; /* 0x148 */
	signed char AnalogueMoveLeftRight;         /* 0x149 */
	signed char AnalogueAimForwardsBackwards;  /* 0x14a */
	signed char AnalogueAimLeftRight;          /* 0x14b */
	int Type;                      /* 0x14c */
	uint ResetCounter;             /* 0x150 */
	uchar Motor0;                  /* 0x154 */
	uchar Motor1;                  /* 0x155 */
	uchar Lock;                    /* 0x156 */
	uchar AlignCalled;             /* 0x157 */
	ushort Motor0Timer;            /* 0x158 */
	ushort Motor1Timer;            /* 0x15a */
	SButton *pTriangle;            /* 0x15c */
	SButton *pSquare;              /* 0x160 */
	SButton *pCircle;              /* 0x164 */
	SButton *pX;                   /* 0x168 */
	/* sizeof(SControl) == 0x16c */
};

/* the controller-options remap: which game function each of the four face buttons does */
struct SPadMap {
	int Function[4];               /* 0x00 */
};

/* one decoded controller (ReadController fills it from the raw buffer) */
struct ControllerData {
	uchar status;                  /* 0x0 */
	uchar type;                    /* 0x1 */
	uchar button1;                 /* 0x2 */
	uchar button2;                 /* 0x3 */
	uchar analog0;                 /* 0x4 */
	uchar analog1;                 /* 0x5 */
	uchar analog2;                 /* 0x6 */
	uchar analog3;                 /* 0x7 */
};

/* one port's raw receive buffer (multitap: 4 x 8 bytes from +2) */
struct ControllerBuffer {
	uchar pad[34];                 /* 0x00 */
};

void Pad_InitAtStart();                              /* 0x80070878 */
void Pad_LockAnalog();                               /* 0x80070928 */
void Pad_ClearAllOne(int);                           /* 0x80070960 */
void Pad_ClearAll();                                 /* 0x800709d4 */
void Pad_Clear(SControl *);                          /* 0x80070a0c */
void Pad_ClearTriggers(SControl *);                  /* 0x80070a60 */
void Pad_ActuatorOn(uchar, ushort, uchar, uchar);    /* 0x80070a80 */
void Pad_ActuatorOff(uchar, uchar);                  /* 0x80070c24 */
void Pad_Button(SButton *, int);                     /* 0x80070c74 */
void ReadControllers();                              /* 0x80070d10 */
void ReadController(ControllerData *, uchar *);      /* 0x80070e5c */
void Pad_Remap();                                    /* 0x80071588 */
void Pad_Restore();                                  /* 0x8007171c */

#endif

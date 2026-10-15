/* Panel text effects (MESS.CPP in retail; the demo's TEXTEFFECTS.CPP).

   Retail's FallingTextEffect is NOT the demo's: the demo derives it from a polymorphic TextEffect
   (vptr at +0, 0x120 bytes); retail's is a plain 0x48-byte class (mWaitTime[0] at +0, no vtable)
   with an out-of-line empty destructor. There is no separate TextEffect class in retail:
   mech/ctor_cbruce.cpp's local `TextEffect` (ctor __10TextEffect = 0x8001dfc4) is this class. */
#ifndef GAME_MESS_H
#define GAME_MESS_H

#include "game/types.h"
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

/* the out-of-bounds / falling-letters effect; two live in CPanelData (+0x164, +0x1b0) */
class FallingTextEffect {
  public:
	short mWaitTime[32]; /* 0x00: per-character fall delay (init: 15..44 frames) */
	short m40;           /* 0x40: init sets 280 */
	ushort m42;          /* 0x42: handleComplete() returns it */
	ushort
	    mScale; /* 0x44: Mess_SetScale argument (FallingTextEffect::bind lhu; FallingTextEffect::setScale sets) */
	short m46; /* 0x46: init sets 240; FUN_8001e100 sets */
	/* sizeof(FallingTextEffect) == 0x48 */

	FallingTextEffect();
	~FallingTextEffect(); /* empty */
	void init();
	void update();
	void bind(int On);
	void setScale(ushort Scale);
	ushort handleComplete(); /* returns m42; front/update calls it */
};

/* one step of a message's program (Mess_Message): every value goes from its 0 to its 1 setting
   over Duration frames; then Next steps to the program entry Next entries on (0: the message is
   deleted, 127: it holds) */
struct SMessageProg {
	ushort Duration;      /* 0x00 */
	uchar SemiTrans;      /* 0x02: semi-transparent text */
	uchar Justify;        /* 0x03: Mess_SetTextJustify value */
	ushort Scale0;        /* 0x04 */
	ushort Scale1;        /* 0x06 */
	uchar R0, G0, B0;     /* 0x08 */
	uchar R1, G1, B1;     /* 0x0b */
	ushort Solid0;        /* 0x0e: Mess_DrawText DrawSolid */
	ushort Solid1;        /* 0x10 */
	short X0;             /* 0x12 */
	short Y0;             /* 0x14 */
	short X1;             /* 0x16 */
	short Y1;             /* 0x18 */
	uchar BlinkRate;      /* 0x1a: drawn only on odd periods of this many frames (0: always) */
	uchar pad1b;          /* 0x1b */
	ushort ColourCycle;   /* 0x1c: cycles the red component over time */
	signed char Next;     /* 0x1e */
	char pad1f;           /* 0x1f */
	                      /* sizeof(SMessageProg) == 0x20 */
};

/* an on-screen message (the list pMessages, newest first) */
struct Message {
	const char *pText;  /* 0x00 (a pointer: 8 bytes in the 64-bit port) */
	ushort Timer;       /* 0x04: frames into the current program step */
	ushort TotalTime;   /* 0x06: frames since it was made */
	short X;            /* 0x08 */
	short Y;            /* 0x0a */
	int Stacked;        /* 0x0c: stacked messages are spread out vertically */
	SMessageProg *pProg; /* 0x10 */
	Message *pNext;     /* 0x14 */
	Message *pPrev;     /* 0x18 */
};

/* functions */
void Mess_SetTextJustify(uchar v);
void Mess_SetScale(int s);
int Mess_GetScale();
void Mess_SetSort(int v);
void Mess_SetRGB(uchar r, uchar g, uchar b, int flag);
void Mess_DrawPanelText(int x, int y, const char *s, int a, unsigned b, int c);
int Mess_TextHeightAboveBaseline2(const char *s);
void Mess_DeleteAll();
Message *Mess_Message(const char *text, SMessageProg *prog, bool flag, short x, short y);
void Mess_Update();
void Mess_Display();
int Mess_TextWidth(const char *s0);
int Mess_TextHeightBelowBaseline(const char *s0);
int Mess_TextHeightAboveBaseline(const char *s0);
int Mess_TextHeight(const char *s);
int Mess_DrawText(int x, int y, const char *pMessage, int DrawSolid, u_int FireRGB, int FireSquash, bool IsPanel);
int Mess_LoadFont(char *filename, char *file_buffer, int char_sep, int shadow_offset, int width);
int Mess_LoadFont(char *name, int a, int b, int c);
void Mess_UnloadFont();
void Mess_UnloadAllFonts();
void Mess_SetCurrentFont(int v);
void Mess_SetCurrentFont(char *name);
void Mess_SetFontShadowOffset(uchar v);
void Mess_SetFontScale(uint s);
int Mess_GetFontScale();

/* globals */
extern int Pack_NoRight;
#endif

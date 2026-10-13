/* Panel text effects (MESS.CPP in retail; the demo's TEXTEFFECTS.CPP).

   Retail's FallingTextEffect is NOT the demo's: the demo derives it from a polymorphic TextEffect
   (vptr at +0, 0x120 bytes); retail's is a plain 0x48-byte class (mWaitTime[0] at +0, no vtable)
   with an out-of-line empty destructor. There is no separate TextEffect class in retail:
   mech/ctor_cbruce.cpp's local `TextEffect` (ctor __10TextEffect = 0x8001dfc4) is this class. */
#ifndef GAME_MESS_H
#define GAME_MESS_H

#include "game/types.h"

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

	FallingTextEffect();         /* 0x8001dfc4 */
	~FallingTextEffect();        /* 0x8001dfec (empty) */
	void init();                 /* 0x8001e014 */
	void update();               /* 0x8001e07c */
	void bind(int On);           /* 0x8001e0a4 */
	void setScale(ushort Scale); /* 0x8001e0f8 */
	ushort handleComplete();     /* 0x8001e108 (returns m42; front/update calls it) */
};

/* functions */
void Mess_SetTextJustify(uchar);                  /* 0x8001d250 */
void Mess_SetScale(int s);                        /* 0x8001d25c */
void Mess_SetSort(int Sort);                      /* 0x8001d2c4 */
void Mess_SetRGB(uchar, uchar, uchar, int);       /* 0x8001d2d0 */
int Mess_TextHeightAboveBaseline2(const char *s); /* 0x8001d528 */
void Mess_Update();                               /* 0x8001d6bc */
int Mess_TextWidth(const char *s0);               /* 0x8001da44 */
int Mess_TextHeightBelowBaseline(const char *s);  /* 0x8001db80 */
int Mess_TextHeightAboveBaseline(const char *);   /* 0x8001dc9c */
int Mess_DrawText(int x, int y, const char *pMessage, int DrawSolid, uint FireRGB, int FireSquash,
                  bool IsPanel);             /* 0x8001e114 */
void Mess_UnloadFont();                      /* 0x8001ef60 */
void Mess_UnloadAllFonts();                  /* 0x8001f0dc */
void Mess_SetCurrentFont(int Font);          /* 0x8001f138 */
void Mess_SetCurrentFont(char *name);        /* 0x8001f144 */
void Mess_SetFontShadowOffset(uchar Offset); /* 0x8001f1b4 */

#endif

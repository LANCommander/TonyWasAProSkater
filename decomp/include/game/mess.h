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
	short mWaitTime[32];           /* 0x00: per-character fall delay (init: 15..44 frames) */
	short m40;                     /* 0x40: init sets 280 */
	ushort m42;                    /* 0x42: handleComplete() returns it */
	ushort mScale;                 /* 0x44: Mess_SetScale argument (FUN_8001e0a4 lhu; FUN_8001e0f8 sets) */
	short m46;                     /* 0x46: init sets 240; FUN_8001e100 sets */
	/* sizeof(FallingTextEffect) == 0x48 */

	FallingTextEffect();           /* 0x8001dfc4 */
	~FallingTextEffect();          /* 0x8001dfec (empty) */
	void init();                   /* 0x8001e014 */
	int handleComplete();          /* 0x8001e108 (returns m42; front/update calls it) */
};

#endif

/* bind__17FallingTextEffecti (0x8001e0a4) */
/* C++ (MESS.CPP). Binds the effect to Mess_DrawText (On != 0: its scale, red text, and the global
   wobble descriptor DAT_800d15e8 points at it) or unbinds it. */
#include "game/mess.h"

extern FallingTextEffect *DAT_800d15e8;

void FallingTextEffect::bind(int On) {
	if (On) {
		Mess_SetScale(mScale);
		Mess_SetRGB(255, 48, 48, 0);

		DAT_800d15e8 = this;
	} else
		DAT_800d15e8 = 0;
}

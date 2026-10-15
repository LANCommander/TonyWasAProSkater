/* FUN_80063290 (0x80063290) */
/* C++: (re)loads a sliced image's 256-colour palette into its slot. */
#include "game/shellutils.h"

extern int Pal256Y;
extern int Pal256X;

int Get256SlotNumber(unsigned short a);
extern "C" void FUN_80061c98(int a, int b);

extern "C" void FUN_80063290(SlicedImage *p) {
	int n = Get256SlotNumber(p->clut);

	FUN_80061c98(Pal256X, Pal256Y + n);
}

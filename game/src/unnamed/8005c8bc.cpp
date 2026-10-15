/* FrontLoader_CheckSet__Fv (0x8005c8bc) */
#include "game/frontend.h"
#include "game/shellutils.h"

extern "C" {
	extern int DAT_800d1cac;

	extern SScreenFiles DAT_800be478[15];
}

void FrontLoader_CheckSet(void) {
	if (DAT_800d1cac == 3)
		((volatile SScreenFiles &)DAT_800be478[3]).Num;
}

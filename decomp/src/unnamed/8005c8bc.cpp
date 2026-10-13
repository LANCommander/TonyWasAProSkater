/* FrontLoader_CheckSet__Fv (0x8005c8bc) */
#include "game/frontend.h"

struct S {
	char pad[524];
	int x;
	int Get() {
		return x;
	}
};

extern "C" {
	int DAT_800d1cac;

	extern S DAT_800be478;
}

void FrontLoader_CheckSet(void) {
	if (DAT_800d1cac == 3)
		((volatile S &)DAT_800be478).x;
}

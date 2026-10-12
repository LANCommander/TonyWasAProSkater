/* FUN_8002ef24 (0x8002ef24) */
#include "game/vector.h"

extern "C" {
	extern CVector DAT_800a7f28;
	extern CVector DAT_800a7f34;

	void FUN_8002ef24(CVector *v);
}

void FUN_8002ef24(CVector *v) {
	DAT_800a7f28 = *v;
	int len = -DAT_800a7f28.Length();

	DAT_800a7f34 = DAT_800a7f28 / len;
}

/* HiRes_ClearOTag__Fv (0x8006b240) */
#include "names.h"

extern char *pDoubleBuffer;

void FUN_8008c7d4(char *a, int b);

void HiRes_ClearOTag(void) {
	FUN_8008c7d4(pDoubleBuffer + 132, 8194);
}

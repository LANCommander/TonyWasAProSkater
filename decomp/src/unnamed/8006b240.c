/* HiRes_ClearOTag__Fv (0x8006b240) */
#include "names.h"
#ifdef PC_PORT
#include "pc_draw.h"
#endif

extern char *pDoubleBuffer;

void FUN_8008c7d4(char *a, int b);

void HiRes_ClearOTag(void) {
#ifdef PC_PORT
	PC_DrawClearOTR((void *)(pDoubleBuffer + 132), 8194);
#else
	FUN_8008c7d4(pDoubleBuffer + 132, 8194);
#endif
}

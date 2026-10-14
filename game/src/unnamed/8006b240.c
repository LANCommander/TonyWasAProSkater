/* HiRes_ClearOTag__Fv (0x8006b240) */
#include "names.h"
#include "game/dbuffer.h"
#include "pc_draw.h"

void HiRes_ClearOTag(void) {
	PC_DrawClearOTR(&pDoubleBuffer->OT2D, 8194);
}

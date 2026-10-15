/* FUN_8006c4c4 (0x8006c4c4) */
#include "names.h"

struct SAnimFrame;

extern struct SAnimFrame *DAT_800d1e4c;

struct SAnimFrame *Spool_FindAnim(char *p);

void FUN_8006c4c4(void) {
	DAT_800d1e4c = Spool_FindAnim("Font");
}

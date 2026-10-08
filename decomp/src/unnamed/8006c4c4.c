/* FUN_8006c4c4 (0x8006c4c4) */
#include "names.h"
extern char s_Font_800d1e54[];

int DAT_800d1e4c;

int Spool_FindAnim(char *p);

void FUN_8006c4c4(void) {
	DAT_800d1e4c = Spool_FindAnim(s_Font_800d1e54);
}

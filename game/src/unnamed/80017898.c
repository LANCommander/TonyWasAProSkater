/* PrintPaused__Fv (0x80017898) */
#include "names.h"

extern void *DAT_800d1858;
extern int OT_entry;

void Sprite_draw(void *a, int b, int c, int d, int e, int f, int g);

void PrintPaused(void) {
	OT_entry = 1;
	Sprite_draw(DAT_800d1858, 128, 20, 1, -1, -1, -1);
}

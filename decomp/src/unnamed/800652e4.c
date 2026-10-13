/* FUN_800652e4 (0x800652e4) */
#include "names.h"

extern char __vt_14VerticalDialog[];

int VerticalMenu_ctor(char **p, int a, int b, int c, int d, int e, int f, int g, int h);

char **FUN_800652e4(char **p, int a, int b, int c, int d, int e, int f, int g, int h) {
	VerticalMenu_ctor(p, a, b, c, d, e, f, g, h);

	*p = __vt_14VerticalDialog;
	return p;
}

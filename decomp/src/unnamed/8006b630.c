/* FUN_8006b630 (0x8006b630) */
#include "names.h"

extern char __vt_4CCar[];
extern char BaddyList[];

void CBody_DeleteFrom(char *p, char *q);
int BASEF(char *p, int a);
void __dl__5CItemPv(void *p);

void FUN_8006b630(char *p, int f) {
	*(char **)(p + 72) = __vt_4CCar;
	CBody_DeleteFrom(p, BaddyList);
	BASEF(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

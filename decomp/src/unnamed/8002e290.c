/* FUN_8002e290 (0x8002e290) */
#include "names.h"

extern char __vt_15CTexturedRibbon[];

void Mem_Delete(int a);
int BASEF(char *p, int a);
void __dl__4CBitPv(void *p);

void FUN_8002e290(char *p, int f) {
	*(char **)(p + 68) = __vt_15CTexturedRibbon;
	Mem_Delete(*(int *)(p + 104));
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

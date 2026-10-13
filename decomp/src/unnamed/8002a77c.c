/* FUN_8002a77c (0x8002a77c) */
#include "names.h"

extern char __vt_9CGlassBit[];

char GlassList[8];

int BASEF(char *p, int a);
void CBit_DeleteFrom(char *p, char *q);
void __dl__4CBitPv(void *p);

void FUN_8002a77c(char *p, int f) {
	*(char **)(p + 68) = __vt_9CGlassBit;
	CBit_DeleteFrom(p, GlassList);
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

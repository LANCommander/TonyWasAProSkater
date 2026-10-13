/* FUN_80066180 (0x80066180) */
#include "names.h"

extern char DAT_800a2120[];
extern char DAT_800a21b0[];

void Mem_Delete(int a);
void __dl__6CClassPv(void *p);

void FUN_80066180(int *p, int f) {
	int i;

	p[0] = (int)DAT_800a2120;

	for (i = 0; i < 16; i++) {
		if (p[1 + i])
			Mem_Delete(p[1 + i]);
	}

	p[0] = (int)DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

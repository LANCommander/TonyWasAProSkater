/* FUN_8001f274 (0x8001f274) */
#include "names.h"

extern char DAT_8009ef94[];
extern char DAT_8009efa4[];

void Mem_Delete(int a);
void __dl__6CClassPv(void *p);

void FUN_8001f274(char **p, int f) {
	*p = DAT_8009ef94;

	if (((int *)p)[2])
		Mem_Delete(((int *)p)[1]);

	*p = DAT_8009efa4;

	if (f & 1)
		__dl__6CClassPv(p);
}

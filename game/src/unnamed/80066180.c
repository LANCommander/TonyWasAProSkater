/* FUN_80066180 (0x80066180) */
#include "names.h"

extern char DAT_800a2120[];
extern char DAT_800a21b0[];

/* PREManager (game/pre.h) */
typedef struct {
	char *vt;
	char *mPre[16];
} PM;

void Mem_Delete(void *a);
void __dl__6CClassPv(void *p);

void FUN_80066180(PM *p, int f) {
	int i;

	p->vt = DAT_800a2120;

	for (i = 0; i < 16; i++) {
		if (p->mPre[i])
			Mem_Delete(p->mPre[i]);
	}

	p->vt = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

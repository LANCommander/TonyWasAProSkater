/* FUN_800596a0 (0x800596a0) */
#include "names.h"

struct Q {
	int a, b, c, d, e;
};

extern struct Q *Pack_pFree;

struct Q *DAT_800d1c80;

void FUN_80059618(int a);
void Mem_Delete(struct Q *p);

struct Q *Mem_NewX(int a, int b, int c);

void FUN_800596a0(void) {
	struct Q *n;

	FUN_80059618(0);

	if (Pack_pFree) {
		do {
			n = (struct Q *)Pack_pFree->c;
			Mem_Delete(Pack_pFree);

			Pack_pFree = n;
		} while (n);
	}

	Pack_pFree = Mem_NewX(20, 1, 1);
	*Pack_pFree = *DAT_800d1c80;
	Pack_pFree->c = 0;
	FUN_80059618(0);
}

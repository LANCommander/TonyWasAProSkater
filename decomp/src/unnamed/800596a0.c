/* Front_ResetSkaterPack__Fv (0x800596a0) */
#include "names.h"

struct Q {
	int a, b, c, d, e;
};

extern struct Q *Pack_pFree;

struct Q *DAT_800d1c80;

void Front_SelectSkaterPack(int a);
void Mem_Delete(struct Q *p);

struct Q *Mem_NewX(int a, int b, int c);

void Front_ResetSkaterPack(void) {
	struct Q *n;

	Front_SelectSkaterPack(0);

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
	Front_SelectSkaterPack(0);
}

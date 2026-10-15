/* Front_ResetSkaterPack__Fv (0x800596a0) */
#include "names.h"

/* TSPack (game/m3d.h) */
struct Q {
	unsigned short X, Y, W, H;
	struct Q *pNext;
	char Type;
	unsigned char no_bottom, bpp, aligned;
	short ReqW, ReqH;
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
			n = Pack_pFree->pNext;
			Mem_Delete(Pack_pFree);

			Pack_pFree = n;
		} while (n);
	}

	Pack_pFree = Mem_NewX(sizeof(struct Q), 1, 1);
	*Pack_pFree = *DAT_800d1c80;
	Pack_pFree->pNext = 0;
	Front_SelectSkaterPack(0);
}

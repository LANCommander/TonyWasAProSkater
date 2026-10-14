/* Front_UnloadSkaterTextures2P__Fv (0x80056428) */
#include "names.h"

typedef struct N {
	int a;
	int b;
	struct N *next;
	int d;
	int e;
} N;

int DAT_800d2304[2];
int DAT_800d231c[2];

extern int DAT_800d3758[];
extern N *Pack_pFree;
extern N *DAT_800d1c80;

void Front_SelectSkaterPack(int);
void Pack_KillSlot(N *);
N *Mem_NewX(int, int, int);
void Spool_RestorePSX(int);
void ClearRegion(int, int, int);

void Front_UnloadSkaterTextures2P(void) {
	int i, n;
	N *t;

	Front_SelectSkaterPack(0);

	n = 2;

	if (DAT_800d2304[0] == DAT_800d2304[1] && DAT_800d231c[0] && DAT_800d231c[1])
		n = 1;

	for (i = 0; i < n; i++) {
		if (DAT_800d231c[i]) {
			ClearRegion(DAT_800d3758[DAT_800d2304[i]], 1, 1);
			Spool_RestorePSX(DAT_800d3758[DAT_800d2304[i]]);
		}
	}

	if (Pack_pFree) {
		do {
			t = Pack_pFree->next;
			Pack_KillSlot(Pack_pFree);

			Pack_pFree = t;
		} while (t);
	}

	Pack_pFree = Mem_NewX(20, 1, 1);
	*Pack_pFree = *DAT_800d1c80;
	Pack_pFree->next = 0;
}

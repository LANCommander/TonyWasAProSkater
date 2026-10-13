/* RemovePaletteEntry__FP9tag_S_Pal (0x8004ab88) */
#include "names.h"

int *DAT_800d1a80;

void Mem_Delete(int *p);

void RemovePaletteEntry(int *n) {
	int *p = (int *)n[4];
	int *q;

	if (p)
		p[5] = n[5];

	q = (int *)n[5];

	if (q)
		q[4] = n[4];

	if (n == DAT_800d1a80)
		DAT_800d1a80 = (int *)n[4];

	Mem_Delete(n);
}

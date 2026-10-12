/* M3dMaths_ScaleMatrix__FP4ItemP6MATRIX (0x8008b4e0) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* Only the field this reads (see struct Item in decomp/include/game/m3d.h). */
struct Item {
	char _00[0x24];
	short Scale[3]; /* 0x24: 4.12 per-axis scale */
};

/* Scales the columns of m's rotation by the item's per-axis scale: column c is multiplied by
 * Scale[c] (4.12) on the GTE (GPF, sf=1: IR0 = scale, IR1..3 = the column). */
void M3dMaths_ScaleMatrix__FP4ItemP6MATRIX(struct Item *item, MATRIX *m)
{
	int c, r;

	for (c = 0; c < 3; c++) {
		PC_GteSetData(8, (uint32_t)(int)item->Scale[c]);
		for (r = 0; r < 3; r++)
			PC_GteSetData(9 + r, (uint32_t)(int)m->m[r][c]);
		gte_gpf12();
		for (r = 0; r < 3; r++)
			m->m[r][c] = (short)PC_GteGetData(9 + r);
	}
}

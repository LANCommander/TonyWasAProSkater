/* M3dMaths_ScaleMatrix__FP4ItemP6MATRIX (0x8008b4e0) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "m3dmaths.h"

/* Only the field this reads (see struct Item in decomp/include/game/m3d.h). */
struct Item {
	char _00[0x24];
	short Scale[3]; /* 0x24: 4.12 per-axis scale */
};

/* Scales the columns of m's rotation by the item's per-axis scale: column c is multiplied by
 * Scale[c] (4.12, >> 12, saturated to 16 bits). */
void M3dMaths_ScaleMatrix__FP4ItemP6MATRIX(struct Item *item, MATRIX *m) {
	int c, r;

	for (c = 0; c < 3; c++) {
		int16_t col[3];
		GeomAcc a;

		for (r = 0; r < 3; r++)
			col[r] = m->m[r][c];

		a = geom_gpf(item->Scale[c], col, 12, 0, 0, 0, 0);

		for (r = 0; r < 3; r++)
			m->m[r][c] = a.ir[r];
	}

}

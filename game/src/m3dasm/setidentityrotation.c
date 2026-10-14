/* M3dMaths_SetIdentityRotation__FP6MATRIX (0x8008b238) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "m3dmaths.h"

/* Sets a matrix's rotation to the identity (1.0 = 4096 on the diagonal). The translation is left
 * alone; the padding halfword after m[2][2] is cleared (retail writes it as one word). */
void M3dMaths_SetIdentityRotation__FP6MATRIX(MATRIX *m) {
	int r, c;

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			m->m[r][c] = r == c ? 4096 : 0;

	m3d_store_m22_word(m, 4096);
}

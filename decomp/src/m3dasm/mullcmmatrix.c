/* M3dMaths_MulLcmMatrix__FP6MATRIXT0 (0x8008b314) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* dst = LCM * src, where LCM is the light colour matrix currently loaded in the GTE (4.12, lcir
 * per column of src). Same shape as M3dMaths_MulRotMatrix: src may be dst, and m[2][2] is
 * stored as the whole MAC3 word (the padding halfword gets MAC3's high bits). */
void M3dMaths_MulLcmMatrix__FP6MATRIXT0(MATRIX *src, MATRIX *dst) {
	short a[3][3], out[3][3];
	uint32_t mac3;
	int r, c;

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			a[r][c] = src->m[r][c];

	mac3 = m3d_mul_matrix_ir(M3D_LCIR, a, out);

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			dst->m[r][c] = out[r][c];

	m3d_store_m22_word(dst, mac3);
}

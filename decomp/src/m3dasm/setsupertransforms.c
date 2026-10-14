/* M3dAsm_SetSuperTransforms__FP6MATRIXT0P7SMatrix (0x8008a1c8) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "supertransforms.h"

/* Loads the GTE with the transforms for one part of a super item (see supertransforms.h):
 * rotation = A * Rc, translation = A * (Tc >> 4) + At, light matrix = B * Rc. The frame's
 * translation is stored in 16ths of a unit, hence the >> 4. */
void M3dAsm_SetSuperTransforms__FP6MATRIXT0P7SMatrix(MATRIX *a, MATRIX *b, struct SMatrix *c) {
	m3d_set_super_transforms(a, b, c, 4);
}

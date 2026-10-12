/* M3dAsm_SetSuperTransformsDewibbled__FP6MATRIXT0P7SMatrix (0x80089fc8) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "supertransforms.h"

/* Loads the GTE with the transforms for one part of a super item (see supertransforms.h):
 * rotation = A * Rc, translation = A * Tc + At, light matrix = B * Rc. Unlike
 * M3dAsm_SetSuperTransforms the frame's translation is used unshifted, so the result stays at
 * 16x scale (the "dewibbled" path projects at that scale for sub-pixel precision). */
void M3dAsm_SetSuperTransformsDewibbled__FP6MATRIXT0P7SMatrix(MATRIX *a, MATRIX *b,
                                                              struct SMatrix *c)
{
	m3d_set_super_transforms(a, b, c, 0);
}

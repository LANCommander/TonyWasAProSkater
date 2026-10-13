/* M3dAsm_TransformAndOutcodeSuperVerticesDewibbled__FP7SVECTORi (0x800860cc) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "vertex.h"

/* TransformAndOutcodeSuperVertices for "dewibbled" rendering: the 12.4 vertices go to the GTE
 * unscaled (the transform is set up 16 times larger, so the projection keeps the fraction bits
 * and the vertices don't jitter), and the rotated position and depth are scaled back (>> 4). */
int M3dAsm_TransformAndOutcodeSuperVerticesDewibbled__FP7SVECTORi(SVECTOR *v, int n) {
	return vtx_transform_super((const uint32_t *)v, n, 1);
}

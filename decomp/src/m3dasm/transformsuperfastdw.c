/* M3dAsm_TransformAndOutcodeSuperVerticesFastDewibbled__FP7SVECTORi (0x80086404) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "vertex.h"

/* TransformAndOutcodeSuperVerticesFast for dewibbled rendering: the 12.4 vertices go to the GTE
 * unscaled and the depth is scaled back (>> 4); the screen position needs no scaling. */
int M3dAsm_TransformAndOutcodeSuperVerticesFastDewibbled__FP7SVECTORi(SVECTOR *v, int n) {
	return vtx_transform_super_fast((const uint32_t *)v, n, 1);
}

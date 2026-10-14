/* M3dAsm_TransformAndOutcodeSuperVerticesFastDewibbled__FP7SVECTORi (0x80086404) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "vertex.h"

static int transform_vertices(void *v, int n, void *o) {
	return vtx_transform_super_fast((const uint32_t *)v, n, 1);
}

/* TransformAndOutcodeSuperVerticesFast for dewibbled rendering: the 12.4 vertices go to the GTE
 * unscaled and the depth is scaled back (>> 4); the screen position needs no scaling. */
int M3dAsm_TransformAndOutcodeSuperVerticesFastDewibbled__FP7SVECTORi(SVECTOR *v, int n) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_SFAST_DW, transform_vertices, v, n, 0);
}

/* M3dAsm_TransformAndOutcodeSuperVerticesFast__FP7SVECTORi (0x800862d4) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "vertex.h"

#ifdef R3D_HOOKS
static int transform_vertices(void *v, int n, void *o) {
	return vtx_transform_super_fast((const uint32_t *)v, n, 0);
}
#endif

/* The fast version of TransformAndOutcodeSuperVertices: screen words with folded-in clip flags
 * and depths go to the scratchpad cache (no rotated positions, no outcodes), stitched as usual.
 * Returns the AND of the screen words' flag bits 0x60006000 (0x60000000 for no vertices). */
int M3dAsm_TransformAndOutcodeSuperVerticesFast__FP7SVECTORi(SVECTOR *v, int n) {
#ifdef R3D_HOOKS
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_SFAST, transform_vertices, v, n, 0);
#else
	return vtx_transform_super_fast((const uint32_t *)v, n, 0);
#endif
}

/* M3dAsm_TransformAndOutcodeSuperVertices__FP7SVECTORi (0x80085e98) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "vertex.h"

static int transform_vertices(void *v, int n, void *o) {
	return vtx_transform_super((const uint32_t *)v, n, 0);
}

/* Projects one part of a super (skinned) model, whose vertices are in 12.4 fixed point, into
 * ProjectedVertices/RotatedVertices with outcodes, sharing the vertices parts have in common
 * through the stitch pool (see vertex.h). Returns the AND of the outcodes (0xff for none). */
int M3dAsm_TransformAndOutcodeSuperVertices__FP7SVECTORi(SVECTOR *v, int n) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_SUPER, transform_vertices, v, n, 0);
}

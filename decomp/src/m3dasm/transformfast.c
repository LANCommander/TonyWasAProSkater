/* M3dAsm_TransformAndOutcodeVerticesFast__FP7SVECTORiP6VECTOR (0x80089790) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "vertex.h"

/* An ordinary model's vertex moved by the offset o (added to each coordinate, 16-bit wrap),
 * projected. */
static GeomProj project_offset_vertex(const SVECTOR *v, const VECTOR *o) {
	return vtx_project(v->vx + o->vx, v->vy + o->vy, v->vz + o->vz);
}

/* Projects a model's vertices, moved by o, with the current rotation (the translation is
 * cleared) into the scratchpad's fast cache: screen word with clip flags (vtx_fast_word) and
 * depth. Returns the AND of the screen words (-1 for no vertices). (The retail loop is
 * software-pipelined and also projects the word pair just past the last vertex, for nothing.) */
static int transform_vertices(SVECTOR *v, int n, VECTOR *o) {
	struct SViewport *vp;
	struct SFastVertex *out = FAST_CACHE;
	uint32_t xRyB, all = 0xffffffffu;
	int yon;

	geom_load_vector(pc_geom.tr, 0, 0, 0);

	if (n == 0) {
		return -1;
	}

	vp = pCurrentViewport;
	xRyB = vtx_pack(vp->xR, vp->yB);
	yon = (short)vp->Yon;

	for (; n != 0; n--, out++, v++) {
		GeomProj p = project_offset_vertex(v, o);
		int z = p.v.ir[2];

		out->XY = vtx_fast_word(p.sxy, z, xRyB, yon);
		out->z = (short)z;
		all &= out->XY;
	}

	return (int)all;
}

int M3dAsm_TransformAndOutcodeVerticesFast__FP7SVECTORiP6VECTOR(SVECTOR *v, int n, VECTOR *o) {
#ifdef R3D_HOOKS
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_FAST, (R3dTransformFn)transform_vertices, v, n, o);
#else
	return transform_vertices(v, n, o);
#endif
}

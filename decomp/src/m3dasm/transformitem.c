/* M3dAsm_TransformAndOutcodeItemVertices__FP7SVECTORi (0x80085d28) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "vertex.h"

/* Projects an ordinary model's vertices with the current rotation/translation: for each one
 * the screen position, depth and outcode go to ProjectedVertices and the view-space x, y to
 * RotatedVertices. Returns the AND of all the outcodes (0xff for no vertices), so the caller can
 * skip a model that is wholly off one side of the view. */
static int transform_vertices(SVECTOR *v, int n) {
	struct ClipBounds b;
	struct SProjectedVertex *pv;
	struct SRotatedVertex *rv;
	uint32_t all = 0xffff;

	if (n == 0)
		return 0xff;

	vtx_clip_bounds(&b);

	pv = ProjectedVertices;
	rv = RotatedVertices;

	for (; n != 0; n--, v++, pv++, rv++) {
		GeomProj p = vtx_project(v->vx, v->vy, v->vz);
		int z;
		uint32_t oc;

		rv->xy = vtx_pack(p.v.ir[0], p.v.ir[1]);
		z = p.v.ir[2];
		pv->XY = p.sxy;
		pv->z = (short)z;
		oc = vtx_outcode(vtx_outcode_bits(&b, pv->XY, z));
		pv->Outcode = (ushort)oc;
		all &= oc;
	}

	SCR_PROJECTED_END = pv;
	return (int)all;
}

int M3dAsm_TransformAndOutcodeItemVertices__FP7SVECTORi(SVECTOR *v, int n) {
#ifdef R3D_HOOKS
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_ITEM, (R3dTransformFn)transform_vertices, v, n, 0);
#else
	return transform_vertices(v, n);
#endif
}

/* M3dAsm_TransformAndOutcodeVerticesFast__FP7SVECTORiP6VECTOR (0x80089790) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "vertex.h"

/* An ordinary model's vertex moved by the offset o (added to each coordinate, 16-bit wrap), as
 * the GTE's V0. */
static void load_offset_vertex(const SVECTOR *v, const VECTOR *o) {
	gte_set(GTE_VXY0, vtx_pack(v->vx + o->vx, v->vy + o->vy));
	gte_set(GTE_VZ0, ((const uint32_t *)v)[1] + (uint32_t)o->vz);
}

/* Projects a model's vertices, moved by o, with the current rotation (the GTE translation is
 * cleared) into the scratchpad's fast cache: screen word with clip flags (vtx_fast_word) and
 * depth. Returns the AND of the screen words (-1 for no vertices).
 *
 * The retail loop is software-pipelined: it loads and projects the next vertex before storing
 * the current one, so it also projects the word pair just past the last vertex; that is kept,
 * since the GTE is left holding its results. */
int M3dAsm_TransformAndOutcodeVerticesFast__FP7SVECTORiP6VECTOR(SVECTOR *v, int n, VECTOR *o) {
	struct SViewport *vp;
	struct SFastVertex *out = FAST_CACHE;
	uint32_t xRyB, all = 0xffffffffu;
	int yon;

	gte_ldtr(0, 0, 0);
	load_offset_vertex(v, o);

	if (n == 0)
		return -1;

	gte_rtps();

	vp = pCurrentViewport;
	xRyB = vtx_pack(vp->xR, vp->yB);
	yon = (short)vp->Yon;

	for (; n != 0; n--, out++) {
		uint32_t sxy = PC_GteGetData(GTE_SXY2);
		int z = gte_get(GTE_IR3);

		load_offset_vertex(++v, o);
		gte_rtps();

		out->XY = vtx_fast_word(sxy, z, xRyB, yon);
		out->z = (short)z;
		all &= out->XY;
	}

	return (int)all;
}

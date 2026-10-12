/* M3dAsm_TransformAndOutcodeItemVertices__FP7SVECTORi (0x80085d28) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "vertex.h"

/* Projects an ordinary model's vertices with the current GTE rotation/translation: for each one
 * the screen position, depth and outcode go to ProjectedVertices and the view-space x, y to
 * RotatedVertices. Returns the AND of all the outcodes (0xff for no vertices), so the caller can
 * skip a model that is wholly off one side of the view. */
int M3dAsm_TransformAndOutcodeItemVertices__FP7SVECTORi(SVECTOR *v, int n)
{
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
		int z;
		uint32_t oc;

		gte_ldv0(v);
		gte_rtps();
		rv->xy = vtx_pack(gte_get(GTE_IR1), gte_get(GTE_IR2));
		z = gte_get(GTE_IR3);
		pv->XY = PC_GteGetData(GTE_SXY2);
		pv->z = (short)z;
		oc = vtx_outcode(vtx_outcode_bits(&b, pv->XY, z));
		pv->Outcode = (ushort)oc;
		all &= oc;
	}
	SCR_PROJECTED_END = pv;
	return (int)all;
}

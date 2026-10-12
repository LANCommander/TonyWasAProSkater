/* M3dAsm_TransformAndOutcodeBackgroundVertices__FP7SVECTORi (0x80086590) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include <limits.h>
#include "vertex.h"

/* Projects background (sky) vertices: the GTE translation is cleared first, so only the camera's
 * rotation applies and the sky stays put around the viewer. Each vertex gets its screen position
 * and outcode in ProjectedVertices, with the depth forced to 0x7fff (furthest) and no far-plane
 * test; nothing goes to RotatedVertices. Returns the AND of the outcodes (0xff for none). */
int M3dAsm_TransformAndOutcodeBackgroundVertices__FP7SVECTORi(SVECTOR *v, int n)
{
	struct ClipBounds b;
	struct SProjectedVertex *pv;
	uint32_t all = 0xffff;

	if (n == 0)
		return 0xff;
	gte_ldtr(0, 0, 0);
	vtx_clip_bounds(&b);
	b.Yon = INT_MAX; /* never beyond the far plane */
	pv = ProjectedVertices;
	for (; n != 0; n--, v++, pv++) {
		uint32_t oc;

		gte_ldv0(v);
		gte_rtps();
		pv->XY = PC_GteGetData(GTE_SXY2);
		pv->z = 0x7fff;
		oc = vtx_outcode(vtx_outcode_bits(&b, pv->XY, gte_get(GTE_IR3)));
		pv->Outcode = (ushort)oc;
		all &= oc;
	}
	return (int)all;
}

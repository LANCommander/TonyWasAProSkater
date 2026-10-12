/* M3dAsm_GetDynamicLighting__FP7SVECTORi (0x80086a4c) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "vertex.h"

/* Lights a model's vertices from their normals (one per vertex, flags in the pad as for super
 * vertices): NCCS with the light/colour matrices and back colour the caller set up, and a white
 * vertex colour. The colour (IR1-3, 4.12 fixed point) is packed 11:11:10 (the top bits of each
 * channel) into the RotatedVertices Special word of the vertex (ProjectedVertices + 3204), and
 * into the stitch pool for exported vertices; stitch vertices copy theirs from the pool. The
 * pool pointer itself is not advanced (the transform that follows does that). The loop tests
 * its count after each vertex, so n == 0 still lights one. */
void M3dAsm_GetDynamicLighting__FP7SVECTORi(SVECTOR *normals, int n)
{
	const uint32_t *v = (const uint32_t *)normals;
	char *pv = (char *)ProjectedVertices, *pool = (char *)FreeStitchVertex;
	char *top = pv + STITCH_TOP;

	gte_set(GTE_RGBC, 0x00ffffff);
	do {
		uint32_t flags = v[1] >> 16, colour;

		gte_set(GTE_VXY0, v[0]);
		gte_set(GTE_VZ0, v[1]);
		gte_nccs();
		if (flags & VF_STITCH) {
			colour = *(uint32_t *)(top - v[0] + 3204);
		} else {
			colour = ((uint32_t)gte_get(GTE_IR1) & 0xffe0) >> 5 |
			         ((uint32_t)gte_get(GTE_IR2) & 0xffe0) << 6 |
			         ((uint32_t)gte_get(GTE_IR3) & 0xffe0) << 17;
			if (flags & VF_EXPORT) {
				*(uint32_t *)(pool + 3204) = colour;
				pool -= 8;
			}
		}
		*(uint32_t *)(pv + 3204) = colour;
		pv += 8;
		v += 2;
	} while (--n > 0);
}

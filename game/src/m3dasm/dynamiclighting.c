/* M3dAsm_GetDynamicLighting__FP7SVECTORi (0x80086a4c) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "processpolys.h"

/* Lights a model's vertices from their normals (one per vertex, flags in the pad as for super
 * vertices): the light and colour matrices and back colour the caller set up, and a white
 * vertex colour (geom_nccs). The colour (4.12 fixed point, 16 bits per channel) is packed
 * 11:11:10 (the top bits of each channel) into the RotatedVertices Special word of the vertex
 * (ProjectedVertices + 3204), and
 * into the stitch pool for exported vertices; stitch vertices copy theirs from the pool. The
 * pool pointer itself is not advanced (the transform that follows does that). The loop tests
 * its count after each vertex, so n == 0 still lights one. The colour code (processpolys.h) is
 * left white, as retail leaves the GTE's RGBC.
 * test_asm: colourcode */
void M3dAsm_GetDynamicLighting__FP7SVECTORi(SVECTOR *normals, int n) {
	const uint32_t *v = (const uint32_t *)normals;
	char *pv = (char *)ProjectedVertices, *pool = (char *)FreeStitchVertex;
	char *top = pv + STITCH_TOP;

	M3dAsm_ColourCode = 0x00ffffff;

	do {
		uint32_t flags = v[1] >> 16, colour;
		GeomAcc l = geom_nccs(&pc_geom, (short)v[0], (int32_t)v[0] >> 16, (short)v[1],
		                      0x00ffffff, 12, 1, 0, 0);

		if (flags & VF_STITCH) {
			colour = *(uint32_t *)(top - v[0] + 3204);
		} else {
			colour = ((uint32_t)l.ir[0] & 0xffe0) >> 5 | ((uint32_t)l.ir[1] & 0xffe0) << 6 |
			         ((uint32_t)l.ir[2] & 0xffe0) << 17;

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

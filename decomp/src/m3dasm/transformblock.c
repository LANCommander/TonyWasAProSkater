/* M3dAsm_TransformAndOutcodeBlockVertices__FP7SVECTORiP6VECTOR (0x80086d2c) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "vertex.h"

int M3dMaths_SquareRoot0__Fi(int n);

#define VF_ALWAYS_FACING 0x10 /* in the vertex's pad: this and all later vertices are billboards */

/* MIPS div: no trap on a zero divisor (the quotient is then -1, or 1 for a negative dividend) */
static int mips_div(int n, int d) {
	if (d == 0)
		return n >= 0 ? -1 : 1;

	return n / d;
}

/* AlwaysFacing_Block_Vertices (0x80086b10, only ever entered from the transform below, with the
 * GTE having just projected the first billboard vertex, whose result is dropped).
 *
 * A billboard vertex is a screen-space offset from an already projected vertex (its first word:
 * the byte offsets in ProjectedVertices of that centre vertex, low half, and of a second vertex
 * that gives the direction, high half). Its offset is the size in its second word (IR0) along the
 * screen direction from the centre to the second vertex, normalised; it is projected at the
 * centre's depth with x stretched by 10485/4096 (the GTE rotation is replaced by that scale and
 * the screen offset is cleared, then restored from the viewport) and added to the centre's
 * screen position. Outcodes as for the others, with the centre's depth. Returns the AND of all
 * the outcodes so far, `all`. */
static int always_facing(const SVECTOR *v, int n, struct SProjectedVertex *pv,
                         const struct ClipBounds *b, uint32_t all) {
	char *base = (char *)ProjectedVertices;
	int len = 0;

	gte_setc(GTE_RT + 0, 10485); /* RT11 = 2.56, RT12 = 0 */
	gte_setc(GTE_RT + 1, 0);
	gte_setc(GTE_RT + 2, 4096); /* RT22 = 1 */
	gte_setc(GTE_RT + 3, 0);
	gte_setc(GTE_RT + 4, 4096); /* RT33 = 1 */
	gte_setc(GTE_OFX, 0);
	gte_setc(GTE_OFY, 0);

	for (; n != 0; n--, v++, pv++) {
		const uint32_t *w = (const uint32_t *)v;
		const struct SProjectedVertex *centre =
		    (const struct SProjectedVertex *)(base + (w[0] & 0xffff));
		const struct SProjectedVertex *toward =
		    (const struct SProjectedVertex *)(base + (w[0] >> 16));
		int cx = (short)centre->XY, cy = (int32_t)centre->XY >> 16, z = centre->z;
		int dx = (short)toward->XY - cx, dy = cy - ((int32_t)toward->XY >> 16);
		int sx, sy;
		uint32_t sxy, oc;

		/* the direction's length: dx^2 + dy^2 by the GTE, then the square root */
		gte_set(GTE_IR1, dx);
		gte_set(GTE_IR2, dy);
		gte_sqr0();

		len = M3dMaths_SquareRoot0__Fi(gte_get(GTE_MAC1) + gte_get(GTE_MAC2));

		/* (dy, dx) normalised (4.12) times the size: the offset, perpendicular to the direction */
		gte_set(GTE_IR1, mips_div(dy << 12, len));
		gte_set(GTE_IR2, mips_div(dx << 12, len));
		gte_set(GTE_IR0, w[1]);
		gte_gpf12();
		gte_set(GTE_VXY0, vtx_pack(gte_get(GTE_MAC1), gte_get(GTE_MAC2)));
		gte_set(GTE_VZ0, z);
		gte_rtps();

		sxy = PC_GteGetData(GTE_SXY2);
		sx = (short)sxy + cx;
		sy = ((int32_t)sxy >> 16) + cy;
		oc = vtx_outcode(vtx_outcode_xy(b, sx, sy, z));
		all &= oc;
		pv->XY = vtx_pack(sx, sy);
		pv->z = (short)z;
		pv->Outcode = (ushort)oc;
	}

	gte_setc(GTE_OFX, (uint32_t)pCurrentViewport->xC << 16);
	gte_setc(GTE_OFY, (uint32_t)pCurrentViewport->yC << 16);

	/* Retail stores v0 here as the end of ProjectedVertices, as the other transforms do, but v0
	 * then holds the last square root (the call clobbered it). Reproduced as it is. */
	*(uint32_t *)&SCR_PROJECTED_END = (uint32_t)len;
	return (int)all;
}

/* Projects a block (a world model) moved by o (added to each vertex, 16-bit wrap) with the
 * current rotation, the GTE translation cleared: screen position, depth and outcode to
 * ProjectedVertices, view-space x, y to RotatedVertices. A vertex whose GTE projection overflowed
 * (FLAG bit 31) gets outcode bit 0x40 and loses its four screen bits. The vertices from the first
 * one flagged always-facing on are billboards (always_facing). Returns the AND of the outcodes
 * (0xff for no vertices). */
int M3dAsm_TransformAndOutcodeBlockVertices__FP7SVECTORiP6VECTOR(SVECTOR *v, int n, VECTOR *o) {
	struct ClipBounds b;
	struct SProjectedVertex *pv;
	struct SRotatedVertex *rv;
	uint32_t all = 0xffff;

	if (n == 0)
		return 0xff;

	vtx_clip_bounds(&b);
	gte_ldtr(0, 0, 0);

	pv = ProjectedVertices;
	rv = RotatedVertices;

	for (; n != 0; n--, v++, pv++, rv++) {
		int z;
		uint32_t oc;

		gte_set(GTE_VXY0, vtx_pack(v->vx + o->vx, v->vy + o->vy));
		gte_set(GTE_VZ0, ((const uint32_t *)v)[1] + (uint32_t)o->vz);
		gte_rtps();

		if (v->pad & VF_ALWAYS_FACING)
			return always_facing(v, n, pv, &b, all);

		rv->xy = vtx_pack(gte_get(GTE_IR1), gte_get(GTE_IR2));
		z = gte_get(GTE_IR3);
		pv->XY = PC_GteGetData(GTE_SXY2);
		pv->z = (short)z;
		oc = vtx_outcode_bits(&b, pv->XY, z);

		if ((int32_t)gte_getc(GTE_FLAG) < 0)
			oc = (oc | 0x40) & ~0xfu;

		oc = vtx_outcode(oc);
		pv->Outcode = (ushort)oc;
		all &= oc;
	}

	SCR_PROJECTED_END = pv;
	return (int)all;
}

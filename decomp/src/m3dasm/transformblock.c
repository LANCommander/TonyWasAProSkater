/* M3dAsm_TransformAndOutcodeBlockVertices__FP7SVECTORiP6VECTOR (0x80086d2c) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "vertex.h"

int M3dMaths_SquareRoot0__Fi(int n);

#define VF_ALWAYS_FACING 0x10 /* in the vertex's pad: this and all later vertices are billboards */

/* MIPS div: no trap on a zero divisor (the quotient is then -1, or 1 for a negative dividend) */
static int mips_div(int n, int d) {
	if (d == 0)
		return n >= 0 ? -1 : 1;

	return n / d;
}

/* AlwaysFacing_Block_Vertices (0x80086b10, only ever entered from the transform below, which
 * has just projected the first billboard vertex and drops the result).
 *
 * A billboard vertex is a screen-space offset from an already projected vertex (its first word:
 * the byte offsets in ProjectedVertices of that centre vertex, low half, and of a second vertex
 * that gives the direction, high half). Its offset is the size in its second word along the
 * screen direction from the centre to the second vertex, normalised; it is projected at the
 * centre's depth with x stretched by 10485/4096 (the rotation is replaced by that scale, which
 * stays, and the screen offset is cleared, then restored from the viewport) and added to the
 * centre's screen position. Outcodes as for the others, with the centre's depth. Returns the AND
 * of all the outcodes so far, `all`. */
static int always_facing(const SVECTOR *v, int n, struct SProjectedVertex *pv,
                         const struct ClipBounds *b, uint32_t all) {
	char *base = (char *)ProjectedVertices;
	int len = 0;

	memset(pc_geom.rot, 0, sizeof pc_geom.rot);
	pc_geom.rot[0][0] = 10485; /* 2.56 */
	pc_geom.rot[1][1] = 4096;
	pc_geom.rot[2][2] = 4096;
	pc_geom.ofx = 0;
	pc_geom.ofy = 0;

	for (; n != 0; n--, v++, pv++) {
		const uint32_t *w = (const uint32_t *)v;
		const struct SProjectedVertex *centre =
		    (const struct SProjectedVertex *)(base + (w[0] & 0xffff));
		const struct SProjectedVertex *toward =
		    (const struct SProjectedVertex *)(base + (w[0] >> 16));
		int cx = (short)centre->XY, cy = (int32_t)centre->XY >> 16, z = centre->z;
		int dx = (short)toward->XY - cx, dy = cy - ((int32_t)toward->XY >> 16);
		int16_t d[3];
		GeomAcc a;
		int sx, sy;
		uint32_t sxy, oc;

		/* the direction's length: the square root of dx^2 + dy^2 (16-bit dx, dy) */
		d[0] = (int16_t)dx;
		d[1] = (int16_t)dy;
		d[2] = 0;
		a = geom_sqr(d, 0, 1, 0);
		len = M3dMaths_SquareRoot0__Fi(a.mac[0] + a.mac[1]);

		/* (dy, dx) normalised (4.12) times the size: the offset, perpendicular to the direction */
		d[0] = (int16_t)mips_div(dy << 12, len);
		d[1] = (int16_t)mips_div(dx << 12, len);
		a = geom_gpf((int16_t)w[1], d, 12, 0, 0, 0, 0);
		sxy = vtx_project(a.mac[0], a.mac[1], z).sxy;
		sx = (short)sxy + cx;
		sy = ((int32_t)sxy >> 16) + cy;
		oc = vtx_outcode(vtx_outcode_xy(b, sx, sy, z));
		all &= oc;
		pv->XY = vtx_pack(sx, sy);
		pv->z = (short)z;
		pv->Outcode = (ushort)oc;
	}

	pc_geom.ofx = (int32_t)((uint32_t)pCurrentViewport->xC << 16);
	pc_geom.ofy = (int32_t)((uint32_t)pCurrentViewport->yC << 16);

	/* Retail stores v0 here as the end of ProjectedVertices, as the other transforms do, but v0
	 * then holds the last square root (the call clobbered it). Reproduced as it is. */
	*(uint32_t *)&SCR_PROJECTED_END = (uint32_t)len;
	return (int)all;
}

/* Projects a block (a world model) moved by o (added to each vertex, 16-bit wrap) with the
 * current rotation, the translation cleared: screen position, depth and outcode to
 * ProjectedVertices, view-space x, y to RotatedVertices. A vertex whose projection overflowed
 * (any error in its FLAG) gets outcode bit 0x40 and loses its four screen bits. The vertices from
 * the first one flagged always-facing on are billboards (always_facing). Returns the AND of the
 * outcodes (0xff for no vertices). */
static int transform_vertices(SVECTOR *v, int n, VECTOR *o) {
	struct ClipBounds b;
	struct SProjectedVertex *pv;
	struct SRotatedVertex *rv;
	uint32_t all = 0xffff;

	if (n == 0)
		return 0xff;

	vtx_clip_bounds(&b);
	geom_load_vector(pc_geom.tr, 0, 0, 0);

	pv = ProjectedVertices;
	rv = RotatedVertices;

	for (; n != 0; n--, v++, pv++, rv++) {
		uint32_t flag, oc;
		GeomProj p = geom_rtps(&pc_geom, (int16_t)(v->vx + o->vx), (int16_t)(v->vy + o->vy),
		                       (int16_t)(v->vz + o->vz), 12, 0, 0, &flag);
		int z;

		if (v->pad & VF_ALWAYS_FACING)
			return always_facing(v, n, pv, &b, all);

		rv->xy = vtx_pack(p.v.ir[0], p.v.ir[1]);
		z = p.v.ir[2];
		pv->XY = p.sxy;
		pv->z = (short)z;
		oc = vtx_outcode_bits(&b, pv->XY, z);

		if (flag & 0x80000000u)
			oc = (oc | 0x40) & ~0xfu;

		oc = vtx_outcode(oc);
		pv->Outcode = (ushort)oc;
		all &= oc;
	}

	SCR_PROJECTED_END = pv;
	return (int)all;
}

int M3dAsm_TransformAndOutcodeBlockVertices__FP7SVECTORiP6VECTOR(SVECTOR *v, int n, VECTOR *o) {
#ifdef R3D_HOOKS
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_BLOCK, (R3dTransformFn)transform_vertices, v, n, o);
#else
	return transform_vertices(v, n, o);
#endif
}

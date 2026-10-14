/* M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX (0x8008b3d4) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "m3dmaths.h"

/* libgte's sine/cosine table: 4096 steps per turn, 4.12 values */
struct SinCos {
	short sin, cos;
};

extern struct SinCos rcossin_tbl[4096];

/* The pairs (a, b) and (c, d) times (x, y): (a*x + b*y) >> 12 and (c*x + d*y) >> 12, each
 * saturated to 16 bits (retail: two rows of the GTE's rotation matrix, rtv0). */
static void rot2(int a, int b, int c, int d, int x, int y, int *p, int *q) {
	int16_t m[3][3] = { { 0 } };
	GeomAcc r;

	m[0][0] = (int16_t)a;
	m[0][1] = (int16_t)b;
	m[2][0] = (int16_t)c;
	m[2][1] = (int16_t)d;
	r = geom_mvmva(m, 0, (int16_t)x, (int16_t)y, 0, 12, 0, 0);
	*p = r.ir[0];
	*q = r.ir[2];
}

/* Builds the rotation matrix for Euler angles r (vx, vy, vz in 4096ths of a turn) applied in
 * Y-X-Z order, M = Ry * Rx * Rz, in 4.12. Each product is (a*b + c*d) >> 12, saturated to 16
 * bits. The translation of m is not touched. */
void M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX(SVECTOR *r, MATRIX *m) {
	struct SinCos x, y, z;
	int sxsz, cxsz, sxcz, cxcz;

	x = rcossin_tbl[r->vx & 0xfff];
	z = rcossin_tbl[r->vz & 0xfff];

	/* (sx, cx) and (cx, 0): sin x and cos x times sin z, then times cos z */
	rot2(x.sin, x.cos, x.cos, 0, z.sin, 0, &sxsz, &cxsz);

	y = rcossin_tbl[r->vy & 0xfff];
	rot2(x.sin, x.cos, x.cos, 0, z.cos, 0, &sxcz, &cxcz);

	/* (sy, cy) and (cy, -sy): the Y rotation applied to the X*Z terms */
	{
		int m00, m20, m01, m21, m02, m22;

		/* sy*sx*sz + cy*cz, cy*sx*sz - sy*cz; sy*sx*cz - cy*sz, cy*sx*cz + sy*sz; sy*cx, cy*cx */
		rot2(y.sin, y.cos, y.cos, -y.sin, sxsz, z.cos, &m00, &m20);
		rot2(y.sin, y.cos, y.cos, -y.sin, sxcz, -z.sin, &m01, &m21);
		rot2(y.sin, y.cos, y.cos, -y.sin, x.cos, 0, &m02, &m22);

		m->m[0][0] = m00;
		m->m[0][1] = m01;
		m->m[0][2] = m02;
		m->m[1][0] = cxsz;
		m->m[1][1] = cxcz;
		m->m[1][2] = -x.sin;
		m->m[2][0] = m20;
		m->m[2][1] = m21;
		m->m[2][2] = m22;
	}

}

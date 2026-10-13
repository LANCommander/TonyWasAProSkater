/* M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX (0x8008b3d4) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* libgte's sine/cosine table: 4096 steps per turn, 4.12 values */
struct SinCos {
	short sin, cos;
};

extern struct SinCos rcossin_tbl[4096];

/* V0 = (x, y, 0) through the rotation matrix (rtv0); returns IR1 and IR3. Only rows 1 and 3 of
 * the rotation matrix, and only their first two entries (R11 R12, R31 R32), matter here. */
static void rot2(int x, int y, int *ir1, int *ir3) {
	PC_GteSetData(0, m3d_pack(x, y));
	gte_rtv0();

	*ir1 = (int)PC_GteGetData(9);
	*ir3 = (int)PC_GteGetData(11);
}

/* Builds the rotation matrix for Euler angles r (vx, vy, vz in 4096ths of a turn) applied in
 * Y-X-Z order, M = Ry * Rx * Rz, in 4.12. The products are done on the GTE, two rows of the
 * rotation matrix at a time (each rtv0 product is (a*b + c*d) >> 12, saturated to 16 bits);
 * the caller's R11R12 and R31R32 are saved and restored, the GTE's V0/IR/MAC are left with the
 * last product. The translation of m is not touched. */
void M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX(SVECTOR *r, MATRIX *m) {
	uint32_t saved_r11r12 = PC_GteGetCtrl(0), saved_r31r32 = PC_GteGetCtrl(3);
	struct SinCos x, y, z;
	int sxsz, cxsz, sxcz, cxcz;

	PC_GteSetData(1, 0); /* VZ0 */

	x = rcossin_tbl[r->vx & 0xfff];
	z = rcossin_tbl[r->vz & 0xfff];

	/* rows (sx, cx) and (cx, 0): sin x and cos x times sin z, then times cos z */
	PC_GteSetCtrl(0, m3d_pack(x.sin, x.cos));
	PC_GteSetCtrl(3, (unsigned short)x.cos);
	rot2(z.sin, 0, &sxsz, &cxsz);

	y = rcossin_tbl[r->vy & 0xfff];
	rot2(z.cos, 0, &sxcz, &cxcz);

	/* rows (sy, cy) and (cy, -sy): the Y rotation applied to the X*Z terms */
	PC_GteSetCtrl(0, m3d_pack(y.sin, y.cos));
	PC_GteSetCtrl(3, m3d_pack(y.cos, -y.sin));

	{
		int m00, m20, m01, m21, m02, m22;

		rot2(sxsz, z.cos, &m00, &m20);  /* sy*sx*sz + cy*cz,  cy*sx*sz - sy*cz */
		rot2(sxcz, -z.sin, &m01, &m21); /* sy*sx*cz - cy*sz,  cy*sx*cz + sy*sz */
		rot2(x.cos, 0, &m02, &m22);     /* sy*cx,             cy*cx */

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

	PC_GteSetCtrl(0, saved_r11r12);
	PC_GteSetCtrl(3, saved_r31r32);
}

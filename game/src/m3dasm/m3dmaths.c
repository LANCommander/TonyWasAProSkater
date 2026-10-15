/* M3DMATHS.S: C interpretations of the hand-written matrix and vector maths (declarations in
   game/m3dasm.h): rotation/lighting matrix products, RotMatrixYXZ, scaling, transposes, square
   roots and the "last vector" helpers. Plain C, not byte-matching by design (reimpl.tsv). */
#include "m3dmaths.h"
#include <sys/types.h>
#include <libgte.h>
#include "lastvec.h"

/* libgte's sine/cosine table: 4096 steps per turn, 4.12 values */
struct SinCos {
	short sin, cos;
};
extern struct SinCos rcossin_tbl[4096];
/* The fields up to the one this reads (see struct Item in game/include/game/m3d.h). */
struct Item {
	ushort Flags;          /* 0x00 */
	ushort Inquiry;        /* 0x02 */
	int Position[3];       /* 0x04 */
	short Angles[3];       /* 0x10 */
	ushort Model;          /* 0x16 */
	u_char Frame;          /* 0x18 */
	u_char TintIndex;      /* 0x19 */
	u_char Anim;           /* 0x1a */
	u_char Region;         /* 0x1b */
	struct Item *NextItem; /* 0x1c */
	uint32_t RGB;          /* 0x20 */
	short Scale[3];        /* 0x24: 4.12 per-axis scale */
};
/* Square-root table: entry i (1024 <= i < 4096) is the root of i << 20, for the top 12 bits of
 * a normalised input. Retail addresses it from 0x800c7490 (index 0); indices below 1024 are
 * never used, so the table proper starts at 0x800c7c90. */
extern unsigned short SquareRootTable[3072];
/* The M3DMATHS "last vector" routines keep their working vector between calls (lastvec.h). */
GeomAcc M3dMaths_LastVec;

/* M3dMaths_SetIdentityRotation__FP6MATRIX (0x8008b238) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* Sets a matrix's rotation to the identity (1.0 = 4096 on the diagonal). The translation is left
 * alone; the padding halfword after m[2][2] is cleared (retail writes it as one word). */
void M3dMaths_SetIdentityRotation__FP6MATRIX(MATRIX *m) {
	int r, c;

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			m->m[r][c] = r == c ? 4096 : 0;

	m3d_store_m22_word(m, 4096);
}

/* M3dMaths_MulRotMatrix__FP6MATRIXT0 (0x8008b254) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* dst = R * src, where R is the current rotation (4.12). Only the 3x3 parts are involved; src may
 * be dst (retail reads all of src first). Retail stores m[2][2] as the whole third sum, so the
 * padding halfword after it gets its high bits. */
void M3dMaths_MulRotMatrix__FP6MATRIXT0(MATRIX *src, MATRIX *dst) {
	short a[3][3], out[3][3];
	uint32_t mac3;
	int r, c;

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			a[r][c] = src->m[r][c];

	mac3 = m3d_mul_matrix_ir(pc_geom.rot, a, out);

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			dst->m[r][c] = out[r][c];

	m3d_store_m22_word(dst, mac3);
}

/* M3dMaths_MulLcmMatrix__FP6MATRIXT0 (0x8008b314) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* dst = LCM * src, where LCM is the current light colour matrix (4.12). Same shape as
 * M3dMaths_MulRotMatrix: src may be dst, and m[2][2] is stored as the whole third sum (the padding
 * halfword gets its high bits). */
void M3dMaths_MulLcmMatrix__FP6MATRIXT0(MATRIX *src, MATRIX *dst) {
	short a[3][3], out[3][3];
	uint32_t mac3;
	int r, c;

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			a[r][c] = src->m[r][c];

	mac3 = m3d_mul_matrix_ir(pc_geom.lcm, a, out);

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			dst->m[r][c] = out[r][c];

	m3d_store_m22_word(dst, mac3);
}

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

/* M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX (0x8008b3d4) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
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

/* M3dMaths_ScaleMatrix__FP4ItemP6MATRIX (0x8008b4e0) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* Scales the columns of m's rotation by the item's per-axis scale: column c is multiplied by
 * Scale[c] (4.12, >> 12, saturated to 16 bits). */
void M3dMaths_ScaleMatrix__FP4ItemP6MATRIX(struct Item *item, MATRIX *m) {
	int c, r;

	for (c = 0; c < 3; c++) {
		int16_t col[3];
		GeomAcc a;

		for (r = 0; r < 3; r++)
			col[r] = m->m[r][c];

		a = geom_gpf(item->Scale[c], col, 12, 0, 0, 0, 0);

		for (r = 0; r < 3; r++)
			m->m[r][c] = a.ir[r];
	}

}

/* M3dMaths_TransposeMatrix__FP6MATRIX (0x8008b5a4) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Transposes a matrix's 3x3 rotation in place (the translation is left alone). */
void M3dMaths_TransposeMatrix__FP6MATRIX(MATRIX *m) {
	int i, j;

	for (i = 0; i < 3; i++)
		for (j = i + 1; j < 3; j++) {
			short t = m->m[i][j];

			m->m[i][j] = m->m[j][i];
			m->m[j][i] = t;
		}
}

/* M3dMaths_SquareRoot0__Fi (0x8008b5dc) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* Integer square root by table lookup. The leading-zero count gives the shift that normalises
 * n by an even amount; the top 12 bits of the normalised value index the table and the result
 * is shifted back by half that amount. Negative n (no normalisation) looks up its top 12 bits
 * directly. */
int M3dMaths_SquareRoot0__Fi(int n) {
	unsigned shift, top12;

	if (n < 0)
		return SquareRootTable[((unsigned)n >> 20) - 1024];

	if (n == 0)
		return 0; /* retail reads index 0 and shifts it right by 16: always 0 */

	shift = (unsigned)geom_lzc(n) & ~1u;
	top12 = ((unsigned)n << shift) >> 20;
	return SquareRootTable[top12 - 1024] >> (shift / 2);
}

/* M3dMaths_MulDiv64__Fiii (0x8008b62c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* a * b / c with a 64-bit intermediate, all unsigned (multu): the 64-bit product is divided by c
 * with a 32-step restoring shift-subtract division that keeps a 32-bit remainder. That is the
 * exact quotient when the product's high word is below c and c < 2^31; outside that range the
 * quotient bits come out as the loop below produces them (remainder bits shifted out are lost;
 * c == 0 gives 0xffffffff). */
int M3dMaths_MulDiv64__Fiii(int a, int b, int c) {
	unsigned long long p = (unsigned long long)(unsigned)a * (unsigned)b;
	unsigned rem = (unsigned)(p >> 32), lo = (unsigned)p, q = 0;
	int i;

	for (i = 0; i < 32; i++) {
		rem = (rem << 1) | (lo >> 31);
		lo <<= 1;
		q <<= 1;

		if (rem >= (unsigned)c) {
			rem -= (unsigned)c;
			q |= 1;
		}
	}

	return (int)q;
}

/* M3dMaths_ZeroMat__FPUs (0x8008b670) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Clears a matrix's nine rotation halfwords (the translation is left alone). */
void M3dMaths_ZeroMat__FPUs(unsigned short *m) {
	int i;

	for (i = 0; i < 9; i++)
		m[i] = 0;
}

/* M3dMaths_CopyMat__FPUsT0 (0x8008b68c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Copies a matrix's nine rotation halfwords from src to dst (the translation is not copied).
 * Retail reads all of src before writing dst. */
void M3dMaths_CopyMat__FPUsT0(unsigned short *src, unsigned short *dst) {
	unsigned short tmp[9];
	int i;

	for (i = 0; i < 9; i++)
		tmp[i] = src[i];

	for (i = 0; i < 9; i++)
		dst[i] = tmp[i];
}

/* M3dMaths_TransposeMat__FPUsT0 (0x8008b6bc) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Writes the transpose of src's 3x3 rotation (nine halfwords, row-major) to dst. Retail reads
 * all of src first, so src == dst transposes in place. */
void M3dMaths_TransposeMat__FPUsT0(unsigned short *src, unsigned short *dst) {
	unsigned short tmp[3][3];
	int r, c;

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			tmp[r][c] = src[r * 3 + c];

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			dst[r * 3 + c] = tmp[c][r];
}

/* M3dMaths_TransformLastVec__FPUs (0x8008b71c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Multiplies the last vector by the matrix m (nine halfwords, 4.12), which becomes the current
 * rotation: last vector = m * last vector >> 12.
 * test_asm: lastvec ir */
void M3dMaths_TransformLastVec__FPUs(unsigned short *m) {
	geom_load_matrix(pc_geom.rot, (const int16_t *)m);
	M3dMaths_LastVec = geom_mvmva(pc_geom.rot, 0, M3dMaths_LastVec.ir[0],
	                              M3dMaths_LastVec.ir[1], M3dMaths_LastVec.ir[2], 12, 0, 0);
}

/* M3dMaths_TransformVec0__FPUs (0x8008b75c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Multiplies a vector by the matrix m (nine halfwords, 4.12), which becomes the current
 * rotation; the result becomes the last vector (lastvec.h). Retail took the vector from the
 * GTE's V0, which its callers had loaded; in the port they pass it in the last vector, which
 * makes this the same operation as M3dMaths_TransformLastVec.
 * test_asm: lastvec v0 */
void M3dMaths_TransformVec0__FPUs(unsigned short *m) {
	geom_load_matrix(pc_geom.rot, (const int16_t *)m);
	M3dMaths_LastVec = geom_mvmva(pc_geom.rot, 0, M3dMaths_LastVec.ir[0],
	                              M3dMaths_LastVec.ir[1], M3dMaths_LastVec.ir[2], 12, 0, 0);
}

/* M3dMaths_LastVecMagnitude__Fv (0x8008b79c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Length of the last vector (lastvec.h): squares the components, which also leaves the squares
 * as the last vector (clamped to 0..0x7fff, the sums unclamped), and returns the square root of
 * their sum (retail tail-jumps into M3dMaths_SquareRoot0).
 * test_asm: lastvec ir */
int M3dMaths_LastVecMagnitude__Fv(void) {
	M3dMaths_LastVec = geom_sqr(M3dMaths_LastVec.ir, 0, 1, 0);
	return M3dMaths_SquareRoot0__Fi((int)((uint32_t)M3dMaths_LastVec.mac[0] +
	                                      (uint32_t)M3dMaths_LastVec.mac[1] +
	                                      (uint32_t)M3dMaths_LastVec.mac[2]));
}

/* M3dMaths_CrossWithLastVec12__FP7SVector (0x8008b7b8) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Cross product of v with the last vector (lastvec.h), in 4.12, as the new last vector:
 * (v.y*l.z - v.z*l.y, v.z*l.x - v.x*l.z, v.x*l.y - v.y*l.x) >> 12. Retail fed v to the GTE through
 * the rotation matrix's diagonal, which the state keeps: rot[0][0], [1][1], [2][2] = v,
 * rot[0][1] = rot[1][2] = 0.
 * test_asm: lastvec ir */
void M3dMaths_CrossWithLastVec12__FP7SVector(struct SVector *v) {
	int16_t d[3];

	geom_load_op_diag(&pc_geom, (unsigned short)v->vx, (unsigned short)v->vy,
	                  (unsigned short)v->vz);
	geom_op_diag(&pc_geom, d);
	M3dMaths_LastVec = geom_op(d, M3dMaths_LastVec.ir, 12, 0, 0);
}

/* MIPS div: never traps. Division by zero gives lo = -1 for n >= 0 and 1 for n < 0. */
static int mips_div(int n, int d) {
	if (d == 0)
		return n < 0 ? 1 : -1;

	return n / d;
}

/* M3dMaths_NormaliseLastVec__Fv (0x8008b7ec) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Scales the last vector (lastvec.h) to unit length in 4.12 fixed point: each component
 * becomes (c << 12) / |v| (16 bits), with |v| from M3dMaths_LastVecMagnitude, whose sums (the
 * squares) stay.
 * test_asm: lastvec ir */
void M3dMaths_NormaliseLastVec__Fv(void) {
	int v[3], mag, i;

	for (i = 0; i < 3; i++)
		v[i] = M3dMaths_LastVec.ir[i];

	mag = M3dMaths_LastVecMagnitude__Fv();

	for (i = 0; i < 3; i++)
		M3dMaths_LastVec.ir[i] = (int16_t)mips_div((int)((unsigned)v[i] << 12), mag);
}

/* M3dMaths_ScaleLastVec12__Fi (0x8008b844) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Multiplies the last vector (lastvec.h) by the 4.12 factor k (16 bits): v = v * k >> 12.
 * test_asm: lastvec ir */
void M3dMaths_ScaleLastVec12__Fi(int k) {
	M3dMaths_LastVec = geom_gpf((int16_t)k, M3dMaths_LastVec.ir, 12, 0, 0, 0, 0);
}

/* M3dMaths_AddScaledLastVec12__FP7SVectori (0x8008b860) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* v + the last vector (lastvec.h) scaled by the 4.12 factor k (16 bits), as the new last
 * vector: ((v << 12) + last * k) >> 12, saturated to 16 bits.
 * test_asm: lastvec ir */
void M3dMaths_AddScaledLastVec12__FP7SVectori(struct SVector *v, int k) {
	int32_t base[3];

	base[0] = v->vx;
	base[1] = v->vy;
	base[2] = v->vz;
	M3dMaths_LastVec = geom_gpl((int16_t)k, M3dMaths_LastVec.ir, base, 12, 0, 0, 0, 0);
}

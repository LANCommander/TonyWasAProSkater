/* libgte (Psy-Q's geometry library) as the port's own C: the matrix and vector helpers, the
 * fixed-point trigonometry and square roots the game calls. 1.0 is 4096 throughout (ONE).
 *
 * The functions that use the GTE in the original library use it here too, through the software
 * GTE (pc_gte.h), with the same commands in the same order: results come out bit-identical
 * (saturation, the GTE's own rounding and the 16-bit IR registers included), and the GTE is left
 * holding what callers rely on, e.g. SetRotMatrix, ApplyMatrix*, MulMatrix and MulMatrix0 leave
 * their matrix loaded as the rotation matrix.
 *
 * The tables the library reads (the sine/cosine table, arctangent tables, square-root tables)
 * are in the PS1 image, which stays mapped at its addresses: they are read from there.
 *
 * Several functions are linked into the retail executable more than once; the build maps each
 * name to every address it has (build/psyq_names.tsv). */
#include <sys/types.h>
#include <stdint.h>

#include <libgte.h>
#include <inline_c.h>

#include "pc_gte.h"

/* rcossin_tbl: 4096 entries of {short sin, short cos} for angles 0..4095 (4096 = 360 degrees). */
#define RCOSSIN_TBL 0x800c9670u
/* ratan_tbl: 1025 shorts, the angle (1024 = 90 degrees) whose tangent is i/1024. */
#define RATAN_TBL 0x800ce704u
/* K_cor_tbl: 12 longs, the angle of atan(2^-i) for the CORDIC steps of catan. */
#define K_COR_TBL 0x800ce6d4u
/* SquareRoot0's table: shorts for normalised inputs 64..255, 4096 * sqrt(n / 64)-ish values;
 * the entry for n is at this address + (n - 64) * 2. */
#define SQRT_TBL 0x800d0a9cu
/* VectorNormal's table: shorts for normalised squared lengths 64..255, 4096 / sqrt-ish values;
 * the entry for n is at this address + (n - 64) * 2. */
#define RSQRT_TBL 0x800ce548u

static int16_t TableShort(uint32_t addr)
{
	return *(const int16_t *)(uintptr_t)addr;
}

static int32_t TableLong(uint32_t addr)
{
	return *(const int32_t *)(uintptr_t)addr;
}

/* Sine and cosine of a 4096-per-turn angle: the table holds 0..4095, a negative angle uses the
 * entry for its magnitude with the sine negated. */
static void SinCos(long a, int *s, int *c)
{
	uint32_t i = (a < 0 ? 0u - (uint32_t)a : (uint32_t)a) & 0xfff;
	uint32_t w = (uint32_t)TableLong(RCOSSIN_TBL + i * 4);
	*s = a < 0 ? -(int16_t)w : (int16_t)w;
	*c = (int16_t)(w >> 16);
}

/* The R3000's division: dividing by zero gives -1 (or 1 for a negative dividend) instead of
 * trapping, and INT_MIN / -1 gives INT_MIN. */
static int32_t MipsDiv(int32_t n, int32_t d)
{
	if (d == 0)
		return n < 0 ? 1 : -1;
	if (d == -1)
		return (int32_t)(0u - (uint32_t)n);
	return n / d;
}

/* ---- loading the GTE ----------------------------------------------------------------------- */

void SetRotMatrix(MATRIX *m)
{
	gte_SetRotMatrix(m);
}

void SetTransMatrix(MATRIX *m)
{
	gte_SetTransMatrix(m);
}

void SetGeomScreen(long h)
{
	PC_GteSetCtrl(26, (uint32_t)h);
}

/* The offsets are kept as 16.16 fixed point. */
void SetGeomOffset(long ofx, long ofy)
{
	PC_GteSetCtrl(24, (uint32_t)ofx << 16);
	PC_GteSetCtrl(25, (uint32_t)ofy << 16);
}

/* The far colour is kept with 4 fractional bits. */
void SetFarColor(long rfc, long gfc, long bfc)
{
	PC_GteSetCtrl(21, (uint32_t)rfc << 4);
	PC_GteSetCtrl(22, (uint32_t)gfc << 4);
	PC_GteSetCtrl(23, (uint32_t)bfc << 4);
}

/* Sets up the GTE: screen distance 1000, offset 0,0, the Z-average scales for 3 and 4 vertices
 * (1/3 and 1/4 of 1024), and the depth-cueing coefficients. On the PS1 it also hooks the GTE into
 * the BIOS exception handler and enables COP2, neither of which means anything here. */
void InitGeom(void)
{
	PC_GteSetCtrl(29, 341);                /* ZSF3 */
	PC_GteSetCtrl(30, 256);                /* ZSF4 */
	PC_GteSetCtrl(26, 1000);               /* H */
	PC_GteSetCtrl(27, (uint32_t)-4194);    /* DQA */
	PC_GteSetCtrl(28, 0x01400000u);        /* DQB */
	PC_GteSetCtrl(24, 0);                  /* OFX */
	PC_GteSetCtrl(25, 0);                  /* OFY */
}

/* ---- matrices -------------------------------------------------------------------------------- */

MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1)
{
	MATRIX t = *m0; /* m1 may be m0 */
	int i, j;
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			m1->m[i][j] = t.m[j][i];
	return m1;
}

MATRIX *TransMatrix(MATRIX *m, VECTOR *v)
{
	m->t[0] = v->vx;
	m->t[1] = v->vy;
	m->t[2] = v->vz;
	return m;
}

/* out = m0 * m1 (rotation parts only): m0 is loaded as the rotation matrix and each column of m1
 * goes through it as V0 (MVMVA rt*v0, >>12). All of m1 is read before out is written, so out may
 * be either input. The pad after m[2][2] receives the sign extension of the last element. The GTE
 * is left with m0 as its rotation matrix and the third column in V0/IR/MAC. */
static void MulRot(MATRIX *m0, MATRIX *m1, MATRIX *out)
{
	short col[3][3];
	int j;

	gte_SetRotMatrix(m0);
	for (j = 0; j < 3; j++) {
		PC_GteSetData(0, (uint16_t)m1->m[0][j] | (uint32_t)(uint16_t)m1->m[1][j] << 16);
		PC_GteSetData(1, (uint32_t)(int32_t)m1->m[2][j]);
		gte_rtv0();
		col[j][0] = (short)PC_GteGetData(9);
		col[j][1] = (short)PC_GteGetData(10);
		col[j][2] = (short)PC_GteGetData(11);
	}
	for (j = 0; j < 3; j++) {
		out->m[0][j] = col[j][0];
		out->m[1][j] = col[j][1];
		out->m[2][j] = col[j][2];
	}
	*(int32_t *)&out->m[2][2] = (int32_t)PC_GteGetData(11); /* stored as a word, pad included */
}

/* m0 = m0 * m1 */
MATRIX *MulMatrix(MATRIX *m0, MATRIX *m1)
{
	MulRot(m0, m1, m0);
	return m0;
}

/* m2 = m0 * m1 */
MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2)
{
	MulRot(m0, m1, m2);
	return m2;
}

/* v1 = m * v0 for short vectors (MVMVA rt*v0, >>12, saturated to 16 bits). */
SVECTOR *ApplyMatrixSV(MATRIX *m, SVECTOR *v0, SVECTOR *v1)
{
	gte_SetRotMatrix(m);
	gte_ldv0(v0);
	gte_rtv0();
	v1->vx = (short)PC_GteGetData(9);
	v1->vy = (short)PC_GteGetData(10);
	v1->vz = (short)PC_GteGetData(11);
	return v1;
}

/* Splits a 32-bit component into a high part (v / 32768) and a low part (the remaining 15 bits),
 * both with v's sign, so each fits the GTE's 16-bit IR registers. */
static void Split15(int32_t v, int32_t *hi, int32_t *lo)
{
	if (v < 0) {
		uint32_t n = 0u - (uint32_t)v;
		*hi = (int32_t)(0u - (uint32_t)((int32_t)n >> 15));
		*lo = (int32_t)(0u - (n & 0x7fff));
	} else {
		*hi = v >> 15;
		*lo = v & 0x7fff;
	}
}

/* v1 = m * v0 for long vectors: the high parts go through the GTE unshifted (rt*ir, sf=0), the
 * low parts shifted by 12 (sf=1), and the high result, now in units of 2^15 / 2^12 = 8, is added
 * back in. */
VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1)
{
	int32_t hi[3], lo[3], mac_hi[3];
	int i;

	gte_SetRotMatrix(m);
	Split15(v0->vx, &hi[0], &lo[0]);
	Split15(v0->vy, &hi[1], &lo[1]);
	Split15(v0->vz, &hi[2], &lo[2]);
	for (i = 0; i < 3; i++)
		PC_GteSetData(9 + i, (uint32_t)hi[i]);
	gte_rtir_sf0();
	for (i = 0; i < 3; i++)
		mac_hi[i] = (int32_t)PC_GteGetData(25 + i);
	for (i = 0; i < 3; i++)
		PC_GteSetData(9 + i, (uint32_t)lo[i]);
	gte_rtir();
	v1->vx = (long)(PC_GteGetData(25) + ((uint32_t)mac_hi[0] << 3));
	v1->vy = (long)(PC_GteGetData(26) + ((uint32_t)mac_hi[1] << 3));
	v1->vz = (long)(PC_GteGetData(27) + ((uint32_t)mac_hi[2] << 3));
	return v1;
}

/* ---- rotation matrices ------------------------------------------------------------------------ */

/* m = Ry * Rx * Rz (rotation part only), computed with the CPU. */
MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m)
{
	int sx, cx, sy, cy, sz, cz, t;

	SinCos(r->vx, &sx, &cx);
	SinCos(r->vy, &sy, &cy);
	m->m[1][2] = (short)-sx;
	m->m[0][2] = (short)((sy * cx) >> 12);
	m->m[2][2] = (short)((cy * cx) >> 12);
	SinCos(r->vz, &sz, &cz);
	m->m[1][0] = (short)((sz * cx) >> 12);
	m->m[1][1] = (short)((cz * cx) >> 12);
	t = (sy * sx) >> 12;
	m->m[0][0] = (short)(((cy * cz) >> 12) + ((t * sz) >> 12));
	m->m[0][1] = (short)(((t * cz) >> 12) - ((cy * sz) >> 12));
	t = (cy * sx) >> 12;
	m->m[2][1] = (short)(((sy * sz) >> 12) + ((t * cz) >> 12));
	m->m[2][0] = (short)(((t * sz) >> 12) - ((sy * cz) >> 12));
	return m;
}

/* The same matrix as RotMatrixYXZ, with the products done on the GTE (GPF, >>12) in four
 * batches; the GTE is left with the last batch's IR/MAC. */
MATRIX *RotMatrixYXZ_gte(SVECTOR *r, MATRIX *m)
{
	int sx, cx, sy, cy, sz, cz;
	int32_t cysx, cysz, cycz, sysx, sysz, sycz, czcx, czsysx, czcysx, szcx, szsysx, szcysx;
	int32_t cxcy, cxsy;

	SinCos(r->vz, &sz, &cz);
	SinCos(r->vy, &sy, &cy);
	SinCos(r->vx, &sx, &cx);

	PC_GteSetData(8, (uint32_t)cy);
	PC_GteSetData(9, (uint32_t)sx);
	PC_GteSetData(10, (uint32_t)sz);
	PC_GteSetData(11, (uint32_t)cz);
	gte_gpf12();
	cxcy = cx * cy;
	cysx = (int32_t)PC_GteGetData(9);
	cysz = (int32_t)PC_GteGetData(10);
	cycz = (int32_t)PC_GteGetData(11);

	PC_GteSetData(8, (uint32_t)sy);
	PC_GteSetData(9, (uint32_t)sx);
	PC_GteSetData(10, (uint32_t)sz);
	PC_GteSetData(11, (uint32_t)cz);
	gte_gpf12();
	m->m[2][2] = (short)(cxcy >> 12);
	sysx = (int32_t)PC_GteGetData(9);
	sysz = (int32_t)PC_GteGetData(10);
	sycz = (int32_t)PC_GteGetData(11);

	PC_GteSetData(8, (uint32_t)cz);
	PC_GteSetData(9, (uint32_t)cx);
	cxsy = cx * sy;
	PC_GteSetData(10, (uint32_t)sysx);
	PC_GteSetData(11, (uint32_t)cysx);
	gte_gpf12();
	czcx = (int32_t)PC_GteGetData(9);
	czsysx = (int32_t)PC_GteGetData(10);
	czcysx = (int32_t)PC_GteGetData(11);

	PC_GteSetData(8, (uint32_t)sz);
	PC_GteSetData(9, (uint32_t)cx);
	PC_GteSetData(10, (uint32_t)sysx);
	PC_GteSetData(11, (uint32_t)cysx);
	gte_gpf12();
	szcx = (int32_t)PC_GteGetData(9);
	szsysx = (int32_t)PC_GteGetData(10);
	szcysx = (int32_t)PC_GteGetData(11);

	m->m[1][1] = (short)czcx;
	m->m[1][2] = (short)-sx;
	m->m[0][0] = (short)(cycz + szsysx);
	m->m[0][1] = (short)(czsysx - cysz);
	m->m[0][2] = (short)(cxsy >> 12);
	m->m[1][0] = (short)szcx;
	m->m[2][0] = (short)(szcysx - sycz);
	m->m[2][1] = (short)(czcysx + sysz);
	return m;
}

/* m = Ry(r) * m: rows 0 and 2 are rotated, row 1 and the translation are unchanged. */
MATRIX *RotMatrixY(long r, MATRIX *m)
{
	int s, c, j;
	int32_t row0[3], row2[3];

	SinCos(r, &s, &c);
	for (j = 0; j < 3; j++) {
		row0[j] = m->m[0][j];
		row2[j] = m->m[2][j];
	}
	for (j = 0; j < 3; j++) {
		m->m[0][j] = (short)((int32_t)((uint32_t)(c * row0[j]) + (uint32_t)(s * row2[j])) >> 12);
		m->m[2][j] = (short)((int32_t)((uint32_t)(c * row2[j]) - (uint32_t)(s * row0[j])) >> 12);
	}
	return m;
}

/* ---- normalising ------------------------------------------------------------------------------ */

/* Scales (x, y, z) to length 4096 and returns the squared length. The squares are summed on the
 * GTE (SQR, so only the low 16 bits of each component count), the sum is normalised by its
 * leading-zero count (LZCS/LZCR) to an index into a 1/sqrt table, the components are multiplied
 * by that (GPF) and shifted back by half the normalisation. */
static long Normalise(int32_t v[3])
{
	uint32_t sum, lz, m;
	int shift, i;

	for (i = 0; i < 3; i++)
		PC_GteSetData(9 + i, (uint32_t)v[i]);
	gte_sqr0();
	sum = PC_GteGetData(25) + PC_GteGetData(26) + PC_GteGetData(27);
	PC_GteSetData(30, sum);
	lz = PC_GteGetData(31) & ~1u;
	shift = (int32_t)(31 - lz) >> 1;
	if ((int32_t)(lz - 24) >= 0)
		m = sum << ((lz - 24) & 31);
	else
		m = (uint32_t)((int32_t)sum >> ((24 - lz) & 31));
	PC_GteSetData(8, (uint32_t)(int32_t)TableShort(RSQRT_TBL + (m - 64) * 2));
	for (i = 0; i < 3; i++)
		PC_GteSetData(9 + i, (uint32_t)v[i]);
	gte_gpf0();
	for (i = 0; i < 3; i++)
		v[i] = (int32_t)PC_GteGetData(25 + i) >> (shift & 31);
	return (long)sum;
}

long VectorNormal(VECTOR *v0, VECTOR *v1)
{
	int32_t v[3] = { v0->vx, v0->vy, v0->vz };
	long sum = Normalise(v);
	v1->vx = v[0];
	v1->vy = v[1];
	v1->vz = v[2];
	return sum;
}

long VectorNormalSS(SVECTOR *v0, SVECTOR *v1)
{
	int32_t v[3] = { v0->vx, v0->vy, v0->vz };
	long sum = Normalise(v);
	v1->vx = (short)v[0];
	v1->vy = (short)v[1];
	v1->vz = (short)v[2];
	return sum;
}

/* Integer square root: a is normalised by its leading-zero count (on the GTE, LZCS/LZCR) to an
 * index 64..255 into a table, and the table value shifted back by half the normalisation. */
long SquareRoot0(long a)
{
	uint32_t lz, m;
	int shift;

	PC_GteSetData(30, (uint32_t)a);
	lz = PC_GteGetData(31);
	if (lz == 32)
		return 0;
	lz &= ~1u;
	shift = (int32_t)(31 - lz) >> 1;
	if ((int32_t)(lz - 24) >= 0)
		m = (uint32_t)a << ((lz - 24) & 31);
	else
		m = (uint32_t)((int32_t)a >> ((24 - lz) & 31));
	return (long)(((uint32_t)(int32_t)TableShort(SQRT_TBL + (m - 64) * 2) << (shift & 31)) >> 12);
}

/* ---- arctangents ------------------------------------------------------------------------------ */

/* Arctangent of a / 4096 (4096 = 360 degrees) by 12 CORDIC steps: the vector (4096, a) is turned
 * towards the x axis by +-atan(2^-i) until y is 0, summing the angles turned. */
int catan(int a)
{
	int32_t x = 4096, y = a, z = 0, nx;
	int i;

	for (i = 0; i < 12; i++) {
		int32_t k = TableLong(K_COR_TBL + i * 4);
		if (y < 0) {
			nx = (int32_t)((uint32_t)x - (uint32_t)(y >> i));
			y = (int32_t)((uint32_t)y + (uint32_t)(x >> i));
			z = (int32_t)((uint32_t)z - (uint32_t)k);
		} else {
			nx = (int32_t)((uint32_t)x + (uint32_t)(y >> i));
			y = (int32_t)((uint32_t)y - (uint32_t)(x >> i));
			z = (int32_t)((uint32_t)z + (uint32_t)k);
		}
		x = nx;
	}
	return z;
}

/* Angle of (x, y) (4096 = 360 degrees, 0 along +x, 1024 along +y), by looking up the ratio of the
 * smaller to the larger magnitude, scaled to 0..1024, in ratan_tbl and folding by octant. Large
 * values are divided by the shifted divisor instead of shifting the dividend, to avoid overflow. */
long ratan2(long y, long x)
{
	int xneg = 0, yneg = 0;
	int32_t ax = x, ay = y, q, r;

	if (ax < 0) {
		xneg = 1;
		ax = (int32_t)(0u - (uint32_t)ax);
	}
	if (ay < 0) {
		yneg = 1;
		ay = (int32_t)(0u - (uint32_t)ay);
	}
	if (ax == 0 && ay == 0)
		return 0;
	if (ay < ax) {
		if (ay & 0x7fe00000)
			q = MipsDiv(ay, ax >> 10);
		else
			q = MipsDiv((int32_t)((uint32_t)ay << 10), ax);
		r = TableShort(RATAN_TBL + (uint32_t)q * 2);
	} else {
		if (ax & 0x7fe00000)
			q = MipsDiv(ax, ay >> 10);
		else
			q = MipsDiv((int32_t)((uint32_t)ax << 10), ay);
		r = 1024 - TableShort(RATAN_TBL + (uint32_t)q * 2);
	}
	if (xneg)
		r = 2048 - r;
	if (yneg)
		r = -r;
	return r;
}

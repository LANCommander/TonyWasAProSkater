/* libgte (Psy-Q's geometry library) as the port's own C: the matrix and vector helpers, the
 * fixed-point trigonometry and square roots the game calls. 1.0 is 4096 throughout (ONE).
 *
 * The game calls these by their Psy-Q names; the build points those calls here (build.py
 * NATIVE_RENAME: SetRotMatrix -> PC_SetRotMatrix ...). Where the original library used the GTE,
 * this uses the same operations of pc_geom.h in the same order, so results are bit-identical
 * (saturation and the 16-bit intermediate results included), and the state is left as the
 * library leaves it: SetRotMatrix, ApplyMatrix*, MulMatrix and MulMatrix0 leave their matrix as
 * the current rotation.
 *
 * The tables the library reads (the sine/cosine table, arctangent tables, square-root tables)
 * are in the PS1 image, which stays mapped at its addresses: they are read from there.
 *
 * The original's working values in the GTE's data registers are not kept: no caller reads them. */
#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>

#include <libgte.h>

#include "pc_geom.h"
#include "pc_runtime.h"

/* rcossin_tbl: 4096 entries of {short sin, short cos} for angles 0..4095 (4096 = 360 degrees). */
extern short rcossin_tbl[];
#define RCOSSIN_TBL ((uintptr_t)rcossin_tbl)
/* ratan_tbl: 1025 shorts, the angle (1024 = 90 degrees) whose tangent is i/1024. */
extern short ratan_tbl[];
#define RATAN_TBL ((uintptr_t)ratan_tbl)
/* K_cor_tbl: 12 longs, the angle of atan(2^-i) for the CORDIC steps of catan. */
extern int K_cor_tbl[];
#define K_COR_TBL ((uintptr_t)K_cor_tbl)
/* SquareRoot0's table: shorts for normalised inputs 64..255, 4096 * sqrt(n / 64)-ish values;
 * the entry for n is at this address + (n - 64) * 2. */
extern unsigned short sqrt_tbl[];
#define SQRT_TBL ((uintptr_t)sqrt_tbl)
/* VectorNormal's table: shorts for normalised squared lengths 64..255, 4096 / sqrt-ish values;
 * the entry for n is at this address + (n - 64) * 2. */
extern unsigned short rsqrt_tbl[];
#define RSQRT_TBL ((uintptr_t)rsqrt_tbl)

static int16_t TableShort(uintptr_t addr)
{
	return *(const int16_t *)(uintptr_t)addr;
}

static int32_t TableLong(uintptr_t addr)
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

/* ---- the state ------------------------------------------------------------------------------ */

void PC_SetRotMatrix(MATRIX *m)
{
	geom_load_matrix(pc_geom.rot, &m->m[0][0]);
}

void PC_SetTransMatrix(MATRIX *m)
{
	geom_load_vector(pc_geom.tr, m->t[0], m->t[1], m->t[2]);
}

void PC_SetGeomScreen(long h)
{
	pc_geom.h = (uint16_t)h;
}

/* The offsets are kept as 16.16 fixed point. */
void PC_SetGeomOffset(long ofx, long ofy)
{
	pc_geom.ofx = (int32_t)((uint32_t)ofx << 16);
	pc_geom.ofy = (int32_t)((uint32_t)ofy << 16);
}

/* The far colour is kept with 4 fractional bits. */
void PC_SetFarColor(long rfc, long gfc, long bfc)
{
	geom_load_vector(pc_geom.fc, (int32_t)((uint32_t)rfc << 4), (int32_t)((uint32_t)gfc << 4),
	                 (int32_t)((uint32_t)bfc << 4));
}

/* Screen distance 1000, offset 0,0, the Z-average scales for 3 and 4 vertices (1/3 and 1/4 of
 * 1024), and the depth-cueing coefficients. On the PS1 it also hooks the GTE into the BIOS
 * exception handler and enables COP2. */
void PC_InitGeom(void)
{
	pc_geom.zsf3 = 341;
	pc_geom.zsf4 = 256;
	pc_geom.h = 1000;
	pc_geom.dqa = -4194;
	pc_geom.dqb = 0x01400000;
	pc_geom.ofx = 0;
	pc_geom.ofy = 0;
}

/* ---- matrices -------------------------------------------------------------------------------- */

MATRIX *PC_TransposeMatrix(MATRIX *m0, MATRIX *m1)
{
	MATRIX t = *m0; /* m1 may be m0 */
	int i, j;
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			m1->m[i][j] = t.m[j][i];
	return m1;
}

MATRIX *PC_TransMatrix(MATRIX *m, VECTOR *v)
{
	m->t[0] = v->vx;
	m->t[1] = v->vy;
	m->t[2] = v->vz;
	return m;
}

/* out = m0 * m1 (rotation parts only): m0 becomes the current rotation and each column of m1 goes
 * through it (>> 12, saturated to 16 bits). All of m1 is read before out is written, so out may
 * be either input. The pad after m[2][2] receives the sign extension of the last element. */
static void MulRot(MATRIX *m0, MATRIX *m1, MATRIX *out)
{
	GeomAcc col[3];
	int j;

	geom_load_matrix(pc_geom.rot, &m0->m[0][0]);
	for (j = 0; j < 3; j++)
		col[j] = geom_mvmva(pc_geom.rot, NULL, m1->m[0][j], m1->m[1][j], m1->m[2][j], 12, 0, NULL);
	for (j = 0; j < 3; j++) {
		out->m[0][j] = col[j].ir[0];
		out->m[1][j] = col[j].ir[1];
		out->m[2][j] = col[j].ir[2];
	}
	*(int32_t *)&out->m[2][2] = col[2].ir[2]; /* stored as a word, pad included */
}

/* m0 = m0 * m1 */
MATRIX *PC_MulMatrix(MATRIX *m0, MATRIX *m1)
{
	MulRot(m0, m1, m0);
	return m0;
}

/* m2 = m0 * m1 */
MATRIX *PC_MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2)
{
	MulRot(m0, m1, m2);
	return m2;
}

/* v1 = m * v0 for short vectors (>> 12, saturated to 16 bits); m becomes the current rotation. */
SVECTOR *PC_ApplyMatrixSV(MATRIX *m, SVECTOR *v0, SVECTOR *v1)
{
	GeomAcc a;

	geom_load_matrix(pc_geom.rot, &m->m[0][0]);
	a = geom_mvmva(pc_geom.rot, NULL, v0->vx, v0->vy, v0->vz, 12, 0, NULL);
	v1->vx = a.ir[0];
	v1->vy = a.ir[1];
	v1->vz = a.ir[2];
	return v1;
}

/* Splits a 32-bit component into a high part (v / 32768) and a low part (the remaining 15 bits),
 * both with v's sign, so each fits in 16 bits (the high part is truncated to 16 bits where it
 * doesn't, as in the original). */
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

/* v1 = m * v0 for long vectors: the high parts go through m unshifted, the low parts shifted by
 * 12, and the high result, now in units of 2^15 / 2^12 = 8, is added back in. m becomes the
 * current rotation. */
VECTOR *PC_ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1)
{
	int32_t hi[3], lo[3];
	GeomAcc h, l;

	geom_load_matrix(pc_geom.rot, &m->m[0][0]);
	Split15(v0->vx, &hi[0], &lo[0]);
	Split15(v0->vy, &hi[1], &lo[1]);
	Split15(v0->vz, &hi[2], &lo[2]);
	h = geom_mvmva(pc_geom.rot, NULL, (int16_t)hi[0], (int16_t)hi[1], (int16_t)hi[2], 0, 0, NULL);
	l = geom_mvmva(pc_geom.rot, NULL, (int16_t)lo[0], (int16_t)lo[1], (int16_t)lo[2], 12, 0, NULL);
	v1->vx = (long)((uint32_t)l.mac[0] + ((uint32_t)h.mac[0] << 3));
	v1->vy = (long)((uint32_t)l.mac[1] + ((uint32_t)h.mac[1] << 3));
	v1->vz = (long)((uint32_t)l.mac[2] + ((uint32_t)h.mac[2] << 3));
	return v1;
}

/* ---- rotation matrices ------------------------------------------------------------------------ */

/* m = Ry * Rx * Rz (rotation part only), computed with the CPU. */
MATRIX *PC_RotMatrixYXZ(SVECTOR *r, MATRIX *m)
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

/* k * (a, b, c) >> 12, each saturated to 16 bits (the original's GTE GPF) */
static GeomAcc Scale3(int k, int a, int b, int c)
{
	int16_t v[3];
	v[0] = (int16_t)a;
	v[1] = (int16_t)b;
	v[2] = (int16_t)c;
	return geom_gpf((int16_t)k, v, 12, 0, 0, NULL, NULL);
}

/* The same matrix as RotMatrixYXZ, with the products saturated to 16 bits, in four batches. */
MATRIX *PC_RotMatrixYXZ_gte(SVECTOR *r, MATRIX *m)
{
	int sx, cx, sy, cy, sz, cz;
	int32_t cysx, cysz, cycz, sysx, sysz, sycz, czcx, czsysx, czcysx, szcx, szsysx, szcysx;
	int32_t cxcy, cxsy;
	GeomAcc a;

	SinCos(r->vz, &sz, &cz);
	SinCos(r->vy, &sy, &cy);
	SinCos(r->vx, &sx, &cx);

	a = Scale3(cy, sx, sz, cz);
	cxcy = cx * cy;
	cysx = a.ir[0];
	cysz = a.ir[1];
	cycz = a.ir[2];

	a = Scale3(sy, sx, sz, cz);
	m->m[2][2] = (short)(cxcy >> 12);
	sysx = a.ir[0];
	sysz = a.ir[1];
	sycz = a.ir[2];

	cxsy = cx * sy;
	a = Scale3(cz, cx, sysx, cysx);
	czcx = a.ir[0];
	czsysx = a.ir[1];
	czcysx = a.ir[2];

	a = Scale3(sz, cx, sysx, cysx);
	szcx = a.ir[0];
	szsysx = a.ir[1];
	szcysx = a.ir[2];

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
MATRIX *PC_RotMatrixY(long r, MATRIX *m)
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

/* Scales (x, y, z) to length 4096 and returns the squared length. Only the low 16 bits of each
 * component count (the original squares them on the GTE). The sum is normalised by its leading
 * zeros to an index into a 1/sqrt table, the components are multiplied by that and shifted back
 * by half the normalisation. */
static long Normalise(int32_t v[3])
{
	uint32_t sum, lz, m;
	int16_t s[3];
	int shift, i;
	GeomAcc a;

	for (i = 0; i < 3; i++)
		s[i] = (int16_t)v[i];
	a = geom_sqr(s, 0, 0, NULL);
	sum = (uint32_t)a.mac[0] + (uint32_t)a.mac[1] + (uint32_t)a.mac[2];
	lz = (uint32_t)geom_lzc((int32_t)sum) & ~1u;
	shift = (int32_t)(31 - lz) >> 1;
	if ((int32_t)(lz - 24) >= 0)
		m = sum << ((lz - 24) & 31);
	else
		m = (uint32_t)((int32_t)sum >> ((24 - lz) & 31));
	a = geom_gpf(TableShort(RSQRT_TBL + (m - 64) * 2), s, 0, 0, 0, NULL, NULL);
	for (i = 0; i < 3; i++)
		v[i] = a.mac[i] >> (shift & 31);
	return (long)sum;
}

long PC_VectorNormal(VECTOR *v0, VECTOR *v1)
{
	int32_t v[3] = { v0->vx, v0->vy, v0->vz };
	long sum = Normalise(v);
	v1->vx = v[0];
	v1->vy = v[1];
	v1->vz = v[2];
	return sum;
}

long PC_VectorNormalSS(SVECTOR *v0, SVECTOR *v1)
{
	int32_t v[3] = { v0->vx, v0->vy, v0->vz };
	long sum = Normalise(v);
	v1->vx = (short)v[0];
	v1->vy = (short)v[1];
	v1->vz = (short)v[2];
	return sum;
}

/* Integer square root: a is normalised by its leading zeros to an index 64..255 into a table, and
 * the table value shifted back by half the normalisation. */
long PC_SquareRoot0(long a)
{
	uint32_t lz, m;
	int shift;

	lz = (uint32_t)geom_lzc((int32_t)a);
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
int PC_catan(int a)
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
long PC_ratan2(long y, long x)
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

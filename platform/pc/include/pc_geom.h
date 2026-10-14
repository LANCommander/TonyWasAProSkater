/* The game's 3D maths: the fixed-point operations the PS1 did on its geometry coprocessor (GTE),
 * as plain C functions with explicit inputs and outputs. Results are bit-identical to the
 * hardware's (psx-spx "Geometry Transformation Engine"): 44-bit accumulators checked after every
 * addition, 16-bit intermediate results saturated, the UNR reciprocal for perspective, and the
 * same FLAG word, so gameplay is exactly the PS1's. tests/test_geom.c checks every function
 * against the register-level reference (src/pc_gte.c).
 *
 * Only what the game keeps between calls is state: GeomState, the current transform, lighting
 * matrices and screen (what libgte's SetRotMatrix, SetGeomScreen ... set). Everything else is a
 * value: a vector result is a GeomAcc (the 32-bit sums and their 16-bit saturated copies).
 *
 * Conventions: 4.12 fixed point (4096 = 1.0) for matrices; `shift` is 0 or 12 (the hardware's
 * sf bit: results >> 12); `lm` 1 clamps the 16-bit results at 0 instead of -32768; `flag`, when
 * not NULL, receives the hardware's FLAG word for the operation (bit 31: any error bit). */
#ifndef PC_GEOM_H
#define PC_GEOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GeomState {
	int16_t rot[3][3]; /* rotation (RT) */
	int32_t tr[3];     /* translation, whole units */
	int16_t llm[3][3]; /* light matrix (THPS1's renderer keeps its frustum normals here) */
	int32_t bk[3];     /* background colour, or the frustum distances */
	int16_t lcm[3][3]; /* light colour matrix */
	int32_t fc[3];     /* far colour, x16 */
	int32_t ofx, ofy;  /* screen offset, 16.16 */
	uint16_t h;        /* projection plane distance */
	int16_t dqa;       /* depth cue: ir0 = (dqa * h / sz + dqb) >> 12 */
	int32_t dqb;
	int16_t zsf3, zsf4; /* average-Z scales (geom_avsz4) */
} GeomState;

/* The game's state (the PS1 had one GTE). */
extern GeomState pc_geom;
void PC_GeomReset(void);

/* A vector result: the 32-bit sums and their saturated 16-bit copies. */
typedef struct {
	int32_t mac[3];
	int16_t ir[3];
} GeomAcc;

/* A perspective projection: the screen word (x low, y high, each clamped to -1024..1023), the
 * clamped depth (0..65535), and the view-space position. */
typedef struct {
	uint32_t sxy;
	uint16_t sz;
	GeomAcc v;
} GeomProj;

/* The depth cue of a projection: mac0 = dqa * h / sz + dqb, ir0 = mac0 >> 12 clamped to 0..4096. */
typedef struct {
	int32_t mac0;
	int16_t ir0;
} GeomDepthQ;

/* Called for every projected vertex when set (the PC renderers' precise vertices and float
 * capture): the unshifted view-space sums (acc), the screen word, the FLAG bits of the operation
 * so far and whether this vertex's screen position was clamped. */
typedef void (*GeomProjObserver)(const GeomState *g, const int64_t acc[3], int shift, uint32_t sxy,
                                 uint32_t flag, int clamped);
extern GeomProjObserver pc_geom_observer;

/* FLAG bits */
enum {
	GEOM_F_MAC1_POS = 1u << 30, /* MAC2: >> 1, MAC3: >> 2 */
	GEOM_F_MAC1_NEG = 1u << 27,
	GEOM_F_IR1 = 1u << 24, /* IR2: 1 << 23, IR3: 1 << 22 */
	GEOM_F_COL_R = 1u << 21,
	GEOM_F_COL_G = 1u << 20,
	GEOM_F_COL_B = 1u << 19,
	GEOM_F_SZ_OTZ = 1u << 18,
	GEOM_F_DIV = 1u << 17,
	GEOM_F_MAC0_POS = 1u << 16,
	GEOM_F_MAC0_NEG = 1u << 15,
	GEOM_F_SX = 1u << 14,
	GEOM_F_SY = 1u << 13,
	GEOM_F_IR0 = 1u << 12,
	GEOM_F_ERROR_MASK = 0x7F87E000u,
};

extern const uint8_t geom_unr_table[257];

/* ---- internals ------------------------------------------------------------------------------- */

static inline int64_t geom__sext44(int64_t v)
{
	return (int64_t)((uint64_t)v << 20) >> 20;
}

/* i: 1..3 */
static inline void geom__mac_check(int i, int64_t v, uint32_t *f)
{
	if (v > 0x7FFFFFFFFFFLL)
		*f |= GEOM_F_MAC1_POS >> (i - 1);
	else if (v < -0x80000000000LL)
		*f |= GEOM_F_MAC1_NEG >> (i - 1);
}

/* one accumulation step: check, then wrap to 44 bits */
static inline int64_t geom__mac_step(int i, int64_t v, uint32_t *f)
{
	geom__mac_check(i, v, f);
	return geom__sext44(v);
}

static inline void geom__mac0_check(int64_t v, uint32_t *f)
{
	if (v > 0x7FFFFFFFLL)
		*f |= GEOM_F_MAC0_POS;
	else if (v < -0x80000000LL)
		*f |= GEOM_F_MAC0_NEG;
}

static inline int16_t geom__ir(int i, int32_t v, int lm, uint32_t *f)
{
	int32_t lo = lm ? 0 : -0x8000;
	if (v < lo) {
		*f |= 1u << (25 - i);
		return (int16_t)lo;
	}
	if (v > 0x7FFF) {
		*f |= 1u << (25 - i);
		return 0x7FFF;
	}
	return (int16_t)v;
}

/* the final sum of component i (0..2) into a */
static inline void geom__set(GeomAcc *a, int i, int64_t v, int shift, int lm, uint32_t *f)
{
	geom__mac_check(i + 1, v, f);
	a->mac[i] = (int32_t)(v >> shift);
	a->ir[i] = geom__ir(i + 1, a->mac[i], lm, f);
}

static inline uint32_t geom__finish(uint32_t f)
{
	return (f & GEOM_F_ERROR_MASK) ? f | 0x80000000u : f;
}

static inline uint32_t geom__col(int32_t v, uint32_t bit, uint32_t *f)
{
	if (v < 0) {
		*f |= bit;
		return 0;
	}
	if (v > 0xFF) {
		*f |= bit;
		return 0xFF;
	}
	return (uint32_t)v;
}

/* the colour word of a colour result: mac >> 4 clamped to bytes, with code in the top byte */
static inline uint32_t geom__rgb(const GeomAcc *a, uint8_t code, uint32_t *f)
{
	uint32_t r = geom__col(a->mac[0] >> 4, GEOM_F_COL_R, f);
	uint32_t g = geom__col(a->mac[1] >> 4, GEOM_F_COL_G, f);
	uint32_t b = geom__col(a->mac[2] >> 4, GEOM_F_COL_B, f);
	return r | g << 8 | b << 16 | (uint32_t)code << 24;
}

/* the perspective divide h / sz (17 bits, 1.16), as the hardware's UNR reciprocal */
static inline uint32_t geom__divide(uint32_t h, uint32_t sz, uint32_t *f)
{
	int z = 0;
	uint32_t n, d, u;
	uint64_t r;
	if (h >= sz * 2) {
		*f |= GEOM_F_DIV;
		return 0x1FFFF;
	}
	while (z < 16 && !(sz & (0x8000u >> z)))
		z++;
	n = h << z;
	d = sz << z;
	u = geom_unr_table[(d - 0x7FC0) >> 7] + 0x101u;
	d = (0x2000080u - d * u) >> 8;
	d = (0x0000080u + d * u) >> 8;
	r = ((uint64_t)n * d + 0x8000) >> 16;
	return r > 0x1FFFF ? 0x1FFFF : (uint32_t)r;
}

static inline void geom__mat_vec(GeomAcc *a, const int16_t m[3][3], const int32_t *t, int32_t vx,
                                 int32_t vy, int32_t vz, int shift, int lm, uint32_t *f)
{
	int i;
	for (i = 0; i < 3; i++) {
		int64_t acc = t ? (int64_t)t[i] * 4096 : 0;
		acc = geom__mac_step(i + 1, acc + (int64_t)m[i][0] * vx, f);
		acc = geom__mac_step(i + 1, acc + (int64_t)m[i][1] * vy, f);
		geom__set(a, i, acc + (int64_t)m[i][2] * vz, shift, lm, f);
	}
}

/* in + (fc * 4096 - in) * ir0, the depth cue towards the far colour */
static inline void geom__depth_cue(GeomAcc *a, const GeomState *g, int64_t m1, int64_t m2,
                                   int64_t m3, int16_t ir0, int shift, int lm, uint32_t *f)
{
	GeomAcc t;
	geom__set(&t, 0, (int64_t)g->fc[0] * 4096 - m1, shift, 0, f);
	geom__set(&t, 1, (int64_t)g->fc[1] * 4096 - m2, shift, 0, f);
	geom__set(&t, 2, (int64_t)g->fc[2] * 4096 - m3, shift, 0, f);
	geom__set(a, 0, (int64_t)t.ir[0] * ir0 + m1, shift, lm, f);
	geom__set(a, 1, (int64_t)t.ir[1] * ir0 + m2, shift, lm, f);
	geom__set(a, 2, (int64_t)t.ir[2] * ir0 + m3, shift, lm, f);
}

/* One projection; returns the divide result for the depth cue. */
static inline uint32_t geom__rtp(const GeomState *g, int32_t vx, int32_t vy, int32_t vz, int shift,
                                 int lm, GeomProj *p, uint32_t *f)
{
	int64_t acc[3], z12, x, y, sx, sy;
	int32_t lo = lm ? 0 : -0x8000, ir3;
	uint32_t n;
	int i, clamped = 0;

	for (i = 0; i < 3; i++) {
		int64_t a = (int64_t)g->tr[i] * 4096;
		a = geom__mac_step(i + 1, a + (int64_t)g->rot[i][0] * vx, f);
		a = geom__mac_step(i + 1, a + (int64_t)g->rot[i][1] * vy, f);
		acc[i] = a + (int64_t)g->rot[i][2] * vz;
	}
	geom__set(&p->v, 0, acc[0], shift, lm, f);
	geom__set(&p->v, 1, acc[1], shift, lm, f);

	/* IR3 saturates on MAC3, but its flag reflects MAC3 >> 12 whatever the shift */
	geom__mac_check(3, acc[2], f);
	p->v.mac[2] = (int32_t)(acc[2] >> shift);
	z12 = acc[2] >> 12;
	if (z12 < -0x8000 || z12 > 0x7FFF)
		*f |= 1u << 22;
	ir3 = p->v.mac[2];
	p->v.ir[2] = (int16_t)(ir3 < lo ? lo : ir3 > 0x7FFF ? 0x7FFF : ir3);

	if (z12 < 0) {
		z12 = 0;
		*f |= GEOM_F_SZ_OTZ;
	} else if (z12 > 0xFFFF) {
		z12 = 0xFFFF;
		*f |= GEOM_F_SZ_OTZ;
	}
	p->sz = (uint16_t)z12;
	n = geom__divide(g->h, p->sz, f);

	x = (int64_t)n * p->v.ir[0] + g->ofx;
	y = (int64_t)n * p->v.ir[1] + g->ofy;
	geom__mac0_check(x, f);
	geom__mac0_check(y, f);
	sx = x >> 16;
	sy = y >> 16;
	if (sx < -0x400) {
		sx = -0x400;
		*f |= GEOM_F_SX;
		clamped = 1;
	} else if (sx > 0x3FF) {
		sx = 0x3FF;
		*f |= GEOM_F_SX;
		clamped = 1;
	}
	if (sy < -0x400) {
		sy = -0x400;
		*f |= GEOM_F_SY;
		clamped = 1;
	} else if (sy > 0x3FF) {
		sy = 0x3FF;
		*f |= GEOM_F_SY;
		clamped = 1;
	}
	p->sxy = (uint16_t)sx | (uint32_t)(uint16_t)sy << 16;
	if (pc_geom_observer)
		pc_geom_observer(g, acc, shift, p->sxy, *f, clamped);
	return n;
}

static inline void geom__depth_q(const GeomState *g, uint32_t n, GeomDepthQ *dq, uint32_t *f)
{
	int64_t v = (int64_t)n * g->dqa + g->dqb, i0;
	geom__mac0_check(v, f);
	i0 = v >> 12;
	if (i0 < 0) {
		i0 = 0;
		*f |= GEOM_F_IR0;
	} else if (i0 > 0x1000) {
		i0 = 0x1000;
		*f |= GEOM_F_IR0;
	}
	if (dq) {
		dq->mac0 = (int32_t)v;
		dq->ir0 = (int16_t)i0;
	}
}

/* ---- operations ------------------------------------------------------------------------------ */

/* Perspective projection of v by g's rotation, translation and screen (RTPS). The view-space
 * position is (rot * v + tr * 4096) >> shift; the depth for the divide is always >> 12. */
static inline GeomProj geom_rtps(const GeomState *g, int32_t vx, int32_t vy, int32_t vz, int shift,
                                 int lm, GeomDepthQ *dq, uint32_t *flag)
{
	GeomProj p;
	uint32_t f = 0, n = geom__rtp(g, vx, vy, vz, shift, lm, &p, &f);
	if (dq || flag)
		geom__depth_q(g, n, dq, &f);
	if (flag)
		*flag = geom__finish(f);
	return p;
}

/* Three projections with one FLAG and the last one's depth cue (RTPT); v: x, y, z each. */
static inline void geom_rtpt(const GeomState *g, const int16_t v[3][3], int shift, int lm,
                             GeomProj out[3], GeomDepthQ *dq, uint32_t *flag)
{
	uint32_t f = 0, n = 0;
	int i;
	for (i = 0; i < 3; i++)
		n = geom__rtp(g, v[i][0], v[i][1], v[i][2], shift, lm, &out[i], &f);
	if (dq || flag)
		geom__depth_q(g, n, dq, &f);
	if (flag)
		*flag = geom__finish(f);
}

/* (m * v + t * 4096) >> shift; t NULL: no translation (MVMVA, and libgte's rt*, ll*, lc*). */
static inline GeomAcc geom_mvmva(const int16_t m[3][3], const int32_t *t, int32_t vx, int32_t vy,
                                 int32_t vz, int shift, int lm, uint32_t *flag)
{
	GeomAcc a;
	uint32_t f = 0;
	geom__mat_vec(&a, m, t, vx, vy, vz, shift, lm, &f);
	if (flag)
		*flag = geom__finish(f);
	return a;
}

/* Outer product d x v >> shift (OP: d is the hardware's rotation diagonal). */
static inline GeomAcc geom_op(const int16_t d[3], const int16_t v[3], int shift, int lm,
                              uint32_t *flag)
{
	GeomAcc a;
	uint32_t f = 0;
	int64_t d1 = d[0], d2 = d[1], d3 = d[2], i1 = v[0], i2 = v[1], i3 = v[2];
	geom__set(&a, 0, d2 * i3 - d3 * i2, shift, lm, &f);
	geom__set(&a, 1, d3 * i1 - d1 * i3, shift, lm, &f);
	geom__set(&a, 2, d1 * i2 - d2 * i1, shift, lm, &f);
	if (flag)
		*flag = geom__finish(f);
	return a;
}

/* Squares of v's components >> shift (SQR). */
static inline GeomAcc geom_sqr(const int16_t v[3], int shift, int lm, uint32_t *flag)
{
	GeomAcc a;
	uint32_t f = 0;
	int i;
	for (i = 0; i < 3; i++)
		geom__set(&a, i, (int64_t)v[i] * v[i], shift, lm, &f);
	if (flag)
		*flag = geom__finish(f);
	return a;
}

/* v * k >> shift (GPF); rgb (if not NULL) gets the result as a colour word with code on top. */
static inline GeomAcc geom_gpf(int16_t k, const int16_t v[3], int shift, int lm, uint8_t code,
                               uint32_t *rgb, uint32_t *flag)
{
	GeomAcc a;
	uint32_t f = 0, c;
	int i;
	for (i = 0; i < 3; i++)
		geom__set(&a, i, (int64_t)v[i] * k, shift, lm, &f);
	if (rgb || flag) {
		c = geom__rgb(&a, code, &f);
		if (rgb)
			*rgb = c;
	}
	if (flag)
		*flag = geom__finish(f);
	return a;
}

/* (base << shift) + v * k, >> shift (GPL: base is the previous result's mac). */
static inline GeomAcc geom_gpl(int16_t k, const int16_t v[3], const int32_t base[3], int shift,
                               int lm, uint8_t code, uint32_t *rgb, uint32_t *flag)
{
	GeomAcc a;
	uint32_t f = 0, c;
	int i;
	for (i = 0; i < 3; i++)
		geom__set(&a, i, (int64_t)v[i] * k + (int64_t)base[i] * ((int64_t)1 << shift), shift, lm,
		          &f);
	if (rgb || flag) {
		c = geom__rgb(&a, code, &f);
		if (rgb)
			*rgb = c;
	}
	if (flag)
		*flag = geom__finish(f);
	return a;
}

/* Depth cue of the colour col (r, g, b bytes) towards g's far colour by k (0..4096) (DPCS);
 * rgb gets the colour word with code on top. */
static inline GeomAcc geom_dpcs(const GeomState *g, uint32_t col, uint8_t code, int16_t k,
                                int shift, int lm, uint32_t *rgb, uint32_t *flag)
{
	GeomAcc a;
	uint32_t f = 0, c;
	geom__depth_cue(&a, g, (int64_t)(col & 0xFF) << 16, (int64_t)((col >> 8) & 0xFF) << 16,
	                (int64_t)((col >> 16) & 0xFF) << 16, k, shift, lm, &f);
	c = geom__rgb(&a, code, &f);
	if (rgb)
		*rgb = c;
	if (flag)
		*flag = geom__finish(f);
	return a;
}

/* DPCS of three colours with one FLAG (DPCT). */
static inline void geom_dpct(const GeomState *g, const uint32_t col[3], uint8_t code, int16_t k,
                             int shift, int lm, uint32_t rgb[3], uint32_t *flag)
{
	GeomAcc a;
	uint32_t f = 0;
	int i;
	for (i = 0; i < 3; i++) {
		geom__depth_cue(&a, g, (int64_t)(col[i] & 0xFF) << 16,
		                (int64_t)((col[i] >> 8) & 0xFF) << 16, (int64_t)((col[i] >> 16) & 0xFF) << 16,
		                k, shift, lm, &f);
		rgb[i] = geom__rgb(&a, code, &f);
	}
	if (flag)
		*flag = geom__finish(f);
}

/* Depth cue of the colour col (r, g, b bytes; code on top) lit by v (4.12 per channel) (DPCL). */
static inline GeomAcc geom_dpcl(const GeomState *g, uint32_t col, const int16_t v[3], int16_t k,
                                int shift, int lm, uint32_t *rgb, uint32_t *flag)
{
	GeomAcc a;
	uint32_t f = 0, c;
	geom__depth_cue(&a, g, ((int64_t)(col & 0xFF) * v[0]) * 16,
	                ((int64_t)((col >> 8) & 0xFF) * v[1]) * 16,
	                ((int64_t)((col >> 16) & 0xFF) * v[2]) * 16, k, shift, lm, &f);
	c = geom__rgb(&a, (uint8_t)(col >> 24), &f);
	if (rgb)
		*rgb = c;
	if (flag)
		*flag = geom__finish(f);
	return a;
}

/* Normal colour: the normal v through g's light matrix, the light colours plus bk, times the
 * colour col (code on top) (NCCS). */
static inline GeomAcc geom_nccs(const GeomState *g, int32_t vx, int32_t vy, int32_t vz,
                                uint32_t col, int shift, int lm, uint32_t *rgb, uint32_t *flag)
{
	GeomAcc a, l;
	uint32_t f = 0, c;
	geom__mat_vec(&l, g->llm, 0, vx, vy, vz, shift, lm, &f);
	geom__mat_vec(&a, g->lcm, g->bk, l.ir[0], l.ir[1], l.ir[2], shift, lm, &f);
	geom__set(&a, 0, ((int64_t)(col & 0xFF) * a.ir[0]) * 16, shift, lm, &f);
	geom__set(&a, 1, ((int64_t)((col >> 8) & 0xFF) * a.ir[1]) * 16, shift, lm, &f);
	geom__set(&a, 2, ((int64_t)((col >> 16) & 0xFF) * a.ir[2]) * 16, shift, lm, &f);
	c = geom__rgb(&a, (uint8_t)(col >> 24), &f);
	if (rgb)
		*rgb = c;
	if (flag)
		*flag = geom__finish(f);
	return a;
}

/* Twice the signed area of the screen triangle (NCLIP): positive for one winding. */
static inline int32_t geom_nclip(uint32_t sxy0, uint32_t sxy1, uint32_t sxy2, uint32_t *flag)
{
	int64_t x0 = (int16_t)sxy0, y0 = (int16_t)(sxy0 >> 16), x1 = (int16_t)sxy1,
	        y1 = (int16_t)(sxy1 >> 16), x2 = (int16_t)sxy2, y2 = (int16_t)(sxy2 >> 16);
	int64_t v = x0 * y1 + x1 * y2 + x2 * y0 - x0 * y2 - x1 * y0 - x2 * y1;
	if (flag) {
		uint32_t f = 0;
		geom__mac0_check(v, &f);
		*flag = geom__finish(f);
	}
	return (int32_t)v;
}

/* The scaled average of four depths (AVSZ4): (zsf4 * sum) >> 12 clamped to 0..65535; mac0 gets
 * the unshifted product. */
static inline uint16_t geom_avsz4(const GeomState *g, uint16_t sz0, uint16_t sz1, uint16_t sz2,
                                  uint16_t sz3, int32_t *mac0, uint32_t *flag)
{
	int64_t v = (int64_t)g->zsf4 * ((int32_t)sz0 + sz1 + sz2 + sz3), z = v >> 12;
	uint32_t f = 0;
	geom__mac0_check(v, &f);
	if (z < 0) {
		z = 0;
		f |= GEOM_F_SZ_OTZ;
	} else if (z > 0xFFFF) {
		z = 0xFFFF;
		f |= GEOM_F_SZ_OTZ;
	}
	if (mac0)
		*mac0 = (int32_t)v;
	if (flag)
		*flag = geom__finish(f);
	return (uint16_t)z;
}

/* The number of leading bits equal to the sign bit (1..32; LZCS/LZCR). */
static inline int geom_lzc(int32_t v)
{
	uint32_t u = v < 0 ? ~(uint32_t)v : (uint32_t)v;
	int n = 0;
	while (n < 32 && !(u & (0x80000000u >> n)))
		n++;
	return n;
}

/* ---- state ----------------------------------------------------------------------------------- */

/* A 4.12 matrix as the hardware loads it from a MATRIX: m points at m[0][0] (9 shorts). */
static inline void geom_load_matrix(int16_t dst[3][3], const int16_t *m)
{
	int i;
	for (i = 0; i < 9; i++)
		dst[i / 3][i % 3] = m[i];
}

static inline void geom_load_vector(int32_t dst[3], int32_t x, int32_t y, int32_t z)
{
	dst[0] = x;
	dst[1] = y;
	dst[2] = z;
}

/* An outer product's first operand, loaded the hardware's way: as whole words over the rotation
 * matrix's diagonal, so the high halves land in rot[0][1] and rot[1][2]. */
static inline void geom_load_op_diag(GeomState *g, uint32_t x, uint32_t y, uint32_t z)
{
	g->rot[0][0] = (int16_t)x;
	g->rot[0][1] = (int16_t)(x >> 16);
	g->rot[1][1] = (int16_t)y;
	g->rot[1][2] = (int16_t)(y >> 16);
	g->rot[2][2] = (int16_t)z;
}

static inline void geom_op_diag(const GeomState *g, int16_t d[3])
{
	d[0] = g->rot[0][0];
	d[1] = g->rot[1][1];
	d[2] = g->rot[2][2];
}

/* ---- the game's vectors in memory ------------------------------------------------------------ */

/* Psy-Q's layouts (SVECTOR: three shorts; VECTOR: three words; or the game's own classes laid out
 * the same), loaded the way the operations take them: a word's low 16 bits where they take 16. */
static inline void geom_ld_sv(int16_t v[3], const void *p)
{
	__builtin_memcpy(v, p, 6);
}

static inline void geom_ld_lv(int16_t v[3], const void *p)
{
	int32_t w[3];
	__builtin_memcpy(w, p, 12);
	v[0] = (int16_t)w[0];
	v[1] = (int16_t)w[1];
	v[2] = (int16_t)w[2];
}

static inline void geom_ld_lv32(int32_t v[3], const void *p)
{
	__builtin_memcpy(v, p, 12);
}

static inline void geom_st_sv(void *p, const int16_t v[3])
{
	__builtin_memcpy(p, v, 6);
}

static inline void geom_st_lv(void *p, const int32_t v[3])
{
	__builtin_memcpy(p, v, 12);
}

/* a word (a screen word into a GPU packet's x, y ...) */
static inline void geom_st_word(void *p, uint32_t w)
{
	__builtin_memcpy(p, &w, 4);
}

/* a MATRIX's rotation part (nine shorts at its start; Psy-Q's SetRotMatrix and friends) */
static inline void geom_ld_mat(int16_t m[3][3], const void *p)
{
	__builtin_memcpy(m, p, 18);
}

/* the squares of a VECTOR's components (16 bits each, results clamped at 0..0x7fff in 16 bits),
 * into a VECTOR's three words: the game's squared lengths (Psy-Q's ldlvl/sqr0/stlvnl) */
static inline void geom_square(const void *in, void *out)
{
	int16_t v[3];
	GeomAcc r;
	geom_ld_lv(v, in);
	r = geom_sqr(v, 0, 1, 0);
	geom_st_lv(out, r.mac);
}

/* a VECTOR (16 bits of each component) / an SVECTOR projected with the game's state (Psy-Q's
 * ldlv0 or ldv0, then rtps) */
static inline GeomProj geom_project_lv(const void *p)
{
	int16_t v[3];
	geom_ld_lv(v, p);
	return geom_rtps(&pc_geom, v[0], v[1], v[2], 12, 0, 0, 0);
}

static inline GeomProj geom_project_sv(const void *p)
{
	int16_t v[3];
	geom_ld_sv(v, p);
	return geom_rtps(&pc_geom, v[0], v[1], v[2], 12, 0, 0, 0);
}

/* an SVECTOR through the game's rotation and translation (>> 12), into a VECTOR's three words
 * (Psy-Q's ldv0/rtv0tr/stlvnl) */
static inline void geom_transform_sv(const void *in, void *out)
{
	int16_t v[3];
	GeomAcc r;
	geom_ld_sv(v, in);
	r = geom_mvmva(pc_geom.rot, pc_geom.tr, v[0], v[1], v[2], 12, 0, 0);
	geom_st_lv(out, r.mac);
}

/* (a x b) >> shift of two VECTORs (16 bits of each component), into a VECTOR's three words: the
 * game's cross products (Psy-Q's ldopv1/ldopv2/op/stlvnl). */
static inline void geom_cross(const void *a, const void *b, void *out, int shift)
{
	int16_t x[3], y[3];
	GeomAcc r;
	geom_ld_lv(x, a);
	geom_ld_lv(y, b);
	r = geom_op(x, y, shift, 0, 0);
	geom_st_lv(out, r.mac);
}

#ifdef __cplusplus
}
#endif

#endif

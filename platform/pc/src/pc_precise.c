/* Precise projections and the float capture of projected vertices (pc_precise.h), recorded from
 * the geometry maths' projections (pc_geom.h's observer). */
#include "pc_precise.h"
#include "pc_geom.h"

#include <math.h>
#include <stddef.h>

extern volatile uint32_t pc_heartbeat;

#define PRECISE_SLOTS (1 << 16)
#define PRECISE_PROBE 8
#define PRECISE_CANDS 4
#define PRECISE_MAX_AGE 6 /* vblanks; a 30 fps frame is projected and then drawn within about 4 */

/* Every distinct projection that rounded to one screen word in one vblank (usually one). */
typedef struct {
	uint32_t key, gen; /* gen: pc_heartbeat + 1 when recorded, 0: never used */
	int n;
	float x[PRECISE_CANDS], y[PRECISE_CANDS], z[PRECISE_CANDS];
} Precise;

static Precise s_precise[PRECISE_SLOTS];
static int precise_on;

static double precise_zscale = 1.0;

static float (*s_capture)[3];
static int s_capture_n, s_capture_max;

/* Only the 11 bits per coordinate the GPU reads: renderers reuse the others (THPS1's world
 * renderer keeps off-screen flags in bits 14 and 30 of the words it caches). */
static inline uint32_t precise_key(uint32_t sxy)
{
	return sxy & 0x07FF07FFu;
}

static inline uint32_t precise_hash(uint32_t key)
{
	return (key * 0x9E3779B1u) >> 16;
}

/* A projection, unrounded: acc = the view-space sums before the shift. */
static void precise_record(const GeomState *g, const int64_t *acc, int shift, uint32_t sxy,
                           uint32_t flag, int clamped)
{
	/* The PS1's projection is less exact than this (the depth is truncated before the divide:
	 * several pixels off near the camera), so the results can't be compared; its flags say
	 * whether it projected at all rather than clamping the screen position or the view-space x/y
	 * or overflowing the divide. */
	if (clamped || (flag & (GEOM_F_IR1 | GEOM_F_IR1 >> 1 | GEOM_F_DIV)) || acc[2] <= 0)
		return;
	double z = (double)acc[2] / 4096.0;
	if (z > 0xFFFF)
		return;
	double scale = (double)g->h / z / (double)(1 << shift); /* x/y >> shift, depth >> 12 */
	double cx = g->ofx / 65536.0, cy = g->ofy / 65536.0;
	double x = (double)acc[0] * scale + cx;
	double y = (double)acc[1] * scale + cy;
	z *= precise_zscale;

	uint32_t gen = pc_heartbeat + 1;
	uint32_t key = precise_key(sxy);
	uint32_t h = precise_hash(key);
	Precise *slot = NULL;
	for (int i = 0; i < PRECISE_PROBE; i++) {
		Precise *e = &s_precise[(h + i) & (PRECISE_SLOTS - 1)];
		if (e->gen && e->key == key) {
			if (e->gen != gen) {
				slot = e;
				break;
			}
			/* The same point projected again from a less exact copy (THPS1's subdivision
			 * re-projects 16-bit view-space positions) is close in depth and off in proportion
			 * to the distance from the centre; the first projection is kept. */
			for (int c = 0; c < e->n; c++) {
				double dz = fabs(e->z[c] - z) / e->z[c] + 1.0 / 128;
				if (fabs(e->z[c] - z) <= 1.0 + e->z[c] / 64 &&
				    fabs(e->x[c] - x) <= 0.25 + fabs(x - cx) * dz &&
				    fabs(e->y[c] - y) <= 0.25 + fabs(y - cy) * dz)
					return;
			}
			if (e->n < PRECISE_CANDS) {
				e->x[e->n] = (float)x;
				e->y[e->n] = (float)y;
				e->z[e->n] = (float)z;
				e->n++;
			}
			return;
		}
		if (!slot || e->gen < slot->gen)
			slot = e; /* else the oldest in the window */
	}
	slot->key = key;
	slot->gen = gen;
	slot->n = 1;
	slot->x[0] = (float)x;
	slot->y[0] = (float)y;
	slot->z[0] = (float)z;
}

static const Precise *precise_find(uint32_t key)
{
	uint32_t h = precise_hash(key);
	for (int i = 0; i < PRECISE_PROBE; i++) {
		const Precise *e = &s_precise[(h + i) & (PRECISE_SLOTS - 1)];
		if (e->gen && e->key == key)
			return pc_heartbeat + 1 - e->gen <= PRECISE_MAX_AGE ? e : NULL;
	}
	return NULL;
}

/* Candidates for one SXY word, into x/y/z (up to 9 * PRECISE_CANDS). THPS1's polygon subdivision
 * moves the vertices on a grid's edges a pixel outwards so that the grid overlaps its neighbours
 * (0x80087ee8), so a word nobody projected falls back to the projections one pixel around it,
 * moved the same way. */
static int precise_candidates(uint32_t sxy, float *x, float *y, float *z)
{
	uint32_t key = precise_key(sxy);
	int kx = (int)(key << 21) >> 21, ky = (int)(key >> 16 << 21) >> 21, n = 0, dx = 0, dy = 0;
	const Precise *e = precise_find(key);
	for (int d = -1; d < 9; d++) {
		if (d >= 0) {
			dx = d % 3 - 1;
			dy = d / 3 - 1;
			if (!dx && !dy)
				continue;
			e = precise_find(precise_key((uint16_t)(kx + dx) | (uint32_t)(uint16_t)(ky + dy) << 16));
		}
		for (int c = 0; e && c < e->n; c++) {
			x[n] = e->x[c] - dx;
			y[n] = e->y[c] - dy;
			z[n] = e->z[c];
			n++;
		}
		if (d < 0 && n)
			return n;
	}
	return n;
}

int PC_PrecisePolygon(const uint32_t *sxy, int n, float *x, float *y, float *z)
{
	float cx[4][9 * PRECISE_CANDS], cy[4][9 * PRECISE_CANDS], cz[4][9 * PRECISE_CANDS];
	int nc[4], known = 0;
	double zsum = 0;
	for (int i = 0; i < n; i++) {
		nc[i] = precise_candidates(sxy[i], cx[i], cy[i], cz[i]);
		if (!nc[i])
			return 0;
		if (nc[i] == 1) {
			zsum += cz[i][0];
			known++;
		}
	}
	/* where several vertices rounded to the same pixel, the one at the polygon's depth */
	if (known < n && !known)
		return 0;
	double zref = known ? zsum / known : 0;
	for (int i = 0; i < n; i++) {
		int best = 0;
		for (int c = 1; c < nc[i]; c++)
			if (fabs(cz[i][c] - zref) < fabs(cz[i][best] - zref))
				best = c;
		x[i] = cx[i][best];
		y[i] = cy[i][best];
		z[i] = cz[i][best];
	}
	return 1;
}

static void observe(const GeomState *g, const int64_t acc[3], int shift, uint32_t sxy, uint32_t flag,
                    int clamped)
{
	if (precise_on)
		precise_record(g, acc, shift, sxy, flag, clamped);
	if (s_capture && s_capture_n < s_capture_max) {
		float *c = s_capture[s_capture_n++];
		c[0] = (float)((double)acc[0] / 4096.0);
		c[1] = (float)((double)acc[1] / 4096.0);
		c[2] = (float)((double)acc[2] / 4096.0);
	}
}

static void update_observer(void)
{
	pc_geom_observer = precise_on || s_capture ? observe : NULL;
}

void PC_PreciseEnable(int on)
{
	precise_on = on;
	update_observer();
}

void PC_PreciseZScale(float s)
{
	precise_zscale = s;
}

void PC_CaptureBegin(float (*buf)[3], int max)
{
	s_capture = buf;
	s_capture_n = 0;
	s_capture_max = max;
	update_observer();
}

int PC_CaptureEnd(void)
{
	s_capture = NULL;
	update_observer();
	return s_capture_n;
}

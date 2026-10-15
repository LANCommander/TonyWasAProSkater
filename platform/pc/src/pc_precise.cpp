/* Precise projections and the float capture of projected vertices (pc_precise.h, pc/precise.h),
 * recorded from the geometry maths' projections (pc_geom.h's observer). */
#include "pc/precise.h"
#include "pc_geom.h"

#include <math.h>
#include <stddef.h>

extern volatile uint32_t pc_heartbeat;

namespace pc {

PreciseVertices g_precise;
PreciseVertices::Entry PreciseVertices::s_table[PreciseVertices::kSlots];

namespace {

/* Only the 11 bits per coordinate the GPU reads: renderers reuse the others (THPS1's world
 * renderer keeps off-screen flags in bits 14 and 30 of the words it caches). */
inline uint32_t PreciseKey(uint32_t sxy)
{
	return sxy & 0x07FF07FFu;
}

inline uint32_t PreciseHash(uint32_t key)
{
	return (key * 0x9E3779B1u) >> 16;
}

/* pc_geom_observer: forwards to the instance */
void ObserveProjection(const GeomState *g, const int64_t acc[3], int shift, uint32_t sxy,
                       uint32_t flag, int clamped)
{
	g_precise.Observe(g, acc, shift, sxy, flag, clamped);
}

} // namespace

/* A projection, unrounded: acc = the view-space sums before the shift. */
void PreciseVertices::Record(const GeomState *g, const int64_t *acc, int shift, uint32_t sxy,
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
	z *= m_zscale;

	uint32_t gen = pc_heartbeat + 1;
	uint32_t key = PreciseKey(sxy);
	uint32_t h = PreciseHash(key);
	Entry *slot = nullptr;
	for (int i = 0; i < kProbe; i++) {
		Entry *e = &s_table[(h + i) & (kSlots - 1)];
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
			if (e->n < kCands) {
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

const PreciseVertices::Entry *PreciseVertices::Find(uint32_t key) const
{
	uint32_t h = PreciseHash(key);
	for (int i = 0; i < kProbe; i++) {
		const Entry *e = &s_table[(h + i) & (kSlots - 1)];
		if (e->gen && e->key == key)
			return pc_heartbeat + 1 - e->gen <= kMaxAge ? e : nullptr;
	}
	return nullptr;
}

/* Candidates for one SXY word, into x/y/z (up to 9 * kCands). THPS1's polygon subdivision
 * moves the vertices on a grid's edges a pixel outwards so that the grid overlaps its neighbours
 * (0x80087ee8), so a word nobody projected falls back to the projections one pixel around it,
 * moved the same way. */
int PreciseVertices::Candidates(uint32_t sxy, float *x, float *y, float *z) const
{
	uint32_t key = PreciseKey(sxy);
	int kx = (int)(key << 21) >> 21, ky = (int)(key >> 16 << 21) >> 21, n = 0, dx = 0, dy = 0;
	const Entry *e = Find(key);
	for (int d = -1; d < 9; d++) {
		if (d >= 0) {
			dx = d % 3 - 1;
			dy = d / 3 - 1;
			if (!dx && !dy)
				continue;
			e = Find(PreciseKey((uint16_t)(kx + dx) | (uint32_t)(uint16_t)(ky + dy) << 16));
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

int PreciseVertices::Polygon(const uint32_t *sxy, int n, float *x, float *y, float *z)
{
	float cx[4][9 * kCands], cy[4][9 * kCands], cz[4][9 * kCands];
	int nc[4], known = 0;
	double zsum = 0;
	for (int i = 0; i < n; i++) {
		nc[i] = Candidates(sxy[i], cx[i], cy[i], cz[i]);
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

void PreciseVertices::Observe(const GeomState *g, const int64_t acc[3], int shift, uint32_t sxy,
                              uint32_t flag, int clamped)
{
	if (m_on)
		Record(g, acc, shift, sxy, flag, clamped);
	if (m_capture && m_captureN < m_captureMax) {
		float *c = m_capture[m_captureN++];
		c[0] = (float)((double)acc[0] / 4096.0);
		c[1] = (float)((double)acc[1] / 4096.0);
		c[2] = (float)((double)acc[2] / 4096.0);
	}
}

void PreciseVertices::UpdateObserver()
{
	pc_geom_observer = m_on || m_capture ? ObserveProjection : nullptr;
}

void PreciseVertices::Enable(bool on)
{
	m_on = on;
	UpdateObserver();
}

void PreciseVertices::ZScale(float s)
{
	m_zscale = s;
}

void PreciseVertices::CaptureBegin(float (*buf)[3], int max)
{
	m_capture = buf;
	m_captureN = 0;
	m_captureMax = max;
	UpdateObserver();
}

int PreciseVertices::CaptureEnd()
{
	m_capture = nullptr;
	UpdateObserver();
	return m_captureN;
}

} // namespace pc

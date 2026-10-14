/* M3dAsm_ClipLine__FP12SClipLineBuf (0x8008ae04) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Projects a 3D line segment (bit/display.cpp's poly-lines, buffer in the scratchpad) for a
 * LINE_F2/G2 primitive. Returns its ordering-table depth (0..4095) with ScrStart/ScrEnd set to the
 * screen end points, or -1 when it isn't drawn.
 *
 * Both end points are projected relative to the camera (rotation, translation and screen as the
 * caller set them, pc_geom.h). A short line (less than 1024 x 512 pixels across) that
 * projects without trouble is taken as it is. Anything else is clipped against the view frustum:
 * the signed distances of both end points from the frustum planes (light matrix + back colour,
 * colour matrix: rows of FrustumNormals) give, for each plane the line crosses, the fraction of the
 * line where it does; the visible part runs from the largest entering fraction tNear to the
 * smallest leaving one tFar. Its end points are then re-projected from those fractions with the
 * two view-space points (scaled by 8) as the rotation, so that rot * (1 - t, 0, t) interpolates
 * between them. The depth is a quarter of the nearer end's view z.
 *
 * State left behind, as retail leaves it: the light matrix, back colour and colour matrix keep
 * the frustum planes (the rotation is as before).
 * test_asm: gte_out none */
#include <sys/types.h>
#include <libgte.h>

#include "pc_geom.h"

typedef struct {
	int Start[3];          /* 0x00: world, 20.12 */
	int End[3];            /* 0x0c */
	unsigned ScrStart;     /* 0x18: screen x | y << 16 */
	unsigned ScrEnd;       /* 0x1c */
	int tNear, tFar;       /* 0x20: the visible part of the line, fractions in 4.12 */
	unsigned zStart, zEnd; /* 0x28: view depths of the drawn end points */
	int Dist[2][6];        /* 0x30: start's and end's distances from the six frustum planes */
} SClipLineBuf;

typedef struct {
	unsigned Style;
	int Position[3]; /* 0x04: world, whole units */
} SCamera;

extern SCamera *pCurrentCamera;
extern SVECTOR FrustumNormals[]; /* plane normals; pad of [0] and [1] is the plane offset */

/* MIPS div: never traps (x / 0 gives -1 for x >= 0, else 1; INT_MIN / -1 gives INT_MIN) */
static int MipsDiv(int n, int d) {
	if (d == 0)
		return n >= 0 ? -1 : 1;

	if (d == -1)
		return (int)(0u - (unsigned)n);

	return n / d;
}

/* the point relative to the camera, in whole units (16 bits each), projected; flag: its FLAG */
static GeomProj ProjectRelative(const int p[3], unsigned *flag) {
	const int *cam = pCurrentCamera->Position;

	return geom_rtps(&pc_geom, (int16_t)((p[0] >> 12) - cam[0]), (int16_t)((p[1] >> 12) - cam[1]),
	                 (int16_t)((p[2] >> 12) - cam[2]), 12, 0, 0, flag);
}

static void LoadPlanes(int16_t m[3][3], const SVECTOR *a, const SVECTOR *b, const SVECTOR *c) {
	m[0][0] = a->vx, m[0][1] = a->vy, m[0][2] = a->vz;
	m[1][0] = b->vx, m[1][1] = b->vy, m[1][2] = b->vz;
	m[2][0] = c->vx, m[2][1] = c->vy, m[2][2] = c->vz;
}

/* Frustum planes 0-2 into the light matrix (offsets of 0 and 1 in the back colour), planes 3, 6
 * and 7 into the colour matrix */
static void LoadFrustumPlanes(void) {
	const SVECTOR *f = FrustumNormals;

	LoadPlanes(pc_geom.llm, &f[0], &f[1], &f[2]);
	geom_load_vector(pc_geom.bk, f[0].pad, f[1].pad, 0);
	LoadPlanes(pc_geom.lcm, &f[3], &f[6], &f[7]);
}

/* The distances (>> 12) of the point v (16 bits each) from three planes (m, + t) into d[0..2];
 * returns the planes the point is outside of (whose distance clamped at 0) as bits shift+2,
 * shift+1, shift. */
static unsigned PlaneDistances(const int16_t m[3][3], const int32_t *t, const int16_t v[3], int *d,
                               int shift) {
	unsigned flag;
	GeomAcc r = geom_mvmva(m, t, v[0], v[1], v[2], 12, 1, &flag);
	int i;

	for (i = 0; i < 3; i++)
		d[i] = r.mac[i];

	return (flag >> 22 & 7) << shift;
}

int M3dAsm_ClipLine__FP12SClipLineBuf(SClipLineBuf *b) {
	unsigned flagS, flagE, sxyS, sxyE, outS, outE, z0, z1, ds;
	int s[3], e[3], i, t, tNear, tFar;
	int16_t vs[3], ve[3];
	GeomState g;
	GeomProj p;

	p = ProjectRelative(b->Start, &flagS);
	sxyS = p.sxy;

	for (i = 0; i < 3; i++)
		s[i] = p.v.mac[i]; /* view space */

	p = ProjectRelative(b->End, &flagE);

	b->ScrStart = sxyS;
	b->zStart = (unsigned)s[2];
	b->tNear = 0;
	b->tFar = 4096;
	sxyE = p.sxy;

	for (i = 0; i < 3; i++)
		e[i] = p.v.mac[i];

	/* short and cleanly projected: no clipping */
	if ((int)(flagS | flagE) < 0) {
		t = (int)(flagS | flagE);
	} else {
		t = (short)(sxyE - sxyS); /* dx */

		if (t < -1023) {
			/* retail: the start's screen y is already loaded (branch delay slot) and is what
			 * gets tested below, not dx */
			t = (int)sxyS >> 16;
		} else if (t < 1024) {
			t = ((int)sxyE >> 16) - ((int)sxyS >> 16); /* dy */

			if (t < 512 && t >= -511) {
				b->ScrEnd = sxyE;
				b->zEnd = (unsigned)e[2];
				z0 = (unsigned)s[2];
				z1 = (unsigned)e[2];
				goto depth;
			}
		}
	}

	/* Bits 22-24 of t (view-space saturation when t is the projections' flags; otherwise t is
	 * the screen extent that was too large, which only passes when it's positive and under 4M,
	 * or the start's screen y, which passes unless negative) */
	if ((unsigned)t >> 22 & 7)
		return -1;

	if ((s[2] & e[2]) < 0)
		return -1; /* both behind the camera */

	/* the view-space points, 16 bits per coordinate */
	for (i = 0; i < 3; i++) {
		vs[i] = (int16_t)s[i];
		ve[i] = (int16_t)e[i];
	}

	LoadFrustumPlanes();

	outS = PlaneDistances(pc_geom.llm, pc_geom.bk, vs, b->Dist[0], 3);
	outS |= PlaneDistances(pc_geom.lcm, 0, vs, b->Dist[0] + 3, 0);
	outE = PlaneDistances(pc_geom.llm, pc_geom.bk, ve, b->Dist[1], 3);
	outE |= PlaneDistances(pc_geom.lcm, 0, ve, b->Dist[1] + 3, 0);

	if (outS & outE)
		return -1; /* both outside one plane */

	if ((outS | outE) & 0x20)
		return -1; /* either outside plane 0 */

	tNear = 0;
	tFar = 4096;

	for (i = 0; i < 6; i++) {
		unsigned bit = 0x20u >> i;

		if ((outS ^ outE) & bit) {
			int d0 = b->Dist[0][i], d1 = b->Dist[1][i];

			t = MipsDiv((int)((unsigned)d0 << 12), (int)((unsigned)d0 - (unsigned)d1));

			if (outS & bit) {
				if ((int)((unsigned)t - (unsigned)tNear) > 0)
					tNear = t; /* enters here */
			} else {
				if ((int)((unsigned)t - (unsigned)tFar) < 0)
					tFar = t; /* leaves here */
			}
		}
	}

	if ((int)((unsigned)tNear - (unsigned)tFar) >= 0)
		return -1;

	b->tNear = tNear;
	b->tFar = tFar;

	/* rot = 8 * [[x0 y0 x1] [y0 x1 y1] [z0 . z1]] (16 bits each; the middle of the bottom row is
	 * the high half of 8 * z0, as retail loads it): rot * (1 - t, 0, t) = 8 * lerp(start, end, t) */
	g = pc_geom;
	g.rot[0][0] = (int16_t)(vs[0] << 3);
	g.rot[0][1] = (int16_t)(vs[1] << 3);
	g.rot[0][2] = (int16_t)(ve[0] << 3);
	g.rot[1][0] = (int16_t)(vs[1] << 3);
	g.rot[1][1] = (int16_t)(ve[0] << 3);
	g.rot[1][2] = (int16_t)(ve[1] << 3);
	g.rot[2][0] = (int16_t)(vs[2] << 3);
	g.rot[2][1] = (int16_t)((uint32_t)((int32_t)vs[2] << 3) >> 16);
	g.rot[2][2] = (int16_t)(ve[2] << 3);

	p = geom_rtps(&g, (int16_t)(4096 - tNear), (int16_t)((uint32_t)(4096 - tNear) >> 16),
	              (int16_t)tNear, 12, 0, 0, 0);
	ds = p.sxy;
	z0 = (uint32_t)(int32_t)p.v.ir[2] >> 3;
	p = geom_rtps(&g, (int16_t)(4096 - tFar), (int16_t)((uint32_t)(4096 - tFar) >> 16),
	              (int16_t)tFar, 12, 0, 0, 0);

	b->ScrStart = ds;
	b->zStart = z0;
	b->ScrEnd = p.sxy;
	z1 = (uint32_t)(int32_t)p.v.ir[2] >> 3;
	b->zEnd = z1;

depth:

	/* a quarter of the nearer depth, at most 4095 */
	z0 = ((int)z0 < (int)z1 ? z0 : z1) >> 2;

	if ((int)z0 >= 4096)
		z0 = 4095;

	return (int)z0;
}

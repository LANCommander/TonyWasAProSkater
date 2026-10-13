/* M3dAsm_ClipLine__FP12SClipLineBuf (0x8008ae04) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Projects a 3D line segment (bit/display.cpp's poly-lines, buffer in the scratchpad) for a
 * LINE_F2/G2 primitive. Returns its ordering-table depth (0..4095) with ScrStart/ScrEnd set to the
 * screen end points, or -1 when it isn't drawn.
 *
 * Both end points go through RTPS relative to the camera (GTE rotation, translation and
 * projection as the caller set them). A short line (less than 1024 x 512 pixels across) that
 * projects without trouble is taken as it is. Anything else is clipped against the view frustum:
 * the signed distances of both end points from the frustum planes (light matrix + back colour,
 * colour matrix: rows of FrustumNormals) give, for each plane the line crosses, the fraction of the
 * line where it does; the visible part runs from the largest entering fraction tNear to the
 * smallest leaving one tFar. Its end points are then re-projected from those fractions with the
 * two view-space points (scaled by 8) loaded into the rotation matrix, so that
 * RT * (1 - t, 0, t) interpolates between them. The depth is a quarter of the nearer end's view z.
 *
 * GTE state left behind: the rotation matrix is restored, but the light matrix, back colour and
 * colour matrix keep the frustum planes, and V0/V1 the last values loaded. */
#include <sys/types.h>
#include <libgte.h>
#include <inline_c.h>

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

#define GTE_S(r) ((int)PC_GteGetData(r))

/* MIPS div: never traps (x / 0 gives -1 for x >= 0, else 1; INT_MIN / -1 gives INT_MIN) */
static int MipsDiv(int n, int d) {
	if (d == 0)
		return n >= 0 ? -1 : 1;

	if (d == -1)
		return (int)(0u - (unsigned)n);

	return n / d;
}

static unsigned Pack(int lo, int hi) {
	return (unsigned short)lo | (unsigned)hi << 16;
}

/* the point relative to the camera, in whole units, into V0 */
static void LoadRelative(const int p[3]) {
	const int *cam = pCurrentCamera->Position;

	PC_GteSetData(0, Pack((p[0] >> 12) - cam[0], (p[1] >> 12) - cam[1]));
	PC_GteSetData(1, (unsigned)((p[2] >> 12) - cam[2]));
}

/* Frustum planes 0-2 into the light matrix (offsets of 0 and 1 in the back colour), planes 3, 6
 * and 7 into the colour matrix */
static void LoadFrustumPlanes(void) {
	const SVECTOR *f = FrustumNormals;

	PC_GteSetCtrl(8, Pack(f[0].vx, f[0].vy));
	PC_GteSetCtrl(9, Pack(f[0].vz, f[1].vx));
	PC_GteSetCtrl(10, Pack(f[1].vy, f[1].vz));
	PC_GteSetCtrl(11, Pack(f[2].vx, f[2].vy));
	PC_GteSetCtrl(12, (unsigned short)f[2].vz);
	PC_GteSetCtrl(13, (unsigned)(int)f[0].pad);
	PC_GteSetCtrl(14, (unsigned)(int)f[1].pad);
	PC_GteSetCtrl(15, 0);
	PC_GteSetCtrl(16, Pack(f[3].vx, f[3].vy));
	PC_GteSetCtrl(17, Pack(f[3].vz, f[6].vx));
	PC_GteSetCtrl(18, Pack(f[6].vy, f[6].vz));
	PC_GteSetCtrl(19, Pack(f[7].vx, f[7].vy));
	PC_GteSetCtrl(20, (unsigned short)f[7].vz);
}

/* MVMVA with lm = 1 (negative results saturate to 0): the distances of V0 or V1 from three
 * planes into d[0..2]; returns the planes the point is outside of (IR1..IR3 saturated) as bits
 * shift+2, shift+1, shift. */
static unsigned PlaneDistances(unsigned op, int *d, int shift) {
	int i;

	PC_GteCommand(op);

	for (i = 0; i < 3; i++)
		d[i] = GTE_S(25 + i);

	return (PC_GteGetCtrl(31) >> 22 & 7) << shift;
}

int M3dAsm_ClipLine__FP12SClipLineBuf(SClipLineBuf *b) {
	unsigned flagS, flagE, sxyS, sxyE, outS, outE, z0, z1, rt[5], v[4], ds;
	int s[3], e[3], i, t, tNear, tFar;

	LoadRelative(b->Start);
	gte_rtps();

	flagS = PC_GteGetCtrl(31);
	sxyS = PC_GteGetData(14);

	for (i = 0; i < 3; i++)
		s[i] = GTE_S(25 + i); /* view space */

	LoadRelative(b->End);
	gte_rtps();

	b->ScrStart = sxyS;
	b->zStart = (unsigned)s[2];
	b->tNear = 0;
	b->tFar = 4096;
	flagE = PC_GteGetCtrl(31);
	sxyE = PC_GteGetData(14);

	for (i = 0; i < 3; i++)
		e[i] = GTE_S(25 + i);

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

	/* Bits 22-24 of t (IR saturation when t is the RTPS flags; otherwise t is the screen
	 * extent that was too large, which only passes when it's positive and under 4M, or the
	 * start's screen y, which passes unless negative) */
	if ((unsigned)t >> 22 & 7)
		return -1;

	if ((s[2] & e[2]) < 0)
		return -1; /* both behind the camera */

	/* view-space points: start in V0, end in V1 */
	PC_GteSetData(0, Pack(s[0], s[1]));
	PC_GteSetData(1, (unsigned)s[2]);
	PC_GteSetData(2, Pack(e[0], e[1]));
	PC_GteSetData(3, (unsigned)e[2]);
	LoadFrustumPlanes();

	outS = PlaneDistances(PCGTE_MVMVA(1, 1, 0, 1, 1), b->Dist[0], 3);      /* LLM * V0 + BK */
	outS |= PlaneDistances(PCGTE_MVMVA(1, 2, 0, 3, 1), b->Dist[0] + 3, 0); /* LCM * V0 */
	outE = PlaneDistances(PCGTE_MVMVA(1, 1, 1, 1, 1), b->Dist[1], 3);
	outE |= PlaneDistances(PCGTE_MVMVA(1, 2, 1, 3, 1), b->Dist[1] + 3, 0);

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

	/* RT = 8 * [[x0 y0 x1] [y0 x1 y1] [z0 . z1]]: RT * (1 - t, 0, t) = 8 * lerp(start, end, t) */
	for (i = 0; i < 5; i++)
		rt[i] = PC_GteGetCtrl(i);

	for (i = 0; i < 4; i++)
		v[i] = PC_GteGetData(i);

	v[0] = v[0] << 3 & 0xfff8fff8u;
	v[2] = v[2] << 3 & 0xfff8fff8u;
	PC_GteSetCtrl(0, v[0]);
	PC_GteSetCtrl(2, v[2]);
	PC_GteSetCtrl(1, (v[0] & 0xffff0000u) | (v[2] & 0xffff));
	PC_GteSetCtrl(3, v[1] << 3);
	PC_GteSetCtrl(4, v[3] << 3);

	PC_GteSetData(0, (unsigned)(4096 - tNear));
	PC_GteSetData(1, (unsigned)tNear);
	gte_rtps();

	ds = PC_GteGetData(14);
	z0 = PC_GteGetData(11) >> 3;
	PC_GteSetData(0, (unsigned)(4096 - tFar));
	PC_GteSetData(1, (unsigned)tFar);
	gte_rtps();

	b->ScrStart = ds;
	b->zStart = z0;
	b->ScrEnd = PC_GteGetData(14);
	z1 = PC_GteGetData(11) >> 3;
	b->zEnd = z1;

	for (i = 0; i < 5; i++)
		PC_GteSetCtrl(i, rt[i]);

depth:
	/* a quarter of the nearer depth, at most 4095 */
	z0 = ((int)z0 < (int)z1 ? z0 : z1) >> 2;

	if ((int)z0 >= 4096)
		z0 = 4095;

	return (int)z0;
}

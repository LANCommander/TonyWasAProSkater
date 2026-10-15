/* M3dAsm_LineColijProcessFaces__FP6SModelP16SProjectedVertexP9SLineInfoP4Item (0x8008ab28) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */

/* Second half of the per-face line test (M3dColij_LineToThisItem), after
 * M3dAsm_LineColijProcessVertices put the model's vertices in line space (the line is the +z axis
 * from 0 to Length). For each collidable face not wholly on one side of the line, the line (the
 * point x = y = 0) must lie inside the face's 2D outline: three or four outer products of the
 * vertices' (x, y) give the edge-side signs. Then the face plane is intersected with the line: with
 * the face normal n in line space, d = v0 . n must lie in [(Length+2) * nz, 0], and the hit
 * distance along the line is d / nz. A trigger face only records the item's model in
 * CollidedTriggerModel (when the query asks for trigger hits); a solid face nearer than the best
 * hit so far becomes the new hit in the SLineInfo and NearestHit. Finally the hit position (or the
 * end point for no hit) is interpolated along the line into info->Position.
 *
 * State used: the colour matrix (line space, set by the caller); this routine replaces the light
 * matrix's row 1 with each tested face's first vertex (llm[1][0] = 0) and row 3 with
 * (0, 0, Length+2), and the rotation's diagonal with the outer products' operands, as retail
 * leaves them. */
#include "linecolij.h"

extern unsigned int M3dColij_ZeroMask; /* face flag word bits allowed to be clear */
extern unsigned int M3dColij_OneMask;  /* face flag word bits that must be clear */
extern int CollidedTriggerModel;
/* the nearest hit so far (game/colij.h SHit; THPS1 keeps the hit face's normal pointer at 0x08) */
extern struct {
	Item *pItem;          /* 0x00 */
	unsigned int *pFace;  /* 0x04 */
	SVECTOR *pNormal;     /* 0x08 */
	int pad0c[2];         /* 0x0c */
	int PolyCentre[3];    /* 0x14 */
	int Dist;             /* 0x20 */
} NearestHit;

/* MIPS div: never traps (x / 0 gives -1 for x >= 0, else 1; INT_MIN / -1 gives INT_MIN) */
static int MipsDiv(int n, int d) {
	if (d == 0)
		return n >= 0 ? -1 : 1;

	if (d == -1)
		return (int)(0u - (unsigned)n);

	return n / d;
}

/* the line passes inside the face outline; returns 0 when it misses */
static int LineInsideFace(const SFace *f, const SProjectedVertex *pv) {
	const SProjectedVertex *a = &pv[f->VertexNums[0]];
	const SProjectedVertex *b = &pv[f->VertexNums[1]];
	const SProjectedVertex *c = &pv[f->VertexNums[2]];
	int s2, s3;

	GeomAcc e;

	/* (Ax, Bx, Cx) on the rotation diagonal (loaded as the words x | y << 16): the edge functions
	 * of the triangle at the origin, x_b*y_c - x_c*y_b and so on */
	geom_load_op_diag(&pc_geom, *(const unsigned *)a, *(const unsigned *)b, *(const unsigned *)c);
	e = LineCross(a->y, b->y, c->y);

	s2 = e.mac[1];
	s3 = e.mac[2];

	if (e.mac[0] < 0) {
		/* outside edge b-c: a quad can still contain it in its other triangle (d, b, c) */
		const SProjectedVertex *d = &pv[f->VertexNums[3]];

		if (f->Flags & FACE_TRIANGLE)
			return 0;

		pc_geom.rot[0][0] = d->x;
		pc_geom.rot[0][1] = d->y;
		e = LineCross(d->y, b->y, c->y);

		s2 = -e.mac[1];
		s3 = -e.mac[2];
	}

	return (s2 | s3) >= 0;
}

static void TestFace(SFace *f, const SProjectedVertex *pv, SVECTOR *normals, SLineInfo *info,
                     Item *item) {
	unsigned w = f->NormalAndFlags2;
	SVECTOR *n;
	const SProjectedVertex *a;
	int d, dmin, nz, t;
	GeomAcc r;

	if (w & M3dColij_OneMask)
		return;

	if (~(w | M3dColij_ZeroMask))
		return;

	if (((w >> 16) & 3) == 1)
		return;

	{
		const SFace *cf = f;
		unsigned out = pv[cf->VertexNums[0]].Outcode & pv[cf->VertexNums[1]].Outcode &
		               pv[cf->VertexNums[2]].Outcode & pv[cf->VertexNums[3]].Outcode;

		if (out & PV_ALL)
			return; /* all four (or three, the fourth repeats one) on one side */
	}

	if (!LineInsideFace(f, pv))
		return;

	/* the normal into line space (colour matrix, no translation, 16-bit results) */
	n = (SVECTOR *)((char *)normals + (w & 0xffff));
	r = geom_mvmva(pc_geom.lcm, 0, n->vx, n->vy, n->vz, 12, 0, 0);

	/* light matrix row 1 = the first vertex: d = a . n; row 3 = (0, 0, Length+2):
	 * dmin = (Length+2) * nz */
	a = &pv[f->VertexNums[0]];
	pc_geom.llm[0][0] = a->x;
	pc_geom.llm[0][1] = a->y;
	pc_geom.llm[0][2] = a->z;
	pc_geom.llm[1][0] = 0;

	nz = r.ir[2];
	r = geom_mvmva(pc_geom.llm, 0, r.ir[0], r.ir[1], r.ir[2], 0, 0, 0);

	d = r.mac[0];
	dmin = r.mac[2];

	if (d > 0 || d < dmin)
		return; /* the plane doesn't cross the line between start and end */

	if ((w >> 16) & FACE2_TRIGGER) {
		if (info->RecordTriggerZoneHits)
			CollidedTriggerModel = item->Model;

		return;
	}

	t = MipsDiv(d, nz);

	if ((int)((unsigned)info->Distance - (unsigned)t) <= 0)
		return;

	info->pItem = item;
	info->Distance = t;
	info->pFace = f;
	info->Model = dmin;
	NearestHit.pNormal = n;
	NearestHit.pFace = (unsigned int *)f;
}

void M3dAsm_LineColijProcessFaces__FP6SModelP16SProjectedVertexP9SLineInfoP4Item(
    SModel *model, SProjectedVertex *pv, SLineInfo *info, Item *item) {
	SVECTOR *normals = model->Vertices + model->NumVertices;
	SFace *f = (SFace *)(normals + model->NumNormals);
	int n = model->NumFaces, t, i;
	int16_t dir[3];
	int32_t start[3];
	GeomAcc p;

	pc_geom.llm[2][0] = 0;
	pc_geom.llm[2][1] = 0;
	pc_geom.llm[2][2] = (int16_t)(info->Length + 2);

	do {
		TestFace(f, pv, normals, info, item);

		f = (SFace *)((char *)f + f->Size);
	} while (--n);

	/* Position = Start + (End - Start) * Distance / Length (the direction and the ratio in 16
	 * bits each) */
	t = MipsDiv((int)((unsigned)info->Distance << 12), info->Length);

	for (i = 0; i < 3; i++) {
		start[i] = info->StartCoords[i];
		dir[i] = (int16_t)((int)((unsigned)info->EndCoords[i] - (unsigned)info->StartCoords[i]) >>
		                   12);
	}

	p = geom_gpl((int16_t)t, dir, start, 0, 0, 0, 0, 0);

	for (i = 0; i < 3; i++)
		info->Position[i] = p.mac[i];
}

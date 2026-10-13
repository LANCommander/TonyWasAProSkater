/* M3dAsm_LineColijProcessFaces__FP6SModelP16SProjectedVertexP9SLineInfoP4Item (0x8008ab28) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Second half of the per-face line test (M3dColij_LineToThisItem), after
 * M3dAsm_LineColijProcessVertices put the model's vertices in line space (the line is the +z axis
 * from 0 to Length). For each collidable face not wholly on one side of the line, the line (the
 * point x = y = 0) must lie inside the face's 2D outline: three or four GTE outer products of the
 * vertices' (x, y) give the edge-side signs. Then the face plane is intersected with the line: with
 * the face normal n in line space, d = v0 . n must lie in [(Length+2) * nz, 0], and the hit
 * distance along the line is d / nz. A trigger face only records the item's model in
 * CollidedTriggerModel (when the query asks for trigger hits); a solid face nearer than the best
 * hit so far becomes the new hit in the SLineInfo and NearestHit. Finally the hit position (or the
 * end point for no hit) is interpolated along the line into info->Position.
 *
 * GTE state used: the colour matrix (line space, set by the caller); this routine replaces light
 * matrix row 1 with each tested face's first vertex (L21 = 0) and row 3 with (0, 0, Length+2). */
#include "linecolij.h"

extern unsigned int M3dColij_ZeroMask; /* face flag word bits allowed to be clear */
extern unsigned int M3dColij_OneMask;  /* face flag word bits that must be clear */
extern int CollidedTriggerModel;
extern int NearestHit[]; /* SHit: pItem, pFace, Normal ... */

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

	/* (Ax, Bx, Cx) on the rotation diagonal: MACi = the edge functions of the triangle at the
	 * origin, x_b*y_c - x_c*y_b and so on */
	PC_GteSetCtrl(0, *(const unsigned *)a);
	PC_GteSetCtrl(2, *(const unsigned *)b);
	PC_GteSetCtrl(4, *(const unsigned *)c);
	LineCross(a->y, b->y, c->y);

	s2 = GTE_S(26);
	s3 = GTE_S(27);

	if (GTE_S(25) < 0) {
		/* outside edge b-c: a quad can still contain it in its other triangle (d, b, c) */
		const SProjectedVertex *d = &pv[f->VertexNums[3]];

		if (f->Flags & FACE_TRIANGLE)
			return 0;

		PC_GteSetData(10, (unsigned)b->y);
		PC_GteSetData(11, (unsigned)c->y);
		PC_GteSetCtrl(0, *(const unsigned *)d);
		PC_GteSetData(9, (unsigned)d->y);
		gte_op0();

		s2 = -GTE_S(26);
		s3 = -GTE_S(27);
	}

	return (s2 | s3) >= 0;
}

static void TestFace(SFace *f, const SProjectedVertex *pv, SVECTOR *normals, SLineInfo *info,
                     Item *item) {
	unsigned w = f->NormalAndFlags2;
	SVECTOR *n;
	const SProjectedVertex *a;
	int d, dmin, nz, t;

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

	/* the normal into line space (colour matrix, no translation) */
	n = (SVECTOR *)((char *)normals + (w & 0xffff));
	PC_GteSetData(0, *(unsigned *)&n->vx);
	PC_GteSetData(1, *(unsigned *)&n->vz);

	a = &pv[f->VertexNums[0]];
	gte_mvmva(1, 2, 0, 3, 0);
	/* light matrix row 1 = the first vertex: MAC1 = a . n; row 3 = (0, 0, Length+2):
	 * MAC3 = (Length+2) * nz */
	PC_GteSetCtrl(8, *(const unsigned *)a);
	PC_GteSetCtrl(9, (unsigned short)a->z);

	nz = GTE_S(11);
	gte_mvmva(0, 1, 3, 3, 0);

	d = GTE_S(25);
	dmin = GTE_S(27);

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
	NearestHit[2] = (int)n;
	NearestHit[1] = (int)f;
}

void M3dAsm_LineColijProcessFaces__FP6SModelP16SProjectedVertexP9SLineInfoP4Item(
    SModel *model, SProjectedVertex *pv, SLineInfo *info, Item *item) {
	SVECTOR *normals = model->Vertices + model->NumVertices;
	SFace *f = (SFace *)(normals + model->NumNormals);
	int n = model->NumFaces, t, i;

	PC_GteSetCtrl(11, 0);                            /* L31 = L32 = 0 */
	PC_GteSetCtrl(12, (unsigned)(info->Length + 2)); /* L33 */

	do {
		TestFace(f, pv, normals, info, item);

		f = (SFace *)((char *)f + f->Size);
	} while (--n);

	/* Position = Start + (End - Start) * Distance / Length (GTE GPL: MAC += IR * IR0) */
	t = MipsDiv((int)((unsigned)info->Distance << 12), info->Length);

	for (i = 0; i < 3; i++)
		PC_GteSetData(25 + i, (unsigned)info->StartCoords[i]);

	for (i = 0; i < 3; i++)
		PC_GteSetData(
		    9 + i,
		    (unsigned)((int)((unsigned)info->EndCoords[i] - (unsigned)info->StartCoords[i]) >> 12));

	PC_GteSetData(8, (unsigned)t);
	gte_gpl0();

	for (i = 0; i < 3; i++)
		info->Position[i] = GTE_S(25 + i);
}

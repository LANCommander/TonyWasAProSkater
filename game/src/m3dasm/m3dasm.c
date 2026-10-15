/* M3DASM.S: C interpretations of the renderer's hand-written assembly (declarations in
   game/m3dasm.h): bounding-sphere culling, the vertex transforms and outcodes, lighting and
   reflection mapping, ProcessPolys (subdivision and water clipping), super-item transforms and
   the line-collision preprocessors and clipping. Plain C, not byte-matching by design
   (reimpl.tsv, platform/pc/tests/test_asm.c). */
#include <stddef.h>
#include "game/scratchpad.h"
#include "vertex.h"
#include <limits.h>
#include "processpolys.h"
#include "m3dmaths.h"
#include <sys/types.h>
#include <libgpu.h>
#include "pc_render3d.h"
#include "lastvec.h"
#include "supertransforms.h"
#include "linecolij.h"
#include <libgte.h>
#include "pc_geom.h"

/* a world position, 20.12 or whole units (BoundingSpherePreprocessing's camera) */
struct Vec3 {
	int vx, vy, vz;
};
#define IF_CULLED      0x8000 /* the result: not visible this frame */
#define IF_NO_YON      0x2000 /* never beyond the far plane (and no far-plane test) */
#define IF_ALWAYS_SKIP 0x1000 /* culled whatever the caller skips */
#define IF_SCALED      0x0200 /* Scale applies: the sphere grows with the largest factor */
#define IF_NOBOX       0x0002 /* sphere test only */
/* The model's bounding box relative to the camera, at half scale like the sphere test's
 * position, as 8 corners in the scratchpad (corner c: bit 0 picks x's second box face, bit 1
 * y's, bit 2 z's). */
#define CORNERS ((SVECTOR *)SCRATCHPAD)
struct SCamera {
	uint32_t Style;   /* 0x00 */
	VECTOR Position;  /* 0x04 */
	VECTOR Focus;     /* 0x14 */
	int Distance;     /* 0x24 */
	int Height;       /* 0x28 */
	SVECTOR Angles;   /* 0x2c */
	MATRIX Transform; /* 0x34 */
};
extern struct SCamera *pCurrentCamera;
extern SVECTOR WaterNormal;
extern uint32_t RefMapU0V0;
/* the reflection map's texture-page origin, added to every uv */
/* The state this function borrows, as retail saved the GTE's control registers in the
 * scratchpad (the same words; the port works on a copy and leaves the state alone). */
struct SavedGte {
	uint32_t tr[3];  /* 0x00: translation */
	uint32_t of[3];  /* 0x0c: screen offset x, y, distance (sign-extended) */
	uint32_t llm[5]; /* 0x18: light matrix, packed pairs */
	uint32_t rt[5];  /* 0x2c: rotation, packed pairs */
};
#define SAVED ((struct SavedGte *)SCRATCHPAD)
#define VF_ALWAYS_FACING 0x10 /* in the vertex's pad: this and all later vertices are billboards */
uint32_t M3dAsm_ColourCode;
/* the water surface's normal (view space); .pad: its level */
/* WaterClip's output: faces made by clipping, drawn after the model's own (retail kept this
 * buffer in the code segment, right after the subdivision code). */
uint32_t ClippedFaceBuffer[16384];
#define CLIPPED_FACE_BUFFER ((uintptr_t)ClippedFaceBuffer)
/* a scratchpad word that holds an address (WaterClip's cursors) */
#define SCRPTR(o) SCRATCH_PTR(o)
typedef struct {
	PolyState ps;
	struct SProjectedVertex *pv; /* ProjectedVertices (RotatedVertices follow, 3200 bytes on) */
	uint32_t *colours;           /* pColourTable */
	uint32_t a2;                 /* the third argument's register, reused by WaterClip */
	int left;                    /* faces left after this one */
	uint32_t off[4];             /* the face's vertices: byte offsets into ProjectedVertices */
} PP;
/* A projected vertex's words (0: screen word, 1: depth | outcode << 16) and its rotated twin's
 * (0: view x | y << 16, 1: lighting / reflection-map uv word), by byte offset. */
#define PV32(pp, off, w) (((uint32_t *)((char *)(pp)->pv + (off)))[w])
#define RV32(pp, off, w) (((uint32_t *)((char *)(pp)->pv + 3200 + (off)))[w])
#define LIGHT(pp, off)   RV32(pp, off, 1)
/* ---------------------------------------------------------------------------------------------
 * Subdivision
 *
 * The face is cut into segs x segs pieces. Its screen-space area (twice the winding value, or a
 * huge value for near-plane faces) picks segs: sqrt(area) / 32 rounded up, at most 7. The grid's
 * vertices are worked out in the scratchpad, one 16-byte cell each, rows 128 bytes apart: the
 * rotation holds the three view-space corners (one per column) and a projection of the weights
 * (4096 in all) gives a weighted sum of them on screen; the light and colour matrices hold the
 * corners' uvs and colours the same way, and the same weights interpolate those into colour
 * words (the projection's depth cue is 4096 here, so that scaling only saturates them).
 * A cell: +0 screen word (clamped, with outcode bits), +8 uv (as a colour word), +12 colour.
 * Every word the renderer keeps in the scratchpad is at +4 of some cell, which the grid never
 * touches; subdivision saves its own state in such words as well. */
#define GRID        SCRATCHPAD
#define CELL(a, o)  (*(uint32_t *)((a) + (o)))
#define CELLH(a, o) (*(uint16_t *)((a) + (o)))
#define OFFSCREEN                                                                                  \
	0x6000e000u /* screen-word bits a piece is dropped for when all corners have one */
enum { SUB_COL = 1, SUB_UV = 2 };
/* gouraud / textured kinds of packet */
typedef struct {
	int kind; /* SUB_COL | SUB_UV: the packet's per-vertex words beyond the screen word */
	int tri;
	int segs;             /* pieces along an edge */
	int step;             /* the weight step between grid vertices, 0x20000 / sqrt(area) */
	uint32_t viewport;    /* xR | yB << 16: the screen words' outcode reference */
	uintptr_t start;      /* the face's packet (the first piece goes there) */
	uint32_t last;        /* size in bytes of the last piece made */
	uint32_t code;        /* flat kinds: the quad command word (code | colour) */
	uint32_t clut, tpage; /* textured kinds: their high halves, for the first two uvs */
	uint32_t and_out;     /* screen words ANDed over the piece being built (and OFFSCREEN) */
	uint32_t rgbc;        /* the packet's command word with the quad code: the cells' code */
} Sub;
/* A grid vertex as the packets take it. */
typedef struct {
	uint32_t xy, col, uv;
} GridVertex;
/* ---------------------------------------------------------------------------------------------
 * Water clipping
 *
 * A face with vertices on both sides of the water surface (outcode bit 0x80 set on those to drop)
 * is not drawn: WaterClip writes the part to keep as one or two new faces (copies of the
 * record with new vertices) to ClippedFaceBuffer, drawn after the model's faces. The new vertices
 * where the edges meet the water go to free ProjectedVertices slots. */
typedef struct {
	PP *pp;
	const uint32_t *face;
	uintptr_t out;   /* the next record in ClippedFaceBuffer */
	uintptr_t vtx;   /* the next free ProjectedVertices slot */
	uint32_t size;   /* the face record's size */
	uint32_t uv[2];  /* the uvs where the last two edges met the water, as colour words */
} Clip;
/* The face's uv halfwords (corners 0-3) and the edges ClipEdge01 ... ClipEdge32. */
#define FACE_UV(c, i) (((const uint16_t *)(c)->face)[(i) == 3 ? 13 : 8 + 2 * (i)])
#define OUT_UV(c, i) (((uint16_t *)(c)->out)[8 + (i)])
/* Packet word indices of the corners' screen words / colours / uvs, per packet type. */
static const int XY_F3[3] = { 2, 3, 4 }, XY_FT3[3] = { 2, 4, 6 }, XY_G3[3] = { 2, 4, 6 };
static const int XY_GT3[3] = { 2, 5, 8 }, XY_F4[4] = { 2, 3, 4, 5 }, XY_FT4[4] = { 2, 4, 6, 8 };
static const int XY_G4[4] = { 2, 4, 6, 8 }, XY_GT4[4] = { 2, 5, 8, 11 };
static const int COL_G[4] = { 1, 3, 5, 7 }, COL_GT[4] = { 1, 4, 7, 10 };
static const int UV_FT[4] = { 3, 5, 7, 9 }, UV_GT[4] = { 3, 6, 9, 12 };
extern SPSXRegion PSXRegion[];
void M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX(SVECTOR *angles, MATRIX *m);
void M3dMaths_TransposeMatrix__FP6MATRIX(MATRIX *m);
/* scratchpad words the retail code leaves behind */
#define SCR_MODEL   (*(SModel **)&SCRATCH_PTR(0x18)) /* last rotated item's model, item, &PSXRegion */
#define SCR_ITEM    (*(Item **)&SCRATCH_PTR(0x1c))
#define SCR_REGIONS (*(SPSXRegion **)&SCRATCH_PTR(0x20))
#define SCR_MATRIX  ((MATRIX *)(SCRATCHPAD + 0x28))  /* the item rotation, transposed */
#define SCR_LINE    ((LineSeg *)(SCRATCHPAD + 0x3c)) /* the sorted query line, world model units */
extern unsigned int M3dColij_ZeroMask;
/* face flag word bits allowed to be clear */
extern unsigned int M3dColij_OneMask;
/* face flag word bits that must be clear */
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
typedef struct {
	int Start[3];          /* 0x00: world, 20.12 */
	int End[3];            /* 0x0c */
	unsigned ScrStart;     /* 0x18: screen x | y << 16 */
	unsigned ScrEnd;       /* 0x1c */
	int tNear, tFar;       /* 0x20: the visible part of the line, fractions in 4.12 */
	unsigned zStart, zEnd; /* 0x28: view depths of the drawn end points */
	int Dist[2][6];        /* 0x30: start's and end's distances from the six frustum planes */
} SClipLineBuf;
extern SVECTOR FrustumNormals[];

/* The six view-frustum planes are the rows of the light matrix (planes 0-2, with the back
 * colour as their distances) and colour matrix (planes 3-5, through the camera). For each plane
 * the box corner to test is chosen by the signs of its normal: a negative component picks that
 * axis's second face. */
static void corner_choice(const int16_t m[3][3], SVECTOR *corner[3]) {
	int i;

	for (i = 0; i < 3; i++)
		corner[i] = &CORNERS[(m[i][0] < 0) | (m[i][1] < 0) << 1 | (m[i][2] < 0) << 2];
}

/* plane i's distance (>> 12, saturated to 16 bits) of a box corner: not negative if the corner is
 * not behind it */
static int corner_distance(int i, const SVECTOR *c) {
	GeomAcc d = i < 3 ? geom_mvmva(pc_geom.llm, pc_geom.bk, c->vx, c->vy, c->vz, 12, 0, 0)
	                  : geom_mvmva(pc_geom.lcm, 0, c->vx, c->vy, c->vz, 12, 0, 0);
	return d.ir[i % 3];
}

/* the sphere and box tests of one item; 1 if it may be visible */
static int item_visible(struct Item *it, uint32_t flags, SVECTOR *corner[6],
                        const struct Vec3 *cam) {
	int dx, dy, dz, r, i;
	int no_yon = flags & IF_NO_YON;
	struct SModel *model;
	GeomAcc near3, far3;

	/* the position relative to the camera, in whole units: it must fit in 16 bits */
	dx = (it->Position[0] >> 12) - cam->vx;

	if (dx >= 32767 || dx < -32767)
		return 0;

	dz = (it->Position[2] >> 12) - cam->vz;

	if (!no_yon && dz >= 32767)
		return 0;

	if (dz < -32767)
		return 0;

	dy = (it->Position[1] >> 12) - cam->vy;

	if (dy >= 32767 || dy < -32767)
		return 0;

	/* the centre's signed distances from the planes (at half scale, 16 bits per coordinate) */
	far3 = geom_mvmva(pc_geom.llm, pc_geom.bk, (int16_t)(dx >> 1), (int16_t)(dy >> 1),
	                  (int16_t)(dz >> 1), 12, 0, 0);
	near3 = geom_mvmva(pc_geom.lcm, 0, (int16_t)(dx >> 1), (int16_t)(dy >> 1),
	                   (int16_t)(dz >> 1), 12, 0, 0);

	model = PSXRegion[it->Region].ppModels[it->Model];
	r = (int)((uint32_t)model->Radius >> 13); /* 19.13 */

	if (flags & IF_SCALED) {
		int s = it->Scale[0];

		if (it->Scale[1] > s)
			s = it->Scale[1];

		if (it->Scale[2] > s)
			s = it->Scale[2];

		r = (int)((uint32_t)(r * s) >> 12);
	}

	/* the sphere wholly behind a plane (plane 0, the far plane, not for IF_NO_YON items) */
	r = -r;

	for (i = no_yon ? 1 : 0; i < 3; i++)
		if (far3.mac[i] < r)
			return 0;

	for (i = 0; i < 3; i++)
		if (near3.mac[i] < r)
			return 0;

	/* the box test, for unrotated, unscaled box-tested items */
	if (it->Angles[0] || it->Angles[1] || it->Angles[2] || (flags & (IF_SCALED | IF_NOBOX)))
		return 1;

	{
		/* per axis, the two box faces' coordinates */
		const short (*box)[2] = (const short (*)[2])model->Box;
		int x0 = (short)(dx + box[0][0]) >> 1, x1 = (dx + box[0][1]) >> 1;
		int y0 = (short)(dy + box[1][0]) >> 1, y1 = (dy + box[1][1]) >> 1;
		int z0 = (short)(dz + box[2][0]) >> 1, z1 = (dz + box[2][1]) >> 1;
		int c;

		for (c = 0; c < 8; c++) {
			CORNERS[c].vx = (short)(c & 1 ? x1 : x0);
			CORNERS[c].vy = (short)(c & 2 ? y1 : y0);
			CORNERS[c].vz = (short)(c & 4 ? z1 : z0);
		}
	}

	/* each plane's chosen corner must not be behind it */
	for (i = no_yon ? 1 : 0; i < 6; i++)
		if (corner_distance(i, corner[i]) < 0)
			return 0;

	return 1;
}

/* M3dAsm_BoundingSpherePreprocessing (0x800858ac) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* View-frustum culling of an item list before it is drawn, with the frustum planes the caller
 * loaded (M3d_Render): each item's flags get IF_CULLED set or cleared. Items with
 * any of the flags in skip (or IF_ALWAYS_SKIP) are culled outright; the others are tested by
 * bounding sphere (the model's radius, scaled for IF_SCALED items) and, when that passes and the
 * item is neither rotated nor scaled, by bounding box against the six planes. The box corners
 * are left in the scratchpad (0x1f800000-0x1f80003f).
 * test_asm: gte_out none */
void M3dAsm_BoundingSpherePreprocessing(struct Item *it, int skip) {
	SVECTOR *corner[6];
	struct Vec3 cam;

	if (it == NULL)
		return;

	skip |= IF_ALWAYS_SKIP;
	corner_choice(pc_geom.llm, &corner[0]);
	corner_choice(pc_geom.lcm, &corner[3]);

	cam = *(const struct Vec3 *)&pCurrentCamera->Position;

	do {
		uint32_t flags = it->Flags;

		if (!(flags & skip) && item_visible(it, flags, corner, &cam))
			it->Flags = (ushort)(flags & ~IF_CULLED);
		else
			it->Flags = (ushort)(flags | IF_CULLED);

		it = it->NextItem;
	} while (it != NULL);
}

/* Projects an ordinary model's vertices with the current rotation/translation: for each one
 * the screen position, depth and outcode go to ProjectedVertices and the view-space x, y to
 * RotatedVertices. Returns the AND of all the outcodes (0xff for no vertices), so the caller can
 * skip a model that is wholly off one side of the view. */
static int transform_item_vertices(SVECTOR *v, int n) {
	struct ClipBounds b;
	struct SProjectedVertex *pv;
	struct SRotatedVertex *rv;
	uint32_t all = 0xffff;

	if (n == 0)
		return 0xff;

	vtx_clip_bounds(&b);

	pv = ProjectedVertices;
	rv = RotatedVertices;

	for (; n != 0; n--, v++, pv++, rv++) {
		GeomProj p = vtx_project(v->vx, v->vy, v->vz);
		int z;
		uint32_t oc;

		rv->xy = vtx_pack(p.v.ir[0], p.v.ir[1]);
		z = p.v.ir[2];
		pv->XY = p.sxy;
		pv->z = (short)z;
		oc = vtx_outcode(vtx_outcode_bits(&b, pv->XY, z));
		pv->Outcode = (ushort)oc;
		all &= oc;
	}

	SCR_PROJECTED_END = pv;
	return (int)all;
}

/* M3dAsm_TransformAndOutcodeItemVertices__FP7SVECTORi (0x80085d28) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
int M3dAsm_TransformAndOutcodeItemVertices__FP7SVECTORi(SVECTOR *v, int n) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_ITEM, (R3dTransformFn)transform_item_vertices, v, n, 0);
}

static int transform_super_vertices(void *v, int n, void *o) {
	return vtx_transform_super((const uint32_t *)v, n, 0);
}

/* M3dAsm_TransformAndOutcodeSuperVertices__FP7SVECTORi (0x80085e98) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* Projects one part of a super (skinned) model, whose vertices are in 12.4 fixed point, into
 * ProjectedVertices/RotatedVertices with outcodes, sharing the vertices parts have in common
 * through the stitch pool (see vertex.h). Returns the AND of the outcodes (0xff for none). */
int M3dAsm_TransformAndOutcodeSuperVertices__FP7SVECTORi(SVECTOR *v, int n) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_SUPER, transform_super_vertices, v, n, 0);
}

static int transform_super_dw_vertices(void *v, int n, void *o) {
	return vtx_transform_super((const uint32_t *)v, n, 1);
}

/* M3dAsm_TransformAndOutcodeSuperVerticesDewibbled__FP7SVECTORi (0x800860cc) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* TransformAndOutcodeSuperVertices for "dewibbled" rendering: the 12.4 vertices go to the GTE
 * unscaled (the transform is set up 16 times larger, so the projection keeps the fraction bits
 * and the vertices don't jitter), and the rotated position and depth are scaled back (>> 4). */
int M3dAsm_TransformAndOutcodeSuperVerticesDewibbled__FP7SVECTORi(SVECTOR *v, int n) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_SUPER_DW, transform_super_dw_vertices, v, n, 0);
}

static int transform_super_fast_vertices(void *v, int n, void *o) {
	return vtx_transform_super_fast((const uint32_t *)v, n, 0);
}

/* M3dAsm_TransformAndOutcodeSuperVerticesFast__FP7SVECTORi (0x800862d4) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* The fast version of TransformAndOutcodeSuperVertices: screen words with folded-in clip flags
 * and depths go to the scratchpad cache (no rotated positions, no outcodes), stitched as usual.
 * Returns the AND of the screen words' flag bits 0x60006000 (0x60000000 for no vertices). */
int M3dAsm_TransformAndOutcodeSuperVerticesFast__FP7SVECTORi(SVECTOR *v, int n) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_SFAST, transform_super_fast_vertices, v, n, 0);
}

static int transform_super_fast_dw_vertices(void *v, int n, void *o) {
	return vtx_transform_super_fast((const uint32_t *)v, n, 1);
}

/* M3dAsm_TransformAndOutcodeSuperVerticesFastDewibbled__FP7SVECTORi (0x80086404) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* TransformAndOutcodeSuperVerticesFast for dewibbled rendering: the 12.4 vertices go to the GTE
 * unscaled and the depth is scaled back (>> 4); the screen position needs no scaling. */
int M3dAsm_TransformAndOutcodeSuperVerticesFastDewibbled__FP7SVECTORi(SVECTOR *v, int n) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_SFAST_DW, transform_super_fast_dw_vertices, v, n, 0);
}

/* M3dAsm_OutcodeVerticesInWater (0x80086500) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* .pad: the water level */
/* After a transform has filled ProjectedVertices/RotatedVertices for n vertices: marks which are
 * below the water. The second row of the rotation becomes the camera transform's second row (with
 * the translation's y = -water level), so a view-space vertex through it gives its height above
 * the water. Outcode bits 0x80/0x8000 are then toggled for the renderer's water clipping: those
 * set in `mask` (WaterClipValue) for a vertex above the water, the others for one below it. Left
 * changed, as retail leaves them: rot[0][2] (retail loads it with rot[1][0]), the second row and
 * the translation's y. */
void M3dAsm_OutcodeVerticesInWater(int n, int mask) {
	struct SProjectedVertex *pv = ProjectedVertices;
	struct SRotatedVertex *rv = RotatedVertices;
	MATRIX *t = &pCurrentCamera->Transform;

	pc_geom.rot[0][2] = t->m[0][2];
	pc_geom.rot[1][0] = t->m[1][0];
	pc_geom.rot[1][1] = t->m[1][1];
	pc_geom.rot[1][2] = t->m[1][2];
	pc_geom.tr[1] = -WaterNormal.pad;

	/* retail counts n down to zero after each vertex: n == 0 is never passed */
	do {
		GeomAcc h = geom_mvmva(pc_geom.rot, pc_geom.tr, (short)rv->xy, (int32_t)rv->xy >> 16,
		                       pv->z, 12, 0, 0);
		uint32_t below;

		below = (uint32_t)(int32_t)h.ir[1] >> 16; /* 0xffff if negative */
		pv->Outcode ^= (ushort)((below ^ (uint32_t)mask) & 0x8080);
		pv++;
		rv++;
	} while (--n != 0);
}

/* Projects background (sky) vertices: the translation is cleared first, so only the camera's
 * rotation applies and the sky stays put around the viewer. Each vertex gets its screen position
 * and outcode in ProjectedVertices, with the depth forced to 0x7fff (furthest) and no far-plane
 * test; nothing goes to RotatedVertices. Returns the AND of the outcodes (0xff for none). */
static int transform_background_vertices(SVECTOR *v, int n) {
	struct ClipBounds b;
	struct SProjectedVertex *pv;
	uint32_t all = 0xffff;

	if (n == 0)
		return 0xff;

	geom_load_vector(pc_geom.tr, 0, 0, 0);
	vtx_clip_bounds(&b);

	b.Yon = INT_MAX; /* never beyond the far plane */
	pv = ProjectedVertices;

	for (; n != 0; n--, v++, pv++) {
		GeomProj p = vtx_project(v->vx, v->vy, v->vz);
		uint32_t oc;

		pv->XY = p.sxy;
		pv->z = 0x7fff;
		oc = vtx_outcode(vtx_outcode_bits(&b, pv->XY, p.v.ir[2]));
		pv->Outcode = (ushort)oc;
		all &= oc;
	}

	return (int)all;
}

/* M3dAsm_TransformAndOutcodeBackgroundVertices__FP7SVECTORi (0x80086590) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
int M3dAsm_TransformAndOutcodeBackgroundVertices__FP7SVECTORi(SVECTOR *v, int n) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_BG, (R3dTransformFn)transform_background_vertices, v, n, 0);
}

/* a matrix as five words of pairs of 16-bit elements, the last one sign-extended */
static void pack_matrix(uint32_t w[5], const int16_t m[3][3]) {
	w[0] = vtx_pack(m[0][0], m[0][1]);
	w[1] = vtx_pack(m[0][2], m[1][0]);
	w[2] = vtx_pack(m[1][1], m[1][2]);
	w[3] = vtx_pack(m[2][0], m[2][1]);
	w[4] = (uint32_t)(int32_t)m[2][2];
}

/* 2560/4096 of a 4.12 matrix element */
static int scale_5_8(int m) {
	return (2560 * m) >> 12;
}

/* M3dAsm_GetReflectionMapping__FP6MATRIXPlPUi (0x800866cc) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Reflection-map texture coordinates for a model's vertices (environment mapping): for each
 * vertex, the eye vector (eye - vertex, in model space) is reflected about the vertex normal,
 * r = 2 (e.n) n - e, and r is projected with the world transform onto a screen of focal length
 * 1024 centred on 0; its screen x, y give u, v (64 steps across) relative to RefMapU0V0. The uv
 * word goes to the vertex's RotatedVertices Special word (ProjectedVertices + 3204) and to the
 * stitch pool for exported vertices; stitch vertices copy theirs from the pool.
 *
 * world: the model-to-world rotation; pos: the model's position relative to the camera, in world
 * space; the model's vertices follow its 28-byte header, then one normal per vertex.
 *
 * The light matrix is used as scratch: first the transposed (inverse) current rotation, column 1
 * scaled by 5/8, takes -pos into model space (the eye position); then its first row holds the
 * eye vector and the rest the identity, so one product gives e.n along with the normal. A slip in
 * the retail code (`or t4,t4,t4` where `or t4,t4,t5` was meant) drops the reflected y: the
 * vector projected is (r.x - e.x, -e.y, r.z - e.z) with the eye vector's halves subtracted as one
 * packed word. That is what the game shows, so it is kept.
 * test_asm: gte_out none */
void M3dAsm_GetReflectionMapping__FP6MATRIXPlPUi(MATRIX *world, int *pos, uint32_t *model) {
	struct SavedGte *s = SAVED;
	GeomState g = pc_geom;
	const int16_t(*r)[3] = pc_geom.rot;
	uint32_t eye_xy, uv0;
	int eye_z, n, i;
	const uint32_t *v, *nrm;
	char *pv, *pool, *top;
	GeomAcc a;

	for (i = 0; i < 3; i++)
		s->tr[i] = (uint32_t)pc_geom.tr[i];

	s->of[0] = (uint32_t)pc_geom.ofx;
	s->of[1] = (uint32_t)pc_geom.ofy;
	s->of[2] = (uint32_t)(int32_t)(int16_t)pc_geom.h;
	pack_matrix(s->llm, pc_geom.llm);
	pack_matrix(s->rt, pc_geom.rot);

	geom_load_vector(g.tr, 0, 0, 0);
	g.ofx = 0;
	g.ofy = 0;
	g.h = 1024;

	/* light matrix = transpose of the rotation, its first column scaled by 5/8 */
	g.llm[0][0] = (int16_t)scale_5_8(r[0][0]);
	g.llm[0][1] = r[1][0];
	g.llm[0][2] = r[2][0];
	g.llm[1][0] = (int16_t)scale_5_8(r[0][1]);
	g.llm[1][1] = r[1][1];
	g.llm[1][2] = r[2][1];
	g.llm[2][0] = (int16_t)scale_5_8(r[0][2]);
	g.llm[2][1] = r[1][2];
	g.llm[2][2] = r[2][2];

	geom_load_matrix(g.rot, &world->m[0][0]);

	/* the eye in model space */
	uv0 = RefMapU0V0;
	a = geom_mvmva(g.llm, 0, (int16_t)-pos[0], (int16_t)-pos[1], (int16_t)-pos[2], 12, 0, 0);

	n = ((short *)model)[1]; /* NumVertices */
	v = model + 7;           /* the vertices, after the 28-byte header */
	nrm = v + 2 * n;
	eye_xy = vtx_pack(a.mac[0], a.mac[1]);
	eye_z = a.mac[2];

	/* rows 2 and 3 of the light matrix: the identity (row 1 is set below) */
	memset(g.llm, 0, sizeof g.llm);
	g.llm[1][1] = 4096;
	g.llm[2][2] = 4096;

	pv = (char *)ProjectedVertices;
	pool = (char *)FreeStitchVertex;
	top = pv + STITCH_TOP;

	/* retail counts n down to zero after each vertex: models always have vertices */
	do {
		uint32_t flags = v[1] >> 16, uv, e_xy, sxy, w;
		int e_z;

		if (flags & VF_STITCH) {
			*(uint32_t *)(pv + 3204) = *(uint32_t *)(top - v[0] + 3204);
		} else {
			int16_t k[3];

			/* the eye vector: eye - vertex (x and y as one packed word) */
			e_xy = eye_xy - v[0];
			e_z = (eye_z - (int)v[1]) & 0xffff;
			g.llm[0][0] = (int16_t)e_xy;
			g.llm[0][1] = (int16_t)(e_xy >> 16);
			g.llm[0][2] = (int16_t)e_z;
			a = geom_mvmva(g.llm, 0, (short)nrm[0], (int32_t)nrm[0] >> 16, (short)nrm[1], 12,
			               0, 0); /* e.n, n.y, n.z */

			/* 2 (e.n) n */
			k[0] = (short)nrm[0];
			k[1] = a.ir[1];
			k[2] = a.ir[2];
			a = geom_gpf((int16_t)(a.ir[0] << 1), k, 12, 0, 0, 0, 0);
			w = ((uint32_t)a.ir[0] & 0xffff) - e_xy;
			sxy = geom_rtps(&g, (int16_t)w, (int16_t)(w >> 16),
			                (int16_t)((uint32_t)a.ir[2] - (uint32_t)e_z), 12, 0, 0, 0).sxy;

			uv = (((sxy >> 5) + 32) & 0x3f) | (((sxy >> 13) + 0x2000) & 0x3f00);
			uv += uv0;
			*(uint32_t *)(pv + 3204) = uv;

			if (flags & VF_EXPORT) {
				*(uint32_t *)(pool + 3204) = uv;
				pool -= 8;
			}
		}

		v += 2;
		nrm += 2;
		pv += 8;
	} while (--n != 0);
}

/* M3dAsm_GetDynamicLighting__FP7SVECTORi (0x80086a4c) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Lights a model's vertices from their normals (one per vertex, flags in the pad as for super
 * vertices): the light and colour matrices and back colour the caller set up, and a white
 * vertex colour (geom_nccs). The colour (4.12 fixed point, 16 bits per channel) is packed
 * 11:11:10 (the top bits of each channel) into the RotatedVertices Special word of the vertex
 * (ProjectedVertices + 3204), and
 * into the stitch pool for exported vertices; stitch vertices copy theirs from the pool. The
 * pool pointer itself is not advanced (the transform that follows does that). The loop tests
 * its count after each vertex, so n == 0 still lights one. The colour code (processpolys.h) is
 * left white, as retail leaves the GTE's RGBC.
 * test_asm: colourcode */
void M3dAsm_GetDynamicLighting__FP7SVECTORi(SVECTOR *normals, int n) {
	const uint32_t *v = (const uint32_t *)normals;
	char *pv = (char *)ProjectedVertices, *pool = (char *)FreeStitchVertex;
	char *top = pv + STITCH_TOP;

	M3dAsm_ColourCode = 0x00ffffff;

	do {
		uint32_t flags = v[1] >> 16, colour;
		GeomAcc l = geom_nccs(&pc_geom, (short)v[0], (int32_t)v[0] >> 16, (short)v[1],
		                      0x00ffffff, 12, 1, 0, 0);

		if (flags & VF_STITCH) {
			colour = *(uint32_t *)(top - v[0] + 3204);
		} else {
			colour = ((uint32_t)l.ir[0] & 0xffe0) >> 5 | ((uint32_t)l.ir[1] & 0xffe0) << 6 |
			         ((uint32_t)l.ir[2] & 0xffe0) << 17;

			if (flags & VF_EXPORT) {
				*(uint32_t *)(pool + 3204) = colour;
				pool -= 8;
			}
		}

		*(uint32_t *)(pv + 3204) = colour;
		pv += 8;
		v += 2;
	} while (--n > 0);
}

/* MIPS div: no trap on a zero divisor (the quotient is then -1, or 1 for a negative dividend) */
static int mips_div(int n, int d) {
	if (d == 0)
		return n >= 0 ? -1 : 1;

	return n / d;
}

/* AlwaysFacing_Block_Vertices (0x80086b10, only ever entered from the transform below, which
 * has just projected the first billboard vertex and drops the result).
 *
 * A billboard vertex is a screen-space offset from an already projected vertex (its first word:
 * the byte offsets in ProjectedVertices of that centre vertex, low half, and of a second vertex
 * that gives the direction, high half). Its offset is the size in its second word along the
 * screen direction from the centre to the second vertex, normalised; it is projected at the
 * centre's depth with x stretched by 10485/4096 (the rotation is replaced by that scale, which
 * stays, and the screen offset is cleared, then restored from the viewport) and added to the
 * centre's screen position. Outcodes as for the others, with the centre's depth. Returns the AND
 * of all the outcodes so far, `all`. */
static int always_facing(const SVECTOR *v, int n, struct SProjectedVertex *pv,
                         const struct ClipBounds *b, uint32_t all) {
	char *base = (char *)ProjectedVertices;
	int len = 0;

	memset(pc_geom.rot, 0, sizeof pc_geom.rot);
	pc_geom.rot[0][0] = 10485; /* 2.56 */
	pc_geom.rot[1][1] = 4096;
	pc_geom.rot[2][2] = 4096;
	pc_geom.ofx = 0;
	pc_geom.ofy = 0;

	for (; n != 0; n--, v++, pv++) {
		const uint32_t *w = (const uint32_t *)v;
		const struct SProjectedVertex *centre =
		    (const struct SProjectedVertex *)(base + (w[0] & 0xffff));
		const struct SProjectedVertex *toward =
		    (const struct SProjectedVertex *)(base + (w[0] >> 16));
		int cx = (short)centre->XY, cy = (int32_t)centre->XY >> 16, z = centre->z;
		int dx = (short)toward->XY - cx, dy = cy - ((int32_t)toward->XY >> 16);
		int16_t d[3];
		GeomAcc a;
		int sx, sy;
		uint32_t sxy, oc;

		/* the direction's length: the square root of dx^2 + dy^2 (16-bit dx, dy) */
		d[0] = (int16_t)dx;
		d[1] = (int16_t)dy;
		d[2] = 0;
		a = geom_sqr(d, 0, 1, 0);
		len = M3dMaths_SquareRoot0__Fi(a.mac[0] + a.mac[1]);

		/* (dy, dx) normalised (4.12) times the size: the offset, perpendicular to the direction */
		d[0] = (int16_t)mips_div(dy << 12, len);
		d[1] = (int16_t)mips_div(dx << 12, len);
		a = geom_gpf((int16_t)w[1], d, 12, 0, 0, 0, 0);
		sxy = vtx_project(a.mac[0], a.mac[1], z).sxy;
		sx = (short)sxy + cx;
		sy = ((int32_t)sxy >> 16) + cy;
		oc = vtx_outcode(vtx_outcode_xy(b, sx, sy, z));
		all &= oc;
		pv->XY = vtx_pack(sx, sy);
		pv->z = (short)z;
		pv->Outcode = (ushort)oc;
	}

	pc_geom.ofx = (int32_t)((uint32_t)pCurrentViewport->xC << 16);
	pc_geom.ofy = (int32_t)((uint32_t)pCurrentViewport->yC << 16);

	/* Retail stores v0 here as the end of ProjectedVertices, as the other transforms do, but v0
	 * then holds the last square root (the call clobbered it). Reproduced as it is. */
	SCR_PROJECTED_END = (struct SProjectedVertex *)(uintptr_t)(uint32_t)len;
	return (int)all;
}

/* Projects a block (a world model) moved by o (added to each vertex, 16-bit wrap) with the
 * current rotation, the translation cleared: screen position, depth and outcode to
 * ProjectedVertices, view-space x, y to RotatedVertices. A vertex whose projection overflowed
 * (any error in its FLAG) gets outcode bit 0x40 and loses its four screen bits. The vertices from
 * the first one flagged always-facing on are billboards (always_facing). Returns the AND of the
 * outcodes (0xff for no vertices). */
static int transform_block_vertices(SVECTOR *v, int n, VECTOR *o) {
	struct ClipBounds b;
	struct SProjectedVertex *pv;
	struct SRotatedVertex *rv;
	uint32_t all = 0xffff;

	if (n == 0)
		return 0xff;

	vtx_clip_bounds(&b);
	geom_load_vector(pc_geom.tr, 0, 0, 0);

	pv = ProjectedVertices;
	rv = RotatedVertices;

	for (; n != 0; n--, v++, pv++, rv++) {
		uint32_t flag, oc;
		GeomProj p = geom_rtps(&pc_geom, (int16_t)(v->vx + o->vx), (int16_t)(v->vy + o->vy),
		                       (int16_t)(v->vz + o->vz), 12, 0, 0, &flag);
		int z;

		if (v->pad & VF_ALWAYS_FACING)
			return always_facing(v, n, pv, &b, all);

		rv->xy = vtx_pack(p.v.ir[0], p.v.ir[1]);
		z = p.v.ir[2];
		pv->XY = p.sxy;
		pv->z = (short)z;
		oc = vtx_outcode_bits(&b, pv->XY, z);

		if (flag & 0x80000000u)
			oc = (oc | 0x40) & ~0xfu;

		oc = vtx_outcode(oc);
		pv->Outcode = (ushort)oc;
		all &= oc;
	}

	SCR_PROJECTED_END = pv;
	return (int)all;
}

/* M3dAsm_TransformAndOutcodeBlockVertices__FP7SVECTORiP6VECTOR (0x80086d2c) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
int M3dAsm_TransformAndOutcodeBlockVertices__FP7SVECTORiP6VECTOR(SVECTOR *v, int n, VECTOR *o) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_BLOCK, (R3dTransformFn)transform_block_vertices, v, n, o);
}

/* M3dAsm_SetAmbient__FPi (0x80086f20) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* The ambient light colour (three ints: r, g, b) becomes the background colour. */
void M3dAsm_SetAmbient__FPi(int *rgb) {
	geom_load_vector(pc_geom.bk, rgb[0], rgb[1], rgb[2]);
}

/* ---------------------------------------------------------------------------------------------
 * Near-plane faces */
/* M3dMaths_OriginAgainstPlaneTest (0x8008b894): which side of the plane through three view-space
 * points the eye is on. The plane normal is the first point x the second (unscaled, retail's OP
 * with the first point on the rotation's diagonal); it goes into the rotation's rows 2 and 3 as
 * rounded high halves and low halves, each dotted with the third point: returns about
 * (normal . third) >> 16, negative for a face turned away. The rotation is left as retail leaves
 * it. */
static int M3dMaths_OriginAgainstPlaneTest(const int16_t first[3], const int16_t second[3],
                                           const int16_t third[3]) {
	int16_t(*r)[3] = pc_geom.rot;
	uint32_t nx, ny, nz;
	GeomAcc n;

	n = geom_op(first, second, 0, 0, 0);
	nx = (uint32_t)n.mac[0];
	ny = (uint32_t)n.mac[1];
	nz = (uint32_t)n.mac[2];
	r[2][0] = (int16_t)nx;
	r[2][1] = (int16_t)ny;
	r[2][2] = (int16_t)nz;
	r[0][2] = (int16_t)(nx + 0x8000); /* (unused) */
	r[1][0] = (int16_t)((nx + 0x8000) >> 16);
	r[1][1] = (int16_t)((ny + 0x8000) >> 16);
	r[1][2] = (int16_t)((nz + 0x8000) >> 16);
	n = geom_mvmva(pc_geom.rot, 0, third[0], third[1], third[2], 0, 0, 0);
	return n.mac[1] + (n.mac[2] >> 16);
}

/* a view-space vertex: x, y (16 bits each) from the rotated twin, z from the projected one */
static void view_vertex(PP *pp, uint32_t off, int16_t v[3]) {
	uint32_t xy = RV32(pp, off, 0);

	v[0] = (int16_t)xy;
	v[1] = (int16_t)(xy >> 16);
	v[2] = (int16_t)PV32(pp, off, 1);
}

/* A face crossing the near plane has no usable screen winding: it is drawn (subdivided) if the
 * eye is on its front side, by its view-space vertices 0, 1, 2. */
static int near_face_visible(PP *pp) {
	int16_t a[3], b[3], c[3];

	view_vertex(pp, pp->off[0], a);
	view_vertex(pp, pp->off[1], b);
	view_vertex(pp, pp->off[2], c);
	/* (retail loads the first point as rotation words: rot[0][1] gets y and rot[1][2] 0) */
	pc_geom.rot[0][0] = a[0];
	pc_geom.rot[0][1] = a[1];
	pc_geom.rot[1][1] = a[1];
	pc_geom.rot[1][2] = 0;
	pc_geom.rot[2][2] = a[2];
	return M3dMaths_OriginAgainstPlaneTest(a, b, c) >= 0;
}

/* SubdivSetup (0x80087b3c): saves the walk's state in the scratchpad and picks segs and step. */
static void subdiv_setup(PP *pp, Sub *s, uint32_t area) {
	int n;

	/* retail's registers, in words the grid leaves alone (+4 of cells 1-11), reloaded by
	 * SubdivisionDone: face, ordering table, a2, faces left, packet, scratchpad base, depth,
	 * flags, (s6, s5: the caller's), colour table */
	SCR32(0x14) = (uint32_t)(uintptr_t)pp->ps.face;
	SCR32(0x24) = (uint32_t)(uintptr_t)pp->ps.ot;
	SCR32(0x34) = pp->a2;
	SCR32(0x44) = (uint32_t)pp->left;
	SCR32(0x54) = (uint32_t)pp->ps.pkt;
	SCR32(0x64) = SCRATCHPAD;
	SCR32(0x74) = (uint32_t)pp->ps.zmax;
	SCR32(0x84) = pp->ps.flags;
	SCR32(0xb4) = (uint32_t)(uintptr_t)pp->colours;
	/* the colour words below carry this code into the cells: the quad version of the face's */
	s->rgbc = PKT(pp->ps.pkt)[1] | CODE_QUAD_BIT;
	M3dAsm_ColourCode = s->rgbc;

	n = M3dMaths_SquareRoot0__Fi((int)area);

	if (224 - n <= 0)
		n = 224;

	s->step = n ? (int)(0x20000u / (uint32_t)n) : -1; /* (divu by zero gives all ones) */
	s->segs = (int)((uint32_t)(n + 31) >> 5);
	s->viewport = *(uint32_t *)pCurrentViewport;
	s->last = s->viewport; /* (no piece made yet) */
}

/* the bytes 0, 1, 2 of three words, x16, as the rows of a matrix (the corners in the columns) */
static void setup_bytes(int16_t m[3][3], uint32_t a, uint32_t b, uint32_t c, int rows) {
	int i;

	for (i = 0; i < 3; i++) {
		m[i][0] = i < rows ? (int16_t)((a >> 8 * i & 0xff) << 4) : 0;
		m[i][1] = i < rows ? (int16_t)((b >> 8 * i & 0xff) << 4) : 0;
		m[i][2] = i < rows ? (int16_t)((c >> 8 * i & 0xff) << 4) : 0;
	}
}

/* SetupUVs (0x80087bd8): the light matrix's rows get three corners' u and v (x16). */
static void setup_uvs(uint32_t a, uint32_t b, uint32_t c) {
	setup_bytes(pc_geom.llm, a, b, c, 2);
}

/* SetupRGBs (0x80087c3c): the colour matrix's rows get three corners' r, g and b (x16). */
static void setup_rgbs(uint32_t a, uint32_t b, uint32_t c) {
	setup_bytes(pc_geom.lcm, a, b, c, 3);
}

/* SetupXYs (0x80087cc0): the rotation's columns get three corners' view-space positions. */
static void setup_xys(PP *pp, uint32_t a, uint32_t b, uint32_t c) {
	int16_t va[3], vb[3], vc[3];
	int i;

	view_vertex(pp, a, va);
	view_vertex(pp, b, vb);
	view_vertex(pp, c, vc);

	for (i = 0; i < 3; i++) {
		pc_geom.rot[i][0] = va[i];
		pc_geom.rot[i][1] = vb[i];
		pc_geom.rot[i][2] = vc[i];
	}
}

/* The grid builder (0x80087d24): a triangle of grid vertices between the three corners in the
 * matrices (weights x, y, z for the columns a, b, c, 16 bits each), starting at cell `out`. Row r
 * (rows .. 0 counting down) has segs - rows + 1 vertices, dcol bytes apart, from corner c's
 * weight w toward corner a; rows are drow bytes apart, w falling by step each row (0 on the
 * last). The last row's last vertex is exactly corner b. */
static void subdiv_grid(const Sub *s, uintptr_t out, int w, int dcol, int drow, int rows) {
	int rgb = s->kind & SUB_COL;
	uint8_t code = (uint8_t)(s->rgbc >> 24);

	do {
		uintptr_t row = out;
		uint32_t next, wxy;
		int a, b, cols;

		if (rows == 0)
			w = 0;

		a = 4096 - w;
		b = 0;
		wxy = (uint32_t)a; /* the weights of columns a, b (16 bits each) */

		out -= dcol;
		cols = s->segs - rows;

		do {
			uint32_t xy, flag, d;
			int16_t wt[3] = { (int16_t)wxy, (int16_t)(wxy >> 16), (int16_t)w };
			GeomDepthQ dq;
			GeomProj p = geom_rtps(&pc_geom, wt[0], wt[1], wt[2], 12, 0, &dq, &flag);
			GeomAcc i;
			int sx, sy;

			out += dcol;
			a -= s->step;
			b += s->step;
			next = (uint32_t)b << 16 | (uint32_t)a; /* (a negative a spills into the y weight) */

			if (((cols - 1) | rows) == 0)
				next = 0x10000000; /* the far corner exactly */

			/* the screen word, clamped to what the GPU takes */
			xy = p.sxy;
			sy = (int32_t)xy >> 16;
			sx = (int16_t)xy;

			if (sy > 510)
				sy = 510;

			if (sx > 1022)
				sx = 1022;

			if (sx < -510)
				sx = -510;

			xy = ((uint32_t)sx & 0xffff) | (uint32_t)sy << 16;

			/* outcode bits as the fast transforms make them: 14/30 x >= xR / y >= yB, 15 the
			 * projection overflowed */
			d = xy - s->viewport;
			xy = ((xy & 0xbfffbfff) | ~(d | 0xbfffbfff)) & 0xffff7fff;
			xy |= flag >> 2 & 0x8000;
			CELL(out, 0) = xy;

			/* uv, colour: the weighted corners, saturated into colour words */
			i = geom_mvmva(pc_geom.llm, 0, wt[0], wt[1], wt[2], 12, 0, 0);
			geom_gpf(dq.ir0, i.ir, 12, 0, code, &CELL(out, 8), 0);

			if (rgb) {
				i = geom_mvmva(pc_geom.lcm, 0, wt[0], wt[1], wt[2], 12, 0, 0);
				geom_gpf(dq.ir0, i.ir, 12, 0, code, &CELL(out, 12), 0);
			}

			wxy = next;
		} while (cols-- != 0);

		w -= s->step;
		out = row + drow;
	} while (rows-- != 0);
}

/* The grid of a triangle (corners 0, 1, 2), or a quad's first half; with the corners' colour and
 * uv words from the face's packet (word indices col/uv[corner], when the kind has them). */
static void subdiv_half1(PP *pp, Sub *s, const int *col, const int *uv) {
	uint32_t *p = PKT(s->start);

	if (s->kind & SUB_UV)
		setup_uvs(p[uv[0]], p[uv[2]], p[uv[1]]);

	if (s->kind & SUB_COL)
		setup_rgbs(p[col[0]], p[col[2]], p[col[1]]);

	setup_xys(pp, pp->off[0], pp->off[2], pp->off[1]);
	subdiv_grid(s, GRID, 4096, 16, 128, s->segs);
}

/* A quad's second half (corners 3, 2, 1): the rest of the square grid, transposed. */
static void subdiv_half2(PP *pp, Sub *s, const int *col, const int *uv) {
	uint32_t *p = PKT(s->start);

	if (s->kind & SUB_UV)
		setup_uvs(p[uv[3]], p[uv[2]], p[uv[1]]);

	if (s->kind & SUB_COL)
		setup_rgbs(p[col[3]], p[col[2]], p[col[1]]);

	setup_xys(pp, pp->off[3], pp->off[2], pp->off[1]);
	subdiv_grid(s, GRID + 16, 4096 - s->step, 128, 16, s->segs - 1);
}

/* ExpandEdge (0x80087ee8): moves the grid vertices inside one edge (from a to b) a pixel
 * outward, so the pieces of neighbouring faces overlap instead of leaving gaps; the cursor walks
 * the edge's cells by stride and is returned past them. */
static uintptr_t expand_edge(uintptr_t cur, uint32_t a, uint32_t b, int stride, int segs) {
	uint32_t d = b - a;
	uint32_t dy = ((d & 0x2000) - 4096) << 4;   /* +-1 in y */
	uint32_t dx = (((int32_t)d >> 28) & 2) - 1; /* +-1 in x */
	uint32_t adj = dy - dx;
	int n = segs - 1;

	cur += stride;

	do {
		CELL(cur, 0) += adj;
		n--;
		cur += stride;
	} while (n > 0);

	return cur;
}

/* ExpandTri (0x80087f30), ExpandQuad (0x80087f74): the edges of the grid, given the corners'
 * screen words; one cursor walks all of them in turn. */
static void expand_tri(uint32_t xy0, uint32_t xy1, uint32_t xy2, int segs) {
	uintptr_t cur = GRID;

	cur = expand_edge(cur, xy1, xy2, 144, segs);
	cur = expand_edge(cur, xy2, xy0, -16, segs);
	expand_edge(cur, xy0, xy1, -128, segs);
}

static void expand_quad(uint32_t xy0, uint32_t xy1, uint32_t xy2, uint32_t xy3, int segs) {
	uintptr_t cur = GRID;

	cur = expand_edge(cur, xy1, xy3, 16, segs);
	cur = expand_edge(cur, xy3, xy2, 128, segs);
	cur = expand_edge(cur, xy2, xy0, -16, segs);
	expand_edge(cur, xy0, xy1, -128, segs);
}

static GridVertex grid_vertex(uintptr_t cell) {
	GridVertex v;

	v.xy = CELL(cell, 0);
	v.col = CELL(cell, 12);
	v.uv = CELLH(cell, 8);
	return v;
}

/* Writes a vertex's words at packet word i: [colour] screen word [uv | hi << 16]. */
static int put_vertex(uint32_t *p, int i, const Sub *s, const GridVertex *v, uint32_t hi) {
	if (s->kind & SUB_COL)
		p[i++] = v->col;

	p[i++] = v->xy;

	if (s->kind & SUB_UV)
		p[i++] = v->uv | hi;

	return i;
}

/* Ends a piece: unless all its corners share an off-screen bit, it becomes a packet of len
 * words linked to the next one. */
static void emit_piece(PP *pp, Sub *s, int len) {
	uintptr_t at = pp->ps.pkt;

	if (s->and_out != 0)
		return;

	pp->ps.pkt = at + PKT_SIZE(len);
	pp_tag(at, pp->ps.pkt, (uint32_t)len);
	s->last = (uint32_t)PKT_SIZE(len);
}

/* The words of a piece's first two corners (bottom-left b, top-left t). A flat piece starts with
 * its command word, a gouraud one with b's colour (the code in it); xor_code turns the quad code
 * into the triangle's. Returns the next word index. */
static int put_left(uint32_t *p, Sub *s, GridVertex *b, const GridVertex *t, uint32_t xor_code) {
	int i = 1;

	if (s->kind & SUB_COL)
		b->col ^= xor_code;
	else
		p[i++] = s->code ^ xor_code;

	i = put_vertex(p, i, s, b, s->clut);
	return put_vertex(p, i, s, t, s->tpage);
}

/* One row's quads (0x80087fc8, 0x80088140, 0x8008830c, 0x80088518 for the four kinds): n pieces
 * from the cursor's cells (row r: t, row r + 1: b) rightward. Returns 0 when the packet buffer
 * fills up. */
static int subdiv_quads(PP *pp, Sub *s, uintptr_t *cur, GridVertex *b, GridVertex *t, int n) {
	int len = (s->kind & SUB_COL ? 0 : 1) +
	          4 * (1 + (s->kind & SUB_COL ? 1 : 0) + (s->kind & SUB_UV ? 1 : 0));

	do {
		uint32_t *p = PKT(pp->ps.pkt), both;
		GridVertex b2, t2;
		int i;

		if (pp->ps.pkt >= pp->ps.limit) {
			/* (retail stores the first command word before it looks) */
			if (!(s->kind & SUB_UV))
				p[1] = (s->kind & SUB_COL) ? b->col : s->code;

			return 0;
		}

		i = put_left(p, s, b, t, 0);
		t2 = grid_vertex(*cur + 16);
		b2 = grid_vertex(*cur + 144);
		*cur += 16;
		both = b2.xy & t2.xy;
		s->and_out &= both;

		if (s->and_out == 0) {
			i = put_vertex(p, i, s, &b2, 0);
			put_vertex(p, i, s, &t2, 0);
		}

		emit_piece(pp, s, len);

		s->and_out = both & OFFSCREEN;
		*b = b2;
		*t = t2;
	} while (n-- > 1);

	return 1;
}

/* A row's last piece of a triangle grid: a triangle up to the next row's last vertex. */
static void subdiv_row_tri(PP *pp, Sub *s, uintptr_t cur, GridVertex *b, const GridVertex *t) {
	uint32_t *p = PKT(pp->ps.pkt);
	GridVertex d = grid_vertex(cur + 144);
	int i = put_left(p, s, b, t, CODE_QUAD_BIT), len;

	if (s->kind == SUB_COL) {
		/* G3: retail tests the colour word for being off-screen and stores the last corner's
		 * screen word and colour the wrong way round */
		s->and_out &= d.col;

		if (s->and_out == 0) {
			p[i] = d.xy;
			p[i + 1] = d.col;
		}

		len = 6;
	} else {
		s->and_out &= d.xy;

		if (s->and_out == 0)
			put_vertex(p, i, s, &d, 0);

		len = (s->kind & SUB_COL ? 0 : 1) +
		      3 * (1 + (s->kind & SUB_COL ? 1 : 0) + (s->kind & SUB_UV ? 1 : 0));
	}

	emit_piece(pp, s, len);
}

/* The pieces, row by row (0x80088024 ... SubdivideGT4). Returns 0 when the buffer fills up. */
static int subdiv_rows(PP *pp, Sub *s) {
	uintptr_t row = GRID;
	int r;

	for (r = 0; r < s->segs; r++) {
		uintptr_t cur = row;
		GridVertex t, b;

		if (pp->ps.pkt >= pp->ps.limit)
			return 0;

		t = grid_vertex(cur);
		b = grid_vertex(cur + 128);
		s->and_out = b.xy & t.xy & OFFSCREEN;

		if (s->tri) {
			if (r > 0 && !subdiv_quads(pp, s, &cur, &b, &t, r))
				return 0;

			subdiv_row_tri(pp, s, cur, &b, &t);
		} else if (!subdiv_quads(pp, s, &cur, &b, &t, s->segs)) {
			return 0;
		}

		row = (cur | 127) + 1;
	}

	return 1;
}

/* Subdivides the face whose packet was just built (kind, tri; the packet's word indices of the
 * corners' screen words, colours and uvs). Returns the size in bytes of the last piece made
 * (0 if none): the pieces are chained from the face's packet to pkt - size. */
static uint32_t subdivide(PP *pp, int kind, int tri, uint32_t area, const int *xyw, const int *colw,
                          const int *uvw) {
	Sub s;
	uint32_t *p;

	s.kind = kind;
	s.tri = tri;
	s.start = pp->ps.pkt;
	s.code = s.clut = s.tpage = 0;
	subdiv_setup(pp, &s, area);

	p = PKT(s.start);
	subdiv_half1(pp, &s, colw, uvw);

	if (!tri) {
		subdiv_half2(pp, &s, colw, uvw);
		expand_quad(p[xyw[0]], p[xyw[1]], p[xyw[2]], p[xyw[3]], s.segs);
	} else {
		expand_tri(p[xyw[0]], p[xyw[1]], p[xyw[2]], s.segs);
	}

	if (kind & SUB_UV) {
		s.clut = p[uvw[0]] & 0xffff0000;
		s.tpage = p[uvw[1]] & 0xffff0000;
	}

	if (!(kind & SUB_COL))
		s.code = s.rgbc;

	subdiv_rows(pp, &s);

	/* SubdivisionDone (0x80088744) */
	M3dAsm_ColourCode = 0;

	if (pp->ps.pkt == s.start)
		return 0;

	return s.last;
}

/* MIPS div: no trap on division by zero (the quotient is -1 or 1) or overflow. */
static int32_t clip_mips_div(int32_t n, int32_t d) {
	if (d == 0)
		return n >= 0 ? -1 : 1;

	if (d == -1 && n == INT32_MIN)
		return n;

	return n / d;
}

/* ClipEdge (0x800894b0): the point where the edge from vertex a to b meets the water, as a new
 * projected vertex (and its rotated twin), and the uv there (into c->uv). Retail does it all on
 * the GTE: the edge ends' heights against WaterNormal (the rotation's rows 1 and 3 hold a and b),
 * the position from a toward b by the ratio, its projection (with the identity rotation and no
 * translation, which stay), then the uvs the same way. Returns uvb << 16 | uva. */
static uint32_t clip_edge(Clip *c, uint32_t a, uint32_t b, uint32_t uva, uint32_t uvb) {
	PP *pp = c->pp;
	uint32_t uv = uvb << 16 | uva, xya = RV32(pp, a, 0), xyb = RV32(pp, b, 0), xy, z, rgb;
	int za = (int16_t)PV32(pp, a, 1), zb = (int16_t)PV32(pp, b, 1), da, t;
	int xa = (int16_t)xya, ya = (int32_t)xya >> 16;
	int16_t(*r)[3] = pc_geom.rot, d[3];
	int32_t base[3];
	GeomAcc h;
	GeomProj p;

	r[0][0] = (int16_t)xa; /* row 1: a */
	r[0][1] = (int16_t)ya;
	r[0][2] = (int16_t)za;
	r[1][0] = (int16_t)((uint32_t)za >> 16);
	r[2][0] = (int16_t)xyb; /* row 3: b */
	r[2][1] = (int16_t)(xyb >> 16);
	r[2][2] = (int16_t)zb;
	h = geom_mvmva(pc_geom.rot, 0, WaterNormal.vx, WaterNormal.vy, WaterNormal.vz, 12, 0, 0);

	da = h.mac[0];
	t = clip_mips_div((int32_t)((uint32_t)(-WaterNormal.pad - da) << 12), h.mac[2] - da);

	memset(pc_geom.rot, 0, sizeof pc_geom.rot);
	r[0][0] = r[1][1] = r[2][2] = 4096; /* identity rotation, no translation */
	geom_load_vector(pc_geom.tr, 0, 0, 0);

	geom_load_vector(base, xa, ya, za);
	d[0] = (int16_t)((int16_t)xyb - xa);
	d[1] = (int16_t)(((int32_t)xyb >> 16) - ya);
	d[2] = (int16_t)(zb - za);
	h = geom_gpl((int16_t)t, d, base, 12, 0, 0, 0, 0);

	xy = ((uint32_t)h.mac[0] & 0xffff) | (uint32_t)h.mac[1] << 16;
	z = (uint32_t)h.mac[2];

	p = geom_rtps(&pc_geom, (int16_t)xy, (int16_t)(xy >> 16), (int16_t)z, 12, 0, 0, 0);
	*(uint32_t *)(c->vtx + 3200) = xy;
	*(uint32_t *)(c->vtx + 4) = (z & 0xffff) | 0xff000000; /* outcode: inside everything */
	*(uint32_t *)c->vtx = p.sxy;
	c->vtx += 8;

	/* (the third component, unused, is what the projection left: its view z) */
	geom_load_vector(base, uv << 4 & 0xff0, uv >> 4 & 0xff0, p.v.mac[2]);
	d[0] = (int16_t)((uv >> 12 & 0xff0) - (uv << 4 & 0xff0));
	d[1] = (int16_t)((uv >> 20 & 0xff0) - (uv >> 4 & 0xff0));
	d[2] = p.v.ir[2];
	geom_gpl((int16_t)t, d, base, 12, 0, (uint8_t)(M3dAsm_ColourCode >> 24), &rgb, 0);
	c->uv[0] = c->uv[1];
	c->uv[1] = rgb;
	return uv;
}

static uint32_t clip(Clip *c, int a, int b) {
	return clip_edge(c, c->pp->off[a], c->pp->off[b], FACE_UV(c, a), FACE_UV(c, b));
}

/* The record's words other than vertices and uvs: flags (triangle or not), colour, lighting
 * offset, clut, tpage, texture window. */
static void copy_record(Clip *c, uint32_t tri) {
	uint32_t *o = (uint32_t *)c->out;

	o[0] = tri ? c->face[0] | FF_TRIANGLE : c->face[0] & ~FF_TRIANGLE;
	o[2] = c->face[2];
	o[3] = c->face[3];
	((uint16_t *)o)[9] = ((const uint16_t *)c->face)[9];
	((uint16_t *)o)[11] = ((const uint16_t *)c->face)[11];
	o[7] = c->face[7];
}

static void set_verts(Clip *c, uint32_t a, uint32_t b, uint32_t cc, uint32_t d) {
	uint8_t *o = (uint8_t *)c->out;

	o[4] = (uint8_t)a;
	o[5] = (uint8_t)b;
	o[6] = (uint8_t)cc;
	o[7] = (uint8_t)d;
}

/* the index of the n-th new vertex from the end (1: the last made) */
static uint32_t new_vertex(Clip *c, int n) {
	return (uint32_t)((c->vtx - (uintptr_t)c->pp->pv - 8 * (uint32_t)n) >> 3);
}

/* CopyTri (0x800896a0): a record for the kept triangle of a quad with one corner dropped. */
static void copy_tri(Clip *c) {
	copy_record(c, 1);

	SCR16(SCR_CLIPCOUNT)++;
}

/* MakeClippedTri (0x80089628): the kept corner a and the two new vertices. */
static void make_clipped_tri(Clip *c, int a) {
	uint32_t v = new_vertex(c, 2);

	set_verts(c, c->pp->off[a] >> 3, v, v + 1, v + 1);

	OUT_UV(c, 2) = (uint16_t)c->uv[0]; /* the two new uvs */
	OUT_UV(c, 4) = (uint16_t)c->uv[1];
	copy_record(c, 1);
}

/* MakeClippedQuad (0x800896ec): the kept corners a, b and the two new vertices. */
static void make_clipped_quad(Clip *c, int a, int b) {
	uint32_t v = new_vertex(c, 2);

	set_verts(c, c->pp->off[a] >> 3, c->pp->off[b] >> 3, v, v + 1);

	OUT_UV(c, 4) = (uint16_t)c->uv[0];
	OUT_UV(c, 5) = (uint16_t)c->uv[1];
	copy_record(c, 0);
}

/* A quad with corner `drop` under water and its opposite corner kept whole: the triangle of the
 * three corners other than `drop` (with `opp` moved to the dry side's outcode), then the quad
 * between that triangle's edge and the two new vertices. tri[]: the triangle's corners. */
static void clip_corner(Clip *c, int opp, const int tri[3]) {
	PP *pp = c->pp;
	uint16_t *oc = (uint16_t *)((char *)pp->pv + pp->off[opp] + 6);
	int i;

	*oc = (uint16_t)((*oc & 0xff7f) | 0x8000);
	copy_tri(c);
	set_verts(c, pp->off[tri[0]] >> 3, pp->off[tri[1]] >> 3, pp->off[tri[2]] >> 3,
	          pp->off[tri[2]] >> 3);

	for (i = 0; i < 3; i++)
		OUT_UV(c, 2 * i) = FACE_UV(c, tri[i]);

	c->out += c->size;
}

/* WaterClip (0x80088f90): which corners are under water (outcode bit 0x80) picks the case, as
 * retail lays them out (combinations it doesn't expect fall into one of them). */
static void water_clip(PP *pp, const uint32_t *face, const uint32_t *w1) {
	Clip c;
	int o0 = (w1[0] & 0x800000) != 0, o1 = (w1[1] & 0x800000) != 0;
	int o2 = (w1[2] & 0x800000) != 0, o3 = (w1[3] & 0x800000) != 0;
	uint32_t f = pp->ps.flags;

	c.pp = pp;
	c.face = face;
	c.vtx = SCRPTR(SCR_CLIPVERTEX);
	c.out = SCRPTR(SCR_CLIPWRITE);
	c.size = f >> 16;

	if (f & FF_TRIANGLE) {
		if (!o0 && !o1) {
			OUT_UV(&c, 0) = (uint16_t)(clip(&c, 2, 0) >> 16);
			OUT_UV(&c, 2) = (uint16_t)clip(&c, 1, 2);
			make_clipped_quad(&c, 0, 1);
		} else if (!o0 && !o2) {
			OUT_UV(&c, 0) = (uint16_t)(clip(&c, 1, 2) >> 16);
			OUT_UV(&c, 2) = (uint16_t)clip(&c, 0, 1);
			make_clipped_quad(&c, 2, 0);
		} else if (!o0) {
			goto keep0;
		} else if (o1) {
			OUT_UV(&c, 0) = (uint16_t)clip(&c, 2, 0);
			clip(&c, 1, 2);
			make_clipped_tri(&c, 2);
		} else if (o2) {
			OUT_UV(&c, 0) = (uint16_t)clip(&c, 1, 2);
			clip(&c, 0, 1);
			make_clipped_tri(&c, 1);
		} else {
			OUT_UV(&c, 0) = (uint16_t)(clip(&c, 0, 1) >> 16);
			OUT_UV(&c, 2) = (uint16_t)clip(&c, 2, 0);
			make_clipped_quad(&c, 1, 2);
		}
	} else if (!o0) {
		if (o1) {
			if (o2)
				goto keep0;

			if (o3) {
				OUT_UV(&c, 0) = (uint16_t)(clip(&c, 3, 2) >> 16);
				OUT_UV(&c, 2) = (uint16_t)clip(&c, 0, 1);
				make_clipped_quad(&c, 2, 0);
			} else {
				static const int t[3] = { 3, 2, 0 };

				clip_corner(&c, 2, t);

				OUT_UV(&c, 0) = (uint16_t)(clip(&c, 1, 3) >> 16);
				OUT_UV(&c, 2) = (uint16_t)clip(&c, 0, 1);
				make_clipped_quad(&c, 3, 0);
			}
		} else if (o2) {
			if (o3) {
				OUT_UV(&c, 0) = (uint16_t)(clip(&c, 2, 0) >> 16);
				OUT_UV(&c, 2) = (uint16_t)clip(&c, 1, 3);
				make_clipped_quad(&c, 0, 1);
			} else {
				static const int t[3] = { 0, 1, 3 };

				clip_corner(&c, 1, t);

				OUT_UV(&c, 0) = (uint16_t)(clip(&c, 2, 0) >> 16);
				OUT_UV(&c, 2) = (uint16_t)clip(&c, 3, 2);
				make_clipped_quad(&c, 0, 3);
			}
		} else {
			static const int t[3] = { 2, 0, 1 };

			clip_corner(&c, 0, t);

			OUT_UV(&c, 0) = (uint16_t)(clip(&c, 3, 2) >> 16);
			OUT_UV(&c, 2) = (uint16_t)clip(&c, 1, 3);
			make_clipped_quad(&c, 2, 1);
		}
	} else if (!o1) {
		if (o2) {
			if (o3) {
				OUT_UV(&c, 0) = (uint16_t)clip(&c, 1, 3);
				clip(&c, 0, 1);
				make_clipped_tri(&c, 1);
			} else {
				OUT_UV(&c, 0) = (uint16_t)(clip(&c, 0, 1) >> 16);
				OUT_UV(&c, 2) = (uint16_t)clip(&c, 3, 2);
				make_clipped_quad(&c, 1, 3);
			}
		} else {
			static const int t[3] = { 1, 3, 2 };

			clip_corner(&c, 3, t);

			OUT_UV(&c, 0) = (uint16_t)(clip(&c, 0, 1) >> 16);
			OUT_UV(&c, 2) = (uint16_t)clip(&c, 2, 0);
			make_clipped_quad(&c, 1, 2);
		}
	} else if (o2) {
		OUT_UV(&c, 0) = (uint16_t)clip(&c, 3, 2);
		clip(&c, 1, 3);
		make_clipped_tri(&c, 3);
	} else if (o3) {
		OUT_UV(&c, 0) = (uint16_t)clip(&c, 2, 0);
		clip(&c, 3, 2);
		make_clipped_tri(&c, 2);
	} else {
		OUT_UV(&c, 0) = (uint16_t)(clip(&c, 1, 3) >> 16);
		OUT_UV(&c, 2) = (uint16_t)clip(&c, 2, 0);
		make_clipped_quad(&c, 3, 2);
	}

	goto done;
keep0:
	/* corner 0 kept, 1 and 2 dropped (triangle; or quad corners 1, 2) */
	OUT_UV(&c, 0) = (uint16_t)clip(&c, 0, 1);
	clip(&c, 2, 0);
	make_clipped_tri(&c, 0);
done:
	/* (0x80089768) */
	SCR16(SCR_CLIPCOUNT)++;
	c.out += c.size;
	SCRPTR(SCR_CLIPVERTEX) = c.vtx;
	SCRPTR(SCR_CLIPWRITE) = c.out;
	pp->a2 = (uint32_t)c.out;
}

/* ---------------------------------------------------------------------------------------------
 * One face */
/* The corners' colours of a gouraud face from its corners' own (lit[corner]): rgb gets those of
 * corners 0-2 of a triangle, 1-3 of a quad, whose corner 0 is col0. */
static void corner_colours(const uint32_t *lit, int tri, uint32_t rgb[3], uint32_t *col0) {
	if (tri) {
		rgb[0] = lit[0];
		rgb[1] = lit[1];
		rgb[2] = lit[2];
	} else {
		*col0 = lit[0];
		rgb[0] = lit[1];
		rgb[1] = lit[2];
		rgb[2] = lit[3];
	}
}

/* Draws one face; returns 0 when the packet buffer is full (the walk stops). */
static int draw_face(PP *pp, const uint32_t *face) {
	PolyState *ps = &pp->ps;
	uint32_t gff = SCR32(SCR_FACEFLAGS);
	uint32_t f = (face[FACE_FLAGS] & (uint32_t)((int32_t)gff >> 16)) | (gff & 0xffff);
	uint32_t vi = face[FACE_VERTS], w1[4], oc, z, c, col0 = 0, len, xy[4], sxy[4];
	uintptr_t start, last;
	uint32_t rgb[3] = { 0, 0, 0 }, lit[4];
	int i, area, tri = (f & FF_TRIANGLE) != 0, nv = tri ? 3 : 4, gouraud, kind, k;
	const int *xyw, *colw = COL_G, *uvw = UV_FT;
	uint32_t *p;

	if (!(f & FF_DRAW))
		return 1;

	for (i = 0; i < 4; i++) {
		pp->off[i] = (vi >> 8 * i & 0xff) * 8;
		w1[i] = PV32(pp, pp->off[i], 1);
		sxy[i] = PV32(pp, pp->off[i], 0);
	}

	/* the screen winding of corners 0, 1, 2 and of 1, 2, 3 */
	PKT(ps->pkt)[2] = sxy[0];
	area = pp_nclip(sxy[0], sxy[1], sxy[2]);

	/* outcodes: drop the face if all corners are off one side; the high byte (complements)
	 * tells near-plane crossing (0x1000/0x4000) and water crossing (0x8000) */
	oc = (w1[0] & w1[1] & w1[2] & w1[3]) >> 16 ^ 0xff00;

	if (oc & 0x20ff)
		return 1;

	if (oc & 0x5000) {
		if (!(f & FF_SUBDIVIDE) || !near_face_visible(pp))
			return 1;

		area = 0x7fff0000; /* always subdivided */
	} else {
		int n1 = area, n2 = pp_nclip(sxy[1], sxy[2], sxy[3]);

		if (SCR16(SCR_MIRROR)) {
			n1 = (int)(0u - (uint32_t)n1);
			n2 = (int)(0u - (uint32_t)n2);
		}

		n2 = (int)(0u - (uint32_t)n2);

		if (tri) {
			if (n1 <= 0)
				return 1;
		} else {
			if (n1 <= 0 && n2 <= 0)
				return 1;

			if (n1 < n2)
				n1 = n2;
		}

		area = n1;
	}

	if (ps->pkt >= ps->limit)
		return 0; /* FullPolys */

	ps->face = face;
	ps->flags = f;

	if (oc & 0x8000) {
		water_clip(pp, face, w1);
		return 1;
	}

	/* the farthest corner (all four, also for a triangle) */
	z = w1[0] << 16;

	for (i = 1; i < 4; i++)
		if ((int32_t)(z - (w1[i] << 16)) < 0)
			z = w1[i] << 16;

	ps->zmax = (int32_t)z >> 16;

	if (pp_fog(ps) > 0 && (int32_t)SCR32(SCR_FADECOLOUR) >= 0) {
		/* the fade polygon's screen words */
		SCR32(0x384) = sxy[0];
		SCR32(0x394) = sxy[1];
		SCR32(0x3a4) = sxy[2];
		SCR32(0x3b4) = sxy[3];
	}

	f = ps->flags;
	k = ps->fog;

	/* colours (0x800877c0 ... GetAndTransferTint): flat, or each corner's for a gouraud face
	 * (rgb: see corner_colours; a flat face's in rgb[2]). (Retail pipelines the per-corner
	 * colour commands, so its first one works on what the GTE held, for nothing.) */
	c = face[FACE_COLOUR];
#define TABLE(i) pp->colours[c >> 8 * (i) & 0xff]
	if (!(f & FF_GOURAUD)) {
		M3dAsm_ColourCode = c;

		if ((f & (FF_VERTEXLIT | FF_TINTED)) == (FF_VERTEXLIT | FF_TINTED)) {
			/* each corner lit by its own lighting word: gouraud after all */
			for (i = 0; i < nv; i++)
				lit[i] = pp_dpcl(c, LIGHT(pp, pp->off[i]), k);

			corner_colours(lit, tri, rgb, &col0);
		} else if (f & FF_VERTEXLIT) {
			/* lit by the lighting word of the vertex the record names */
			rgb[2] = pp_dpcl(c, *(uint32_t *)((char *)pp->pv + ((const uint16_t *)face)[6] + 3204),
			                 k);
		} else if (f & FF_TINTED) {
			rgb[2] = pp_dpcl(c, SCR32(SCR_TINT), k);
		} else {
			rgb[2] = pp_dpcs(c, c, k);
		}

		gouraud = (f & (FF_VERTEXLIT | FF_TINTED)) == (FF_VERTEXLIT | FF_TINTED);
	} else {
		gouraud = 1;

		if (f & (FF_VERTEXLIT | FF_TINTED)) {
			uint32_t l = 0;

			if (!(f & FF_TINTED))
				l = *(uint32_t *)((char *)pp->pv + ((const uint16_t *)face)[6] + 3204);
			else if (!(f & FF_VERTEXLIT))
				l = SCR32(SCR_TINT);

			for (i = 0; i < nv; i++) {
				M3dAsm_ColourCode = TABLE(i);
				lit[i] = pp_dpcl(TABLE(i),
				                 (f & FF_VERTEXLIT) && (f & FF_TINTED) ? LIGHT(pp, pp->off[i]) : l, k);
			}

			corner_colours(lit, tri, rgb, &col0);
		} else if (tri) {
			/* with the code byte the colour code holds (the last face's) */
			uint32_t in[3] = { TABLE(0), TABLE(1), TABLE(2) };

			geom_dpct(&pc_geom, in, (uint8_t)(M3dAsm_ColourCode >> 24), (int16_t)k, 12, 0, rgb,
			          0);
		} else {
			uint32_t in[3] = { TABLE(1), TABLE(2), TABLE(3) };

			M3dAsm_ColourCode = TABLE(0);
			col0 = pp_dpcs(TABLE(0), TABLE(0), k);
			geom_dpct(&pc_geom, in, (uint8_t)(TABLE(0) >> 24), (int16_t)k, 12, 0, rgb, 0);
		}
	}
#undef TABLE

	/* the packet: corners 1-3 screen words, the colours as above */
	p = PKT(ps->pkt);

	{
		uint32_t uv0 = 0, uv1 = 0, uv23 = 0;
		int textured = (f & FF_TEXTURED) || (!gouraud && (f & FF_REFLECT));

		if (f & FF_TEXTURED) {
			uv0 = face[FACE_UV0];
			uv1 = face[FACE_UV1];
			uv23 = face[FACE_UV23];
		} else if (textured) {
			/* reflection mapping: the uvs are the vertices' special words */
			uv0 = LIGHT(pp, pp->off[0]) | SCR32(SCR_REFMAPCLUT);
			uv1 = LIGHT(pp, pp->off[1]) | SCR32(SCR_REFMAPTPAGE);
			uv23 = LIGHT(pp, pp->off[2]);

			if (!tri)
				uv23 |= LIGHT(pp, pp->off[3]) << 16;
		}

		uv1 |= (f & FF_ABR) << 14; /* the blend mode into the tpage */
		kind = (gouraud ? SUB_COL : 0) | (textured ? SUB_UV : 0);

		if (tri) {
			uint32_t s1 = sxy[1], s2 = sxy[2];

			switch (kind) {
				case 0:
					p[3] = s1;
					p[4] = s2;
					pp_code(ps, rgb[2], CODE_F3);

					len = 4;
					xyw = XY_F3;
					break;
				case SUB_UV:
					p[4] = s1;
					p[6] = s2;
					p[3] = uv0;
					p[5] = uv1;
					p[7] = uv23;
					pp_code(ps, rgb[2], CODE_FT3);

					len = 7;
					xyw = XY_FT3;
					break;
				case SUB_COL:
					p[4] = s1;
					p[6] = s2;
					pp_code(ps, rgb[0], CODE_G3);

					p[3] = rgb[1];
					p[5] = rgb[2];
					len = 6;
					xyw = XY_G3;
					break;
				default:
					p[5] = s1;
					p[8] = s2;
					p[3] = uv0;
					p[6] = uv1;
					p[9] = uv23;
					pp_code(ps, rgb[0], CODE_GT3);

					p[4] = rgb[1];
					p[7] = rgb[2];
					len = 9;
					xyw = XY_GT3;
					colw = COL_GT;
					uvw = UV_GT;
					break;
			}
		} else {
			uint32_t s1 = sxy[1], s2 = sxy[2], s3 = sxy[3];

			switch (kind) {
				case 0:
					p[3] = s1;
					p[4] = s2;
					p[5] = s3;
					pp_code(ps, rgb[2], CODE_F4);

					len = 5;
					xyw = XY_F4;
					break;
				case SUB_UV:
					p[4] = s1;
					p[6] = s2;
					p[8] = s3;
					p[3] = uv0;
					p[5] = uv1;
					p[7] = uv23;
					p[9] = uv23 >> 16;
					pp_code(ps, rgb[2], CODE_FT4);

					len = 9;
					xyw = XY_FT4;
					break;
				case SUB_COL:
					p[4] = s1;
					p[6] = s2;
					p[8] = s3;
					pp_code(ps, col0, CODE_G4);

					p[3] = rgb[0];
					p[5] = rgb[1];
					p[7] = rgb[2];
					len = 8;
					xyw = XY_G4;
					break;
				default:
					p[5] = s1;
					p[8] = s2;
					p[11] = s3;
					p[3] = uv0;
					p[6] = uv1;
					p[9] = uv23;
					p[12] = uv23 >> 16;
					pp_code(ps, col0, CODE_GT4);

					p[4] = rgb[0];
					p[7] = rgb[1];
					p[10] = rgb[2];
					len = 12;
					xyw = XY_GT4;
					colw = COL_GT;
					uvw = UV_GT;
					break;
			}
		}
	}

	/* SubdivTest: big faces become a chain of pieces */
	start = last = ps->pkt;

	if ((f & FF_SUBDIVIDE) && (int32_t)(2048u - (uint32_t)area) < 0) {
		uint32_t size = subdivide(pp, kind, tri, (uint32_t)area, xyw, colw, uvw);

		if (size == 0)
			return 1; /* every piece off-screen */

		last = ps->pkt - size;
		len = (uint32_t)(size - sizeof(P_TAG)) >> 2;
	}

	for (i = 0; i < 4; i++)
		xy[i] = SCR32(0x384 + 16 * i);

	pp_link(ps, start, last, len, xy);
	return 1;
}

/* M3dAsm_ProcessPolys__FPUiP7SVECTORi (0x80086f40) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Draws a model's faces from the ProjectedVertices cache a transform just filled: back-face and
 * outcode culling, fog, the face's colours (flat, gouraud, lit by the vertices' or the item's
 * lighting, depth-cued), GPU packets (F3/FT3/G3/GT3/F4/FT4/G4/GT4) linked
 * into the ordering table by the face's farthest depth. Big faces with the subdivide flag (and
 * those crossing the near plane) are cut into a grid of smaller ones, projected again from the
 * view-space vertices; faces crossing the water surface are clipped against it into new faces,
 * drawn after the others (see processpolys.h for the face record and flags).
 *
 * Also the retail module's internal entry points that only this function uses: 0x80087628
 * (FullPolys/Done), 0x80087668, GetVertexColour, the colour helpers (LitVertex3 ...
 * GetAndTransferTint), SubdivTest/SubdivSetup/SetupUVs/SetupRGBs/SetupXYs, the grid builder at
 * 0x80087d24, ExpandEdge/ExpandTri/ExpandQuad, the packet emitters 0x80087fc8 ... SubdivideGT4,
 * WaterClip, ClipEdge*, MakeClippedTri/CopyTri/MakeClippedQuad, and the near-plane facing test
 * M3dMaths_OriginAgainstPlaneTest (0x8008b894), which take and return values in registers. The
 * fade polygon (0x8008771c) and the linking are shared with ProcessPolysFast (processpolys.h).
 *
 * test_asm: kseg 0x800d1a54
 * (pPoly: the retail code stores it back without its top byte; the port keeps it in KSEG0.)
 * test_asm: ignore 0x1f800094 0x1f8000a4
 * (Subdivision spills its registers into free scratchpad words and reloads them when it is
 * done; these two hold the caller's s6 and s5, whose values only the retail code itself uses.)
 * test_asm: colourcode
 *
 * The matrices of the state (pc_geom) are the operands of the subdivision and the clipping, as
 * they were the GTE's in retail, and are left as retail leaves them. */
void M3dAsm_ProcessPolys__FPUiP7SVECTORi(uint32_t *faces, SVECTOR *normals, int count) {
	PP pp;
	const uint32_t *face = faces;
	int n = count;

	/* the native renderer draws the faces (pc_render3d.h) */
	if (R3d_ProcessPolys(0, faces, count))
		return;

	(void)normals;

	if (count == 0)
		return;

	pp_begin(&pp.ps);

	pp.pv = ProjectedVertices;
	pp.colours = pColourTable;
	pp.a2 = (uint32_t)count;
	geom_load_vector(pc_geom.tr, 0, 0, 0);
	pc_geom.dqb = 0x10000000; /* the subdivision's projections: a depth cue of 4096 */
	pc_geom.dqa = 0;

	SCRPTR(SCR_CLIPWRITE) = CLIPPED_FACE_BUFFER;
	SCR16(SCR_CLIPCOUNT) = 0;

	for (;;) {
		for (; n != 0; n--) {
			const uint32_t *next =
			    (const uint32_t *)((const char *)face +
				                   (((face[FACE_FLAGS] &
				                      (uint32_t)((int32_t)SCR32(SCR_FACEFLAGS) >> 16)) |
				                     (SCR32(SCR_FACEFLAGS) & 0xffff)) >>
				                    16));

			pp.left = n - 1;

			if (!draw_face(&pp, face))
				goto done;

			face = next;
		}

		/* then the faces water clipping made (which may add more) */
		n = SCR16(SCR_CLIPCOUNT);

		if (n == 0)
			break;

		face = (const uint32_t *)CLIPPED_FACE_BUFFER;
		SCR16(SCR_CLIPCOUNT) = 0;
	}
done:
	pp_end(&pp.ps);
}

/* An ordinary model's vertex moved by the offset o (added to each coordinate, 16-bit wrap),
 * projected. */
static GeomProj project_offset_vertex(const SVECTOR *v, const VECTOR *o) {
	return vtx_project(v->vx + o->vx, v->vy + o->vy, v->vz + o->vz);
}

/* Projects a model's vertices, moved by o, with the current rotation (the translation is
 * cleared) into the scratchpad's fast cache: screen word with clip flags (vtx_fast_word) and
 * depth. Returns the AND of the screen words (-1 for no vertices). (The retail loop is
 * software-pipelined and also projects the word pair just past the last vertex, for nothing.) */
static int transform_fast_vertices(SVECTOR *v, int n, VECTOR *o) {
	struct SViewport *vp;
	struct SFastVertex *out = FAST_CACHE;
	uint32_t xRyB, all = 0xffffffffu;
	int yon;

	geom_load_vector(pc_geom.tr, 0, 0, 0);

	if (n == 0) {
		return -1;
	}

	vp = pCurrentViewport;
	xRyB = vtx_pack(vp->xR, vp->yB);
	yon = (short)vp->Yon;

	for (; n != 0; n--, out++, v++) {
		GeomProj p = project_offset_vertex(v, o);
		int z = p.v.ir[2];

		out->XY = vtx_fast_word(p.sxy, z, xRyB, yon);
		out->z = (short)z;
		all &= out->XY;
	}

	return (int)all;
}

/* M3dAsm_TransformAndOutcodeVerticesFast__FP7SVECTORiP6VECTOR (0x80089790) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
int M3dAsm_TransformAndOutcodeVerticesFast__FP7SVECTORiP6VECTOR(SVECTOR *v, int n, VECTOR *o) {
	/* the native renderer keeps the vertices unrounded (pc_render3d.h) */
	return R3d_Transform(R3D_T_FAST, (R3dTransformFn)transform_fast_vertices, v, n, o);
}

/* M3dAsm_ProcessPolysFast__FPUiP7SVECTORi (0x800898ac) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* The faces of a model the fast transforms put in the scratchpad cache (vertex.h, SFastVertex):
 * as M3dAsm_ProcessPolys (processpolys.h) without GlobalFaceFlags, lighting, reflection mapping,
 * subdivision or water. The screen words carry their clip flags, which the GPU ignores, so they
 * go into the packets as they are. Faces with a corner beyond Yon (bit 31 clear) or all corners
 * off one screen side are dropped. Includes FullFast (0x80089d20) and LinktwinFast (0x80089d54).
 *
 * test_asm: kseg 0x800d1a54
 * (pPoly: the retail code stores it back without its top byte; the port keeps it in KSEG0.)
 * test_asm: colourcode
 *
 * When the packet buffer is full retail jumps to ProcessPolys' exit (0x80087628), which stores
 * pPoly and returns but restores registers from the wrong (40-byte) frame; this returns
 * normally. */
void M3dAsm_ProcessPolysFast__FPUiP7SVECTORi(uint32_t *faces, SVECTOR *normals, int count) {
	PolyState ps;
	const uint32_t *face = faces;
	uint32_t *colours = pColourTable;

	/* the native renderer draws the faces (pc_render3d.h) */
	if (R3d_ProcessPolys(1, faces, count))
		return;

	(void)normals;

	if (count == 0)
		return;

	pp_begin(&ps);

	for (; count != 0; count--, face = (const uint32_t *)((const char *)face + (face[0] >> 16))) {
		uint32_t vi = face[FACE_VERTS], f, xy[4], c, len, *p;
		int z[4], n1, n2, i, visible, tri;
		struct SFastVertex *v[4];

		for (i = 0; i < 4; i++) {
			v[i] = &FAST_CACHE[vi >> 8 * i & 0xff];
			xy[i] = v[i]->XY;
		}

		f = face[FACE_FLAGS];

		if ((int32_t)(xy[0] & xy[1] & xy[2] & xy[3]) >= 0 ||
		    (xy[0] & xy[1] & xy[2] & xy[3] & 0x6000e000))
			continue;

		if (!(f & FF_DRAW))
			continue;

		/* the windings of corners 0, 1, 2 and 1, 2, 3 (the words x4: the flags shift out of
		 * the 16-bit halves) */
		n1 = pp_nclip(xy[0] << 2, xy[1] << 2, xy[2] << 2);
		n2 = pp_nclip(xy[1] << 2, xy[2] << 2, xy[3] << 2);

		if (!SCR16(SCR_MIRROR))
			visible = n1 > 0 || n2 < 0;
		else
			visible = n1 < 0 || n2 > 0;

		if (!visible)
			continue;

		p = PKT(ps.pkt);
		p[2] = xy[0];

		if (ps.pkt >= ps.limit)
			break; /* FullFast */

		ps.face = face;
		ps.flags = f;

		for (i = 0; i < 4; i++)
			z[i] = v[i]->z;

		ps.zmax = z[0];

		for (i = 1; i < 4; i++)
			if (ps.zmax < z[i])
				ps.zmax = z[i];

		pp_fog(&ps);

		f = ps.flags;
		tri = (f & FF_TRIANGLE) != 0;

		c = face[FACE_COLOUR];

		if (!(f & FF_GOURAUD)) {
			uint32_t rgb = pp_dpcs(c, c, ps.fog);

			M3dAsm_ColourCode = c;

			if (!(f & FF_TEXTURED)) {
				p[3] = xy[1];
				p[4] = xy[2];

				if (tri) {
					pp_code(&ps, rgb, CODE_F3);

					len = 4;
				} else {
					p[5] = xy[3];
					pp_code(&ps, rgb, CODE_F4);

					len = 5;
				}
			} else {
				p[4] = xy[1];
				p[6] = xy[2];
				p[3] = face[FACE_UV0];
				p[5] = face[FACE_UV1] | (f & FF_ABR) << 14;
				p[7] = face[FACE_UV23];

				if (tri) {
					pp_code(&ps, rgb, CODE_FT3);

					len = 7;
				} else {
					p[8] = xy[3];
					p[9] = face[FACE_UV23] >> 16;
					pp_code(&ps, rgb, CODE_FT4);

					len = 9;
				}
			}
		} else {
			/* gouraud: the table colours of the corners depth-cued, corners 0-2 with the code byte
			 * the colour code holds (the last face's), corner 3 with its own (also for a
			 * triangle) */
			uint32_t in[3], col[3], col0, col3;

			in[0] = colours[c & 0xff];
			in[1] = colours[c >> 8 & 0xff];
			in[2] = colours[c >> 16 & 0xff];
			geom_dpct(&pc_geom, in, (uint8_t)(M3dAsm_ColourCode >> 24), (int16_t)ps.fog, 12, 0,
			          col, 0);
			M3dAsm_ColourCode = colours[c >> 24];
			col3 = pp_dpcs(M3dAsm_ColourCode, M3dAsm_ColourCode, ps.fog);

			if (!(f & FF_TEXTURED)) {
				p[4] = xy[1];
				p[6] = xy[2];

				col0 = col[0];
				p[3] = col[1];
				p[5] = col[2];

				if (tri) {
					pp_code(&ps, col0, CODE_G3);

					len = 6;
				} else {
					p[8] = xy[3];
					p[7] = col3;
					pp_code(&ps, col0, CODE_G4);

					len = 8;
				}
			} else {
				uint32_t uv23 = face[FACE_UV23];

				p[5] = xy[1];
				p[8] = xy[2];
				p[3] = face[FACE_UV0];
				p[6] = face[FACE_UV1] | (f & FF_ABR) << 14;
				p[9] = uv23;

				col0 = col[0] & 0xffffff;
				p[4] = col[1];
				p[7] = col[2];

				if (tri) {
					pp_code(&ps, col0, CODE_GT3);

					len = 9;
				} else {
					p[11] = xy[3];
					p[12] = uv23 >> 16;
					p[10] = col3;
					pp_code(&ps, col0, CODE_GT4);

					len = 12;
				}
			}
		}

		pp_link(&ps, ps.pkt, ps.pkt, len, xy);
	}

	pp_end(&ps);
}

/* CompactOTagR (0x80089ddc) -- C interpretation of hand-written assembly (M3DASM) */
/* Shortens the links of a reverse ordering table (ClearOTagR layout: ot[i] links to ot[i - 1],
 * drawn from ot[n - 1] down to ot[0]) once the primitives are in. In each run of empty entries
 * (an entry still linking to the next one down) the first entry, which the chain above leads
 * into, gets the link of the next entry that has primitives, so the run is skipped; ot[0] (the
 * terminator) ends the last run. The GPU then walks only the entries that draw something. */
void CompactOTagR(P_TAG *ot, int n) {
	P_TAG *link = &ot[n - 1]; /* the entry whose link is being extended */
	int skipping = 1;         /* inside a run of empty entries that starts at *link */
	int i;

	/* the native renderer's layers find each bucket by walking every entry */
	if (R3d_Native())
		return;

	for (i = n - 1; i > 0; i--) {
		int empty = ot[i].next == &ot[i - 1];

		if (skipping && !empty) {
			link->next = ot[i].next;
			skipping = 0;
		} else if (!skipping && empty) {
			link = &ot[i];
			skipping = 1;
		}
	}

	if (skipping)
		link->next = ot[0].next;
}

/* M3dAsm_TransformTwice__FP7SVectorP7SMatrixP6MATRIX (0x80089eac) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* Transforms v by the animation frame sm (rotation + short translation), then the result by m
 * (rotation + translation): v' = Rsm*v + Tsm, then Rm*v' + Tm, each >> 12 and saturated to 16
 * bits. The point is left as M3DMATHS' last vector (lastvec.h; retail left it in the GTE) for the
 * caller to read. State left as retail leaves it: rotation + translation = sm, light matrix = m,
 * background colour = m's translation (retail's second product runs on those).
 * test_asm: lastvec out */
void M3dAsm_TransformTwice__FP7SVectorP7SMatrixP6MATRIX(struct SVector *v, struct SMatrix *sm,
                                                        MATRIX *m) {
	GeomAcc a;

	geom_load_matrix(pc_geom.rot, &sm->m[0][0]);
	geom_load_vector(pc_geom.tr, sm->t[0], sm->t[1], sm->t[2]);
	geom_load_vector(pc_geom.bk, m->t[0], m->t[1], m->t[2]);
	a = geom_mvmva(pc_geom.rot, pc_geom.tr, v->vx, v->vy, v->vz, 12, 0, 0);
	geom_load_matrix(pc_geom.llm, &m->m[0][0]);
	M3dMaths_LastVec = geom_mvmva(pc_geom.llm, pc_geom.bk, a.ir[0], a.ir[1], a.ir[2], 12, 0, 0);
}

/* M3dAsm_SetTransVector__FP6Vector (0x80089f54) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* A 20.12 fixed-point position becomes the translation, as whole units (arithmetic shift right
 * by 12).
 *
 * The retail block also holds the label M3dAsm_NewSetRotTransMatrix__FP7SMatrix (0x80089f80),
 * which nothing calls; SetTransVector returns before it and does not fall through. */
void M3dAsm_SetTransVector__FP6Vector(struct Vector *v) {
	geom_load_vector(pc_geom.tr, v->vx >> 12, v->vy >> 12, v->vz >> 12);
}

/* M3dAsm_SetSuperTransformsDewibbled__FP6MATRIXT0P7SMatrix (0x80089fc8) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* Loads the GTE with the transforms for one part of a super item (see supertransforms.h):
 * rotation = A * Rc, translation = A * Tc + At, light matrix = B * Rc. Unlike
 * M3dAsm_SetSuperTransforms the frame's translation is used unshifted, so the result stays at
 * 16x scale (the "dewibbled" path projects at that scale for sub-pixel precision). */
void M3dAsm_SetSuperTransformsDewibbled__FP6MATRIXT0P7SMatrix(MATRIX *a, MATRIX *b,
                                                              struct SMatrix *c) {
	m3d_set_super_transforms(a, b, c, 0);
}

/* M3dAsm_SetSuperTransforms__FP6MATRIXT0P7SMatrix (0x8008a1c8) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* Loads the GTE with the transforms for one part of a super item (see supertransforms.h):
 * rotation = A * Rc, translation = A * (Tc >> 4) + At, light matrix = B * Rc. The frame's
 * translation is stored in 16ths of a unit, hence the >> 4. */
void M3dAsm_SetSuperTransforms__FP6MATRIXT0P7SMatrix(MATRIX *a, MATRIX *b, struct SMatrix *c) {
	m3d_set_super_transforms(a, b, c, 4);
}

/* rotate one point (16 bits per coordinate) by the current rotation (>> 12, saturated) */
static void Rotate(int *x, int *y, int *z) {
	GeomAcc r = geom_mvmva(pc_geom.rot, 0, (int16_t)*x, (int16_t)*y, (int16_t)*z, 12, 0, 0);

	*x = r.ir[0];
	*y = r.ir[1];
	*z = r.ir[2];
}

/* M3dAsm_LineColijPreprocessItems__FP4ItemiP9SLineInfoUs (0x8008a658) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* Coarse pass of a line-collision query over a linked item list (M3dColij_LineToItem): every item
 * whose model's bounding box the line can't touch is marked as already looked at (its Inquiry set
 * to this query's), so the per-face test skips it. Items that don't collide at all get the same
 * treatment.
 *
 * The test works in the item's model space: the line is made relative to the item, and for a
 * rotated item first checked against the bounding cube of the model's radius, then rotated by the
 * transposed item matrix (built in the scratchpad) and re-sorted. A scaled item's box is scaled.
 *
 * Retail quirk kept: the axis-swap flags of a rotated item's re-sorted line are toggled onto the
 * running flags and never reset, so every later unrotated item mirrors its box by them as well.
 *
 * test_asm: ignore 0x1f800000 0x1f800004 0x1f800008 0x1f80000c 0x1f800010 0x1f800014 0x1f800024
 * (the retail code parks its caller's s0-s5 and its return address there and restores them; a C
 * caller has no such registers, and nothing reads the slots) */
void M3dAsm_LineColijPreprocessItems__FP4ItemiP9SLineInfoUs(Item *item, int unused, SLineInfo *info,
                                                            unsigned short inquiry) {
	unsigned flip;

	if (!item)
		return;

	GetLineSegment(info, SCR_LINE, &flip);

	do {
		SModel *model;
		LineSeg l;
		LineBox box;
		static const int zero[3];
		int i;

		if (ItemNotCollidable(item->Flags))
			goto reject;

		if (item->Inquiry == inquiry)
			goto next;

		model = PSXRegion[item->Region].ppModels[item->Model];

		/* the line relative to the item */
		l = *SCR_LINE;

		for (i = 0; i < 3; i++) {
			l.lo[i] -= item->Position[i] >> 12;
			l.hi[i] -= item->Position[i] >> 12;
		}

		if (*(int *)&item->Angles[0] || item->Angles[2]) {
			int r = model->Radius >> 12;

			for (i = 0; i < 3; i++)
				if (l.lo[i] - r > 0 || l.hi[i] + r < 0)
					goto reject;

			SCR_MODEL = model;
			SCR_ITEM = item;
			SCR_REGIONS = PSXRegion;
			M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX((SVECTOR *)item->Angles, SCR_MATRIX);
			M3dMaths_TransposeMatrix__FP6MATRIX(SCR_MATRIX);
			geom_load_matrix(pc_geom.rot, &SCR_MATRIX->m[0][0]);

			/* into model space: lo and hi are the line's (sorted) end points */
			Rotate(&l.lo[0], &l.lo[1], &l.lo[2]);
			Rotate(&l.hi[0], &l.hi[1], &l.hi[2]);
			SortLineSegmentCoords(&l, &flip);
		}

		geom_load_op_diag(&pc_geom, (unsigned)(l.hi[0] - l.lo[0]), (unsigned)(l.hi[1] - l.lo[1]),
		                  (unsigned)(l.hi[2] - l.lo[2]));

		GetBoundingBox(model, zero, flip, &l, &box);

		if (item->Flags & ITEM_SCALED)
			for (i = 0; i < 3; i++) {
				/* low 32 bits of the product, as the retail mult/mflo */
				box.min[i] = (int)((unsigned)item->Scale[i] * (unsigned)box.min[i]) >> 12;
				box.max[i] = (int)((unsigned)item->Scale[i] * (unsigned)box.max[i]) >> 12;
			}

		if (LineToBoxTest(&l, &box))
			goto next;
	reject:
		item->Inquiry = inquiry;
	next:
		item = item->NextItem;
	} while (item);
}

/* M3dAsm_LineColijPreprocessItemsZoned__FPP4ItemiP9SLineInfoUs (0x8008a948) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* Coarse pass of a line-collision query over a zone's zero-terminated item array
 * (M3dColij_LineToItemZoned): every item whose model's bounding box, placed at the item's position,
 * the line can't touch is marked as already looked at (its Inquiry set to this query's), as are
 * items that don't collide at all. Rotation and scale are ignored here, and every item's model is
 * looked up in the region of the first item.
 *
 * test_asm: ignore 0x1f800000 0x1f800004 0x1f800008 0x1f80000c 0x1f800010 0x1f800014 0x1f800018
 * (the retail code parks its caller's s0-s6 there and restores them; a C caller has no such
 * registers, and nothing reads the slots) */
void M3dAsm_LineColijPreprocessItemsZoned__FPP4ItemiP9SLineInfoUs(Item **items, int unused,
                                                                  SLineInfo *info,
                                                                  unsigned short inquiry) {
	Item *item = *items;
	SModel **models;
	LineSeg l;
	unsigned flip;

	if (!item)
		return;

	GetLineSegment(info, &l, &flip);

	models = PSXRegion[item->Region].ppModels;

	do {
		if (ItemNotCollidable(item->Flags))
			goto reject;

		if (item->Inquiry == inquiry)
			goto next;

		{
			int pos[3], i;
			LineBox box;

			for (i = 0; i < 3; i++)
				pos[i] = item->Position[i] >> 12;

			GetBoundingBox(models[item->Model], pos, flip, &l, &box);

			if (LineToBoxTest(&l, &box))
				goto next;
		}
	reject:
		item->Inquiry = inquiry;
	next:
		item = *++items;
	} while (item);
}

/* M3dAsm_LineColijProcessVertices__FP6SModelP16SProjectedVertexiP6VECTOR (0x8008aa50) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
/* First half of the per-face line test (M3dColij_LineToThisItem): moves every vertex of a model
 * into line space (offset by the item position relative to the line start, then through the
 * colour matrix and translation the caller set up, which turn the line into the +z axis from 0 to
 * length). Each vertex is stored with an outcode: which side of the line it lies on in x and y,
 * and whether it's before the start or past the end in z. Returns the AND of all the outcodes
 * (masked to PV_ALL): non-zero when the whole model is on one side, so the line misses it. */
int M3dAsm_LineColijProcessVertices__FP6SModelP16SProjectedVertexiP6VECTOR(SModel *model,
                                                                           SLineVertex *out,
                                                                           int length,
                                                                           VECTOR *ofs) {
	SVECTOR *v = model->Vertices;
	int n = model->NumVertices;
	unsigned all = PV_ALL;

	do {
		/* colour matrix * the vertex (16 bits per coordinate) + translation */
		GeomAcc r = geom_mvmva(pc_geom.lcm, pc_geom.tr, (int16_t)(v->vx + ofs->vx),
		                       (int16_t)(v->vy + ofs->vy), (int16_t)(v->vz + ofs->vz), 12, 0, 0);
		int x = r.mac[0], y = r.mac[1], z = r.mac[2];
		unsigned code;

		code = 0;

		if (x < 0)
			code |= PV_XNEG;

		if (y < 0)
			code |= PV_YNEG;

		if (z < 0)
			code |= PV_ZNEG;

		if (length < z)
			code |= PV_ZFAR;

		if (x > 0)
			code |= PV_XPOS;

		if (y > 0)
			code |= PV_YPOS;

		out->x = (short)x;
		out->y = (short)y;
		out->z = (short)z;
		out->Outcode = (unsigned short)code;
		all &= code;
		out++;
		v++;
	} while (--n);

	return (int)all;
}

/* MIPS div: never traps (x / 0 gives -1 for x >= 0, else 1; INT_MIN / -1 gives INT_MIN) */
static int MipsDiv(int n, int d) {
	if (d == 0)
		return n >= 0 ? -1 : 1;

	if (d == -1)
		return (int)(0u - (unsigned)n);

	return n / d;
}

/* the line passes inside the face outline; returns 0 when it misses */
static int LineInsideFace(const SFace *f, const SLineVertex *pv) {
	const SLineVertex *a = &pv[f->VertexNums[0]];
	const SLineVertex *b = &pv[f->VertexNums[1]];
	const SLineVertex *c = &pv[f->VertexNums[2]];
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
		const SLineVertex *d = &pv[f->VertexNums[3]];

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

static void TestFace(SFace *f, const SLineVertex *pv, SVECTOR *normals, SLineInfo *info,
                     Item *item) {
	unsigned w = f->NormalAndFlags2;
	SVECTOR *n;
	const SLineVertex *a;
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
void M3dAsm_LineColijProcessFaces__FP6SModelP16SProjectedVertexP9SLineInfoP4Item(
    SModel *model, SLineVertex *pv, SLineInfo *info, Item *item) {
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

/* the point relative to the camera, in whole units (16 bits each), projected; flag: its FLAG */
static GeomProj ProjectRelative(const int p[3], unsigned *flag) {
	const int *cam = &pCurrentCamera->Position.vx;

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

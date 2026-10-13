/* M3dAsm_BoundingSpherePreprocessing (0x800858ac) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include <stddef.h>

#include "vertex.h"

/* the fields of M3D's Item, SModel, SPSXRegion and SCamera used here (decomp/include/game) */
struct Vec3 {
	int vx, vy, vz;
};

struct Item {
	ushort Flags;          /* 0x00 */
	ushort Inquiry;        /* 0x02 */
	struct Vec3 Position;  /* 0x04: 20.12 */
	short Angles[3];       /* 0x10 */
	ushort Model;          /* 0x16 */
	u_char Frame;          /* 0x18 */
	u_char TintIndex;      /* 0x19 */
	u_char Anim;           /* 0x1a */
	u_char Region;         /* 0x1b */
	struct Item *NextItem; /* 0x1c */
	uint32_t RGB;          /* 0x20 */
	short Scale[3];        /* 0x24: 4.12 */
};

struct SModel {
	ushort Flags;       /* 0x00 */
	ushort NumVertices; /* 0x02 */
	ushort NumNormals;  /* 0x04 */
	ushort NumFaces;    /* 0x06 */
	uint32_t Radius;    /* 0x08: 19.13 */
	short Box[3][2];    /* 0x0c: per axis, the two box faces' coordinates */
};

struct SPSXRegion {
	char pad[0x10];
	struct SModel **ppModels; /* 0x10 */
	char pad14[0x40 - 0x14];
};

struct SCamera {
	uint32_t Style;       /* 0x00 */
	struct Vec3 Position; /* 0x04 */
};

extern struct SPSXRegion PSXRegion[];
extern struct SCamera *pCurrentCamera;

#define IF_CULLED      0x8000 /* the result: not visible this frame */
#define IF_NO_YON      0x2000 /* never beyond the far plane (and no far-plane test) */
#define IF_ALWAYS_SKIP 0x1000 /* culled whatever the caller skips */
#define IF_SCALED      0x0200 /* Scale applies: the sphere grows with the largest factor */
#define IF_NOBOX       0x0002 /* sphere test only */

/* The model's bounding box relative to the camera, at half scale like the sphere test's
 * position, as 8 corners in the scratchpad (corner c: bit 0 picks x's second box face, bit 1
 * y's, bit 2 z's); the GTE loads them as vertices. */
#define CORNERS ((SVECTOR *)0x1f800000)

/* The six view-frustum planes are the rows of the GTE light matrix (planes 0-2, with the back
 * colour as their distances) and colour matrix (planes 3-5, through the camera). For each plane
 * the box corner to test is chosen by the signs of its normal: a negative component picks that
 * axis's second face. */
static SVECTOR *plane_corner(uint32_t x_neg, uint32_t y_neg, uint32_t z_neg) {
	return &CORNERS[x_neg | y_neg << 1 | z_neg << 2];
}

static void corner_choice(int mx, SVECTOR *corner[3]) {
	uint32_t w0 = gte_getc(mx + 0), w1 = gte_getc(mx + 1), w2 = gte_getc(mx + 2);
	uint32_t w3 = gte_getc(mx + 3), w4 = gte_getc(mx + 4);

	/* the sign bits of M11 M12 M13 / M21 M22 M23 / M31 M32 M33 in the packed words */
	corner[0] = plane_corner(w0 >> 15 & 1, w0 >> 31, w1 >> 15 & 1);
	corner[1] = plane_corner(w1 >> 31, w2 >> 15 & 1, w2 >> 31);
	corner[2] = plane_corner(w3 >> 15 & 1, w3 >> 31, w4 >> 15 & 1);
}

/* the sphere and box tests of one item; 1 if it may be visible */
static int item_visible(struct Item *it, uint32_t flags, SVECTOR *corner[6],
                        const struct Vec3 *cam) {
	int dx, dy, dz, r, i;
	int no_yon = flags & IF_NO_YON;
	struct SModel *model;
	int dist[6];

	/* the position relative to the camera, in whole units: it must fit the GTE's 16 bits */
	dx = (it->Position.vx >> 12) - cam->vx;

	if (dx >= 32767 || dx < -32767)
		return 0;

	dz = (it->Position.vz >> 12) - cam->vz;

	if (!no_yon && dz >= 32767)
		return 0;

	if (dz < -32767)
		return 0;

	dy = (it->Position.vy >> 12) - cam->vy;

	if (dy >= 32767 || dy < -32767)
		return 0;

	/* the centre's signed distances from the planes (at half scale) */
	gte_set(GTE_VXY0, vtx_pack(dx >> 1, dy >> 1));
	gte_set(GTE_VZ0, dz >> 1);
	gte_mvmva(1, MX_LLM, MV_V0, CV_BK, 0);

	model = PSXRegion[it->Region].ppModels[it->Model];
	r = (int)(model->Radius >> 13);

	if (flags & IF_SCALED) {
		int s = it->Scale[0];

		if (it->Scale[1] > s)
			s = it->Scale[1];

		if (it->Scale[2] > s)
			s = it->Scale[2];

		r = (int)((uint32_t)(r * s) >> 12);
	}

	dist[0] = gte_get(GTE_MAC1);
	dist[1] = gte_get(GTE_MAC2);
	dist[2] = gte_get(GTE_MAC3);
	gte_mvmva(1, MX_LCM, MV_V0, CV_NONE, 0);

	/* the sphere wholly behind a plane (plane 0, the far plane, not for IF_NO_YON items) */
	r = -r;

	for (i = no_yon ? 1 : 0; i < 3; i++)
		if (dist[i] < r)
			return 0;

	for (i = 0; i < 3; i++)
		if (gte_get(GTE_MAC1 + i) < r)
			return 0;

	/* the box test, for unrotated, unscaled box-tested items */
	if (it->Angles[0] || it->Angles[1] || it->Angles[2] || (flags & (IF_SCALED | IF_NOBOX)))
		return 1;

	{
		int x0 = (short)(dx + model->Box[0][0]) >> 1, x1 = (dx + model->Box[0][1]) >> 1;
		int y0 = (short)(dy + model->Box[1][0]) >> 1, y1 = (dy + model->Box[1][1]) >> 1;
		int z0 = (short)(dz + model->Box[2][0]) >> 1, z1 = (dz + model->Box[2][1]) >> 1;
		int c;

		for (c = 0; c < 8; c++) {
			CORNERS[c].vx = (short)(c & 1 ? x1 : x0);
			CORNERS[c].vy = (short)(c & 2 ? y1 : y0);
			CORNERS[c].vz = (short)(c & 4 ? z1 : z0);
		}
	}

	/* Each plane's chosen corner must not be behind it (IR, saturated). The loads of the next
	 * corner are interleaved with the tests as in retail, which leaves them in the GTE even when
	 * an earlier test culls; plane 0's x/y load happens even for IF_NO_YON items. */
	gte_set(GTE_VXY0, vtx_pack(corner[0]->vx, corner[0]->vy));

	if (!no_yon) {
		gte_set(GTE_VZ0, vtx_pack(corner[0]->vz, corner[0]->pad));
		gte_mvmva(1, MX_LLM, MV_V0, CV_BK, 0);

		if (gte_get(GTE_IR1) < 0)
			return 0;
	}

	gte_ldv1(corner[1]);
	gte_mvmva(1, MX_LLM, MV_V1, CV_BK, 0);

	i = gte_get(GTE_IR2);
	gte_ldv2(corner[2]);

	if (i < 0)
		return 0;

	gte_mvmva(1, MX_LLM, MV_V2, CV_BK, 0);

	i = gte_get(GTE_IR3);
	gte_ldv0(corner[3]);

	if (i < 0)
		return 0;

	gte_mvmva(1, MX_LCM, MV_V0, CV_NONE, 0);

	i = gte_get(GTE_IR1);
	gte_ldv1(corner[4]);

	if (i < 0)
		return 0;

	gte_mvmva(1, MX_LCM, MV_V1, CV_NONE, 0);

	i = gte_get(GTE_IR2);
	gte_ldv2(corner[5]);

	if (i < 0)
		return 0;

	gte_mvmva(1, MX_LCM, MV_V2, CV_NONE, 0);
	return gte_get(GTE_IR3) >= 0;
}

/* View-frustum culling of an item list before it is drawn, with the frustum planes the caller
 * loaded into the GTE (M3d_Render): each item's flags get IF_CULLED set or cleared. Items with
 * any of the flags in skip (or IF_ALWAYS_SKIP) are culled outright; the others are tested by
 * bounding sphere (the model's radius, scaled for IF_SCALED items) and, when that passes and the
 * item is neither rotated nor scaled, by bounding box against the six planes. The box corners
 * are left in the scratchpad (0x1f800000-0x1f80003f). */
void M3dAsm_BoundingSpherePreprocessing(struct Item *it, int skip) {
	SVECTOR *corner[6];
	struct Vec3 cam;

	if (it == NULL)
		return;

	skip |= IF_ALWAYS_SKIP;
	corner_choice(GTE_LLM, &corner[0]);
	corner_choice(GTE_LCM, &corner[3]);

	cam = pCurrentCamera->Position;

	do {
		uint32_t flags = it->Flags;

		if (!(flags & skip) && item_visible(it, flags, corner, &cam))
			it->Flags = (ushort)(flags & ~IF_CULLED);
		else
			it->Flags = (ushort)(flags | IF_CULLED);

		it = it->NextItem;
	} while (it != NULL);
}

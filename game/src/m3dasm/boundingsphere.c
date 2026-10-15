/* M3dAsm_BoundingSpherePreprocessing (0x800858ac) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include <stddef.h>
#include "game/scratchpad.h"

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

/* the whole entry, pointer fields as pointers, so the array strides right on x64 too */
struct SPSXRegion {
	char Filename[9];             /* 0x00 */
	u_char IsSuper;               /* 0x09 */
	u_char Usable;                /* 0x0a */
	u_char Protected;             /* 0x0b */
	uint32_t *pModelChecksums;    /* 0x0c */
	struct SModel **ppModels;     /* 0x10 */
	uint32_t *pPSX;               /* 0x14 */
	uint32_t *pAnimFile;          /* 0x18 */
	ushort *pHierarchy;           /* 0x1c */
	void *pHooks;                 /* 0x20 */
	uint32_t *pColourTable;       /* 0x24 */
	uint32_t *pTexWibData;        /* 0x28 */
	uint32_t *pColourPulseData;   /* 0x2c */
	ushort NumParts;              /* 0x30 */
	u_char Fastable;              /* 0x32 */
	u_char TexturesInRAM;         /* 0x33 */
	u_char KeepTexturesInRAM;     /* 0x34 */
	char pad35[0x38 - 0x35];
	void **ppTextures;            /* 0x38 */
	ushort NumModels;             /* 0x3c */
	ushort NumTextures;           /* 0x3e */
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
 * y's, bit 2 z's). */
#define CORNERS ((SVECTOR *)SCRATCHPAD)

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

	/* the centre's signed distances from the planes (at half scale, 16 bits per coordinate) */
	far3 = geom_mvmva(pc_geom.llm, pc_geom.bk, (int16_t)(dx >> 1), (int16_t)(dy >> 1),
	                  (int16_t)(dz >> 1), 12, 0, 0);
	near3 = geom_mvmva(pc_geom.lcm, 0, (int16_t)(dx >> 1), (int16_t)(dy >> 1),
	                   (int16_t)(dz >> 1), 12, 0, 0);

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

	/* each plane's chosen corner must not be behind it */
	for (i = no_yon ? 1 : 0; i < 6; i++)
		if (corner_distance(i, corner[i]) < 0)
			return 0;

	return 1;
}

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

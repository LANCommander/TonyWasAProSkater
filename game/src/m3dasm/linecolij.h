/* Shared pieces of the M3DASM line-collision preprocessors (linecolijpreprocess.c,
 * linecolijzoned.c, linecolijvertices.c, linecolijfaces.c): C views of the game structures they
 * walk, and the four internal routines of the retail module (GetLineSegment,
 * SortLineSegmentCoords, GetBoundingBox, LineToBoxTest at 0x8008a3d4..0x8008a657).
 *
 * The retail routines pass everything in registers: the line in s0..s5, the axis-swap flags in
 * v1, the box in t0..t5, the result in t0. Here they are static functions with the line, the flags
 * and the box as explicit arguments. As in retail, the line's per-axis lengths sit on the
 * rotation's diagonal (pc_geom.h; the state keeps them there) and the box tests are outer
 * products with them. */
#ifndef M3DASM_LINECOLIJ_H
#define M3DASM_LINECOLIJ_H

#include <sys/types.h>
#include <libgte.h>

#include "pc_geom.h"

/* M3D's world item (game/m3d.h Item; only what these routines touch is named) */
typedef struct Item {
	unsigned short Flags;    /* 0x00 */
	unsigned short Inquiry;  /* 0x02: last line query that looked at the item */
	int Position[3];         /* 0x04: 20.12 */
	short Angles[3];         /* 0x10 */
	unsigned short Model;    /* 0x16 */
	unsigned char Frame;     /* 0x18 */
	unsigned char TintIndex; /* 0x19 */
	unsigned char Anim;      /* 0x1a */
	unsigned char Region;    /* 0x1b */
	struct Item *NextItem;   /* 0x1c */
	unsigned int RGB;        /* 0x20 */
	short Scale[3];          /* 0x24: 4.12, used when Flags & ITEM_SCALED */
} Item;

#define ITEM_SCALED 0x200 /* Flags: Scale applies */

/* a PSX model (game/m3d.h SModel) */
typedef struct SModel {
	unsigned short Flags; /* 0x00 */
	short NumVertices;    /* 0x02 */
	short NumNormals;     /* 0x04 */
	short NumFaces;       /* 0x06 */
	int Radius;           /* 0x08: 20.12 */
	/* 0x0c: bounding box, one word per axis: the low halfword is the maximum, the high
	 * halfword the minimum (model units) */
	int Box[3];
	short zMax;             /* 0x18 */
	unsigned short NextLOD; /* 0x1a */
	SVECTOR Vertices[1];    /* 0x1c: then NumNormals normals, then the faces */
} SModel;

/* a model face (game/m3d.h SFace); variable size */
typedef struct SFace {
	unsigned short Flags;        /* 0x00: FACE_TRIANGLE ... */
	unsigned short Size;         /* 0x02: bytes to the next face */
	unsigned char VertexNums[4]; /* 0x04 */
	unsigned int Colour;         /* 0x08 */
	/* 0x0c: NormalOffset (bytes into the normals) | Flags2 << 16, tested as one word against
	 * M3dColij_ZeroMask / M3dColij_OneMask */
	unsigned int NormalAndFlags2;
} SFace;

#define FACE_TRIANGLE 0x10 /* Flags: three vertices, else four */
#define FACE2_TRIGGER 0x2  /* Flags2: a trigger face, not solid */

/* a loaded region (game/m3d.h SPSXRegion; sizeof 0x40): the whole entry, pointer fields as
   pointers, so the array strides right on x64 too */
typedef struct SPSXRegion {
	char Filename[9];
	unsigned char IsSuper, Usable, Protected;
	unsigned int *pModelChecksums;  /* 0x0c */
	SModel **ppModels;              /* 0x10 */
	unsigned int *pPSX;             /* 0x14 */
	unsigned int *pAnimFile;        /* 0x18 */
	unsigned short *pHierarchy;     /* 0x1c */
	void *pHooks;                   /* 0x20 */
	unsigned int *pColourTable;     /* 0x24 */
	unsigned int *pTexWibData;      /* 0x28 */
	unsigned int *pColourPulseData; /* 0x2c */
	unsigned short NumParts;        /* 0x30 */
	unsigned char Fastable;         /* 0x32 */
	unsigned char TexturesInRAM;    /* 0x33 */
	unsigned char KeepTexturesInRAM; /* 0x34 */
	char pad35[0x38 - 0x35];
	void **ppTextures;              /* 0x38 */
	unsigned short NumModels;       /* 0x3c */
	unsigned short NumTextures;     /* 0x3e */
} SPSXRegion;

/* a line-collision query and its result (game/colij.h SLineInfo) */
typedef struct SLineInfo {
	int StartCoords[3];                  /* 0x00: 20.12 */
	int EndCoords[3];                    /* 0x0c */
	int MinCoords[3];                    /* 0x18 */
	int MaxCoords[3];                    /* 0x24 */
	int iLo, iHi, jLo, jHi;              /* 0x30 */
	int Distance;                        /* 0x40: nearest hit so far, along the line, model units */
	int Length;                          /* 0x44: line length, model units */
	MATRIX WorldCst;                     /* 0x48 */
	Item *pItem;                         /* 0x68: the item hit */
	int Position[3];                     /* 0x6c: the hit point */
	SVECTOR Normal;                      /* 0x78 */
	SFace *pFace;                        /* 0x80 */
	int Model;                           /* 0x84 */
	unsigned char RecordTriggerZoneHits; /* 0x88 */
	unsigned char DropDown;              /* 0x89 */
	unsigned short Inquiry;              /* 0x8a */
} SLineInfo;

/* a model vertex in line space, as M3dAsm_LineColijProcessVertices leaves it */
typedef struct SLineVertex {
	short x, y;             /* across the line */
	short z;                /* along the line (low 16 bits) */
	unsigned short Outcode; /* PV_* */
} SLineVertex;

#define PV_ZNEG 0x001 /* behind the line start */
#define PV_YNEG 0x002
#define PV_XNEG 0x004
#define PV_ZFAR 0x008 /* beyond the line end */
#define PV_YPOS 0x200
#define PV_XPOS 0x400
#define PV_ALL  0x60f

/* An axis-aligned line segment with lo <= hi on every axis (model units). The flip bits (1 = x,
 * 2 = y, 4 = z) say which axes were swapped to get there: the line really runs from hi to lo on
 * those axes. */
typedef struct {
	int lo[3];
	int hi[3];
} LineSeg;

typedef struct {
	int min[3];
	int max[3];
} LineBox;

/* SortLineSegmentCoords (0x8008a408): put lo <= hi on each axis, toggling that axis's flip bit
 * when it swaps (the caller's flags are toggled, not reset), and load the per-axis lengths onto
 * the rotation's diagonal for the outer products in LineToBoxTest (as whole words: the high
 * halves land in rot[0][1] and rot[1][2]). */
static __inline__ void SortLineSegmentCoords(LineSeg *l, unsigned *flip) {
	int i, lens[3];

	for (i = 0; i < 3; i++) {
		int len = l->hi[i] - l->lo[i];

		if (len < 0) {
			int t = l->lo[i];

			l->lo[i] = l->hi[i];
			l->hi[i] = t;
			len = -len;
			*flip ^= 1u << i;
		}

		lens[i] = len;
	}

	geom_load_op_diag(&pc_geom, (unsigned)lens[0], (unsigned)lens[1], (unsigned)lens[2]);
}

/* GetLineSegment (0x8008a3d4): the query line in model units, sorted (flip starts at 0) */
static __inline__ void GetLineSegment(const SLineInfo *info, LineSeg *l, unsigned *flip) {
	int i;

	for (i = 0; i < 3; i++) {
		l->lo[i] = info->StartCoords[i] >> 12;
		l->hi[i] = info->EndCoords[i] >> 12;
	}

	*flip = 0;
	SortLineSegmentCoords(l, flip);
}

/* GetBoundingBox (0x8008a470): the model's box moved by ofs and grown by 2 on every side. On a
 * flipped axis the box is mirrored about the line's centre, so that testing it against the sorted
 * line is the same as testing the original box against the original line. */
static __inline__ void GetBoundingBox(const SModel *m, const int ofs[3], unsigned flip,
                                      const LineSeg *l, LineBox *b) {
	int i;

	for (i = 0; i < 3; i++) {
		int mn = (m->Box[i] >> 16) + ofs[i] - 2;
		int mx = (short)m->Box[i] + ofs[i] + 2;

		if (flip & (1u << i)) {
			int c = l->lo[i] + l->hi[i];

			b->min[i] = c - mx;
			b->max[i] = c - mn;
		} else {
			b->min[i] = mn;
			b->max[i] = mx;
		}
	}
}

/* Outer product of the line lengths (the rotation's diagonal) with (a, b, c) (16 bits each), no
 * shift: (Ly*c - Lz*b, Lz*a - Lx*c, Lx*b - Ly*a), with saturated 16-bit copies. */
static __inline__ GeomAcc LineCross(int a, int b, int c) {
	int16_t d[3], v[3];

	geom_op_diag(&pc_geom, d);
	v[0] = (int16_t)a;
	v[1] = (int16_t)b;
	v[2] = (int16_t)c;
	return geom_op(d, v, 0, 0, 0);
}

/* LineToBoxTest (0x8008a52c): does the sorted segment l hit box b? 1 if so.
 * First the boxes must overlap. Then, with everything relative to the line start, the line
 * direction L is crossed with box corners: the sign of L x corner says on which side of the line
 * the corner lies. The first cross product (with the far corner) picks the two box edges the line
 * could pass between; one more cross product with the corner between them decides. */
static __inline__ int LineToBoxTest(const LineSeg *l, const LineBox *b) {
	int far[3], near[3], i;
	GeomAcc c, n;

	for (i = 0; i < 3; i++)
		if (l->hi[i] - b->min[i] < 0)
			return 0;

	for (i = 0; i < 3; i++) {
		far[i] = b->max[i] - l->lo[i];

		if (far[i] < 0)
			return 0;
	}

	c = LineCross(far[0], far[1], far[2]);

	for (i = 0; i < 3; i++)
		near[i] = b->min[i] - l->lo[i];

	if (c.mac[0] < 0 && c.mac[1] >= 0) {
		n = LineCross(near[0], near[1], far[2]);
		return !(n.ir[0] < 0 || n.ir[1] > 0);
	}

	if (c.mac[0] >= 0 && c.mac[2] < 0) {
		n = LineCross(near[0], far[1], near[2]);
		return !(n.ir[0] > 0 || n.ir[2] < 0);
	}

	n = LineCross(far[0], near[1], near[2]);
	return !(n.ir[1] < 0 || n.ir[2] > 0);
}

/* Items the preprocessors reject outright (marked as already looked at): Flags 0x01 and 0x40 both
 * set, or 0x20 set */
static __inline__ int ItemNotCollidable(unsigned flags) {
	return (flags & 0x41) == 0x41 || (flags & 0x20);
}

#endif

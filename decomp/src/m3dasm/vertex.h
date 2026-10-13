/* Shared declarations for the C interpretations of the M3DASM vertex transforms, culling and
 * lighting (decomp/src/m3dasm/transform*.c, outcodewater.c, dynamiclighting.c,
 * reflectionmapping.c, boundingsphere.c). Plain C: the game headers (decomp/include/game) are C++.
 *
 * The transforms fill two caches, one record per model vertex:
 *   - ProjectedVertices (SProjectedVertex): the screen position (the GTE's SXY2 word), the view
 *     depth and an outcode halfword; RotatedVertices (SRotatedVertex, ProjectedVertices + 3200)
 *     the view-space x and y and a per-vertex word (lighting colour or reflection-map uv).
 *   - the "fast" paths write the scratchpad from 0x1f800000 instead (SFastVertex): the screen word
 *     with clip flags folded into it, and the depth.
 * Super (skinned) models share vertices between parts: a vertex flagged "export" is also copied
 * to a stitch pool that grows down from FreeStitchVertex, and a later part's "stitch" vertex is a
 * byte offset back from the pool's top that the transform copies the result from. */
#ifndef M3DASM_VERTEX_H
#define M3DASM_VERTEX_H

#include <stdint.h>
#include <sys/types.h>
#include <libgte.h>
#include <inline_c.h>

/* GTE data registers (mfc2/mtc2) */
enum {
	GTE_VXY0 = 0,
	GTE_VZ0 = 1,
	GTE_VXY1 = 2,
	GTE_VZ1 = 3,
	GTE_VXY2 = 4,
	GTE_VZ2 = 5,
	GTE_RGBC = 6,
	GTE_IR0 = 8,
	GTE_IR1 = 9,
	GTE_IR2 = 10,
	GTE_IR3 = 11,
	GTE_SXY2 = 14,
	GTE_MAC1 = 25,
	GTE_MAC2 = 26,
	GTE_MAC3 = 27
};

/* GTE control registers (cfc2/ctc2): matrices are five packed words each */
enum {
	GTE_RT = 0,
	GTE_TRX = 5,
	GTE_TRY = 6,
	GTE_TRZ = 7,
	GTE_LLM = 8,
	GTE_LCM = 16,
	GTE_OFX = 24,
	GTE_OFY = 25,
	GTE_H = 26,
	GTE_FLAG = 31
};

/* MVMVA operands (see PCGTE_MVMVA in inline_c.h) */
enum { MX_RT = 0, MX_LLM = 1, MX_LCM = 2 };
enum { MV_V0 = 0, MV_V1 = 1, MV_V2 = 2, MV_IR = 3 };
enum { CV_TR = 0, CV_BK = 1, CV_NONE = 3 };

#define gte_get(r)     ((int32_t)PC_GteGetData(r))
#define gte_set(r, v)  PC_GteSetData((r), (uint32_t)(v))
#define gte_getc(r)    PC_GteGetCtrl(r)
#define gte_setc(r, v) PC_GteSetCtrl((r), (uint32_t)(v))

/* Two 16-bit halves in one word (VXY, packed matrix pairs, screen words). */
static __inline__ uint32_t vtx_pack(int lo, int hi) {
	return ((uint32_t)lo & 0xffff) | ((uint32_t)hi << 16);
}

struct SViewport {
	ushort xR;      /* 0x00 */
	ushort yB;      /* 0x02 */
	ushort xL;      /* 0x04 */
	ushort yT;      /* 0x06 */
	ushort Hither;  /* 0x08 */
	ushort Yon;     /* 0x0a */
	ushort TanHalf; /* 0x0c */
	ushort Zoom;    /* 0x0e */
	ushort xC;      /* 0x10: the GTE screen offset (OFX) */
	ushort yC;      /* 0x12: OFY */
};

struct SProjectedVertex {
	uint32_t XY;    /* 0x00: the GTE's SXY2 (x low, y high) */
	short z;        /* 0x04: view depth (IR3) */
	ushort Outcode; /* 0x06: see vtx_outcode */
};

struct SRotatedVertex {
	uint32_t xy;      /* 0x00: view-space x, y (IR1, IR2) */
	uint32_t Special; /* 0x04: lighting colour / reflection-map uv */
};

/* the scratchpad cache of the fast transforms */
struct SFastVertex {
	uint32_t XY; /* 0x00: see vtx_fast_word */
	short pad;   /* 0x04: not written */
	short z;     /* 0x06 */
};

#define FAST_CACHE      ((struct SFastVertex *)0x1f800000)
#define FAST_STITCH_TOP 0x1f8003f8u /* stitch offsets of the fast paths count back from here */
#define STITCH_TOP      3192        /* ... and of the others from ProjectedVertices + 3192 */

/* where the last item-format transform stopped writing ProjectedVertices */
#define SCR_PROJECTED_END (*(struct SProjectedVertex **)0x1f8003d4)

/* super-model vertex flags (the high half of the vertex's second word) */
#define VF_EXPORT 1 /* also copy the result to the stitch pool */
#define VF_STITCH 2 /* the first word is a byte offset back from the pool's top: copy that */

extern struct SViewport *pCurrentViewport;
extern struct SProjectedVertex *ProjectedVertices;
extern struct SRotatedVertex *RotatedVertices;
extern struct SProjectedVertex *FreeStitchVertex;

/* The viewport's clip bounds as the outcodes compare against them. */
struct ClipBounds {
	int xR, yB, xL, yT, Hither, Yon;
};

static __inline__ void vtx_clip_bounds(struct ClipBounds *b) {
	struct SViewport *vp = pCurrentViewport;

	b->xR = vp->xR;
	b->yB = vp->yB;
	b->xL = vp->xL;
	b->yT = vp->yT;
	b->Hither = vp->Hither;
	b->Yon = vp->Yon;
}

/* Outcode bits of a projected vertex: 1 right of xR, 2 left of xL, 4 below yB, 8 above yT,
 * 0x10 nearer than Hither, 0x20 beyond Yon. Behind the camera (z < 0) the projection is
 * mirrored, so the four screen bits are inverted. */
static __inline__ uint32_t vtx_outcode_xy(const struct ClipBounds *b, int sx, int sy, int z) {
	uint32_t oc = 0;

	oc |= (uint32_t)(sx > b->xR) << 0;
	oc |= (uint32_t)(sx < b->xL) << 1;
	oc |= (uint32_t)(sy > b->yB) << 2;
	oc |= (uint32_t)(sy < b->yT) << 3;
	oc |= (uint32_t)(z < b->Hither) << 4;
	oc |= (uint32_t)(z > b->Yon) << 5;

	if (z < 0)
		oc ^= 0xf;

	return oc;
}

/* the same for a screen word (SXY2) */
static __inline__ uint32_t vtx_outcode_bits(const struct ClipBounds *b, uint32_t sxy, int z) {
	return vtx_outcode_xy(b, (short)sxy, (int32_t)sxy >> 16, z);
}

/* The outcode halfword: the bits, and their "not out" complements in the high byte (the
 * renderer ANDs a face's outcodes for "all out" and ORs ... the complements for "not all in"). */
static __inline__ uint32_t vtx_outcode(uint32_t oc) {
	return oc | (oc ^ 0xff) << 8;
}

/* Fast-format screen word: SXY2 with flags in the bits the 11-bit screen coordinates leave free.
 * Bits 13/29 are x's and y's own sign bits. Bits 14/30: the halves of SXY2 - (xR | yB << 16) were
 * not negative, i.e. x >= xR and y >= yB. Bits 15/31 come from z - Yon: bit 15 its low half not
 * negative (z >= Yon), bit 31 the word negative (z < Yon). The callers AND the words and test
 * bits 13, 14, 29, 30 for "all off one side". */
static __inline__ uint32_t vtx_fast_word(uint32_t sxy, int z, uint32_t xRyB, int yon) {
	uint32_t d = sxy - xRyB, dz = (uint32_t)z - (uint32_t)yon;

	sxy = (sxy & ~0x40004000u) | (~d & 0x40004000u);
	return (sxy & ~0x80008000u) | (dz & 0x80000000u) | (~dz & 0x8000u);
}

/* A super vertex in the 12.4 fixed point of non-dewibbled models, as the GTE takes it: the three
 * coordinates >> 4, the flags left in the high half of the second word. */
static __inline__ void vtx_load_super_scaled(const uint32_t *v) {
	int x = (short)v[0], y = (int32_t)v[0] >> 16, z = (short)v[1];

	gte_set(GTE_VXY0, vtx_pack(x >> 4, y >> 4));
	gte_set(GTE_VZ0, (v[1] & 0xffff0000u) | ((uint32_t)(z >> 4) & 0xffff));
}

/* SuperVertices (dewibbled: the 12.4 input goes to the GTE as it is, and the results are >> 4).
 * Item-format results: the rotated position too, and the outcodes. Returns the AND of the outcode
 * halfwords (0xff for no vertices). */
static __inline__ int vtx_transform_super(const uint32_t *v, int n, int dewibbled) {
	struct ClipBounds b;
	struct SProjectedVertex *pv, *pool;
	struct SRotatedVertex *rv;
	int rot_delta; /* RotatedVertices - ProjectedVertices, in bytes: the pool has rotated twins */
	uint32_t all = 0xffff;

	if (n == 0)
		return 0xff;

	vtx_clip_bounds(&b);

	pv = ProjectedVertices;
	rv = RotatedVertices;
	rot_delta = (char *)rv - (char *)pv;
	pool = FreeStitchVertex;

	for (; n != 0; n--, v += 2, pv++, rv++) {
		uint32_t flags = v[1] >> 16;

		/* stitch vertices go through the GTE too (it is all the same to the hardware) */
		if (dewibbled) {
			gte_set(GTE_VXY0, v[0]);
			gte_set(GTE_VZ0, v[1]);
		} else {
			vtx_load_super_scaled(v);
		}

		gte_rtps();

		if (flags & VF_STITCH) {
			struct SProjectedVertex *src =
			    (struct SProjectedVertex *)((char *)ProjectedVertices + STITCH_TOP - v[0]);

			*pv = *src;
			rv->xy = ((struct SRotatedVertex *)((char *)src + rot_delta))->xy;
			all &= src->Outcode;
		} else {
			int x = gte_get(GTE_IR1), y = gte_get(GTE_IR2), z = gte_get(GTE_IR3);
			uint32_t oc;

			if (dewibbled) {
				x >>= 4;
				y >>= 4;
				z >>= 4;
			}

			pv->XY = PC_GteGetData(GTE_SXY2);
			oc = vtx_outcode(vtx_outcode_bits(&b, pv->XY, z));
			all &= oc;
			pv->z = (short)z;
			pv->Outcode = (ushort)oc;
			rv->xy = vtx_pack(x, y);

			if (flags & VF_EXPORT) {
				*pool = *pv;
				((struct SRotatedVertex *)((char *)pool + rot_delta))->xy = rv->xy;
				pool--;
			}
		}
	}

	FreeStitchVertex = pool;
	SCR_PROJECTED_END = pv;
	return (int)all;
}

/* SuperVerticesFast (+ dewibbled): fast-format results in the scratchpad, no rotated positions or
 * outcodes. Returns the AND of the screen words' x/y flag bits (0x60000000 for no vertices). */
static __inline__ int vtx_transform_super_fast(const uint32_t *v, int n, int dewibbled) {
	struct SViewport *vp = pCurrentViewport;
	uint32_t xRyB = vtx_pack(vp->xR, vp->yB);
	int yon = (short)vp->Yon;
	struct SFastVertex *out = FAST_CACHE, *pool;
	uint32_t all = 0x60006000;

	if (n == 0)
		return 0x60000000; /* retail's early return comes before the constant's low half */

	pool = (struct SFastVertex *)FreeStitchVertex;

	for (; n != 0; n--, v += 2, out++) {
		uint32_t flags = v[1] >> 16, xy;
		int z;

		if (dewibbled) {
			gte_set(GTE_VXY0, v[0]);
			gte_set(GTE_VZ0, v[1]);
		} else {
			vtx_load_super_scaled(v);
		}

		gte_rtps();

		if (flags & VF_STITCH) {
			struct SFastVertex *src = (struct SFastVertex *)(FAST_STITCH_TOP - v[0]);

			xy = src->XY;
			z = src->z;
		} else {
			z = gte_get(GTE_IR3);

			if (dewibbled)
				z >>= 4;

			xy = vtx_fast_word(PC_GteGetData(GTE_SXY2), z, xRyB, yon);

			if (flags & VF_EXPORT) {
				pool->XY = xy;
				pool->z = (short)z;
				pool--;
			}
		}

		out->XY = xy;
		out->z = (short)z;
		all &= xy;
	}

	FreeStitchVertex = (struct SProjectedVertex *)pool;
	return (int)all;
}

#endif

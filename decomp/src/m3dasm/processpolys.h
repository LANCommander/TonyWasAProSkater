/* Shared pieces of the two face renderers, M3dAsm_ProcessPolys (processpolys.c) and
 * M3dAsm_ProcessPolysFast (processpolysfast.c): C interpretation of hand-written assembly (M3DASM).
 *
 * Both walk a model's faces, build GPU packets for the visible ones in the packet buffer (pPoly)
 * and link them into the current ordering table. A face record (words):
 *   [0] flags (low half) | record size in bytes (high half)
 *   [1] four vertex indices (bytes; a triangle's fourth is ignored where it matters)
 *   [2] the colour (code byte free), or four colour-table indices (gouraud faces)
 *   [3] low half: byte offset of the vertex whose lighting a flat lit face takes
 *   [4] uv0 | clut << 16   [5] uv1 | tpage << 16   [6] uv2 | uv3 << 16   (textured faces)
 *   [7] a texture-window command (flag 0x20)
 * Face flags: 1 textured, 2 reflection-mapped, 4 lit by the vertices' lighting words, 8 lit by
 * the item's tint, 0x10 triangle, 0x20 own texture window, 0x40 semi-transparent, 0x80/0x100
 * blend mode, 0x40|0x80 at least one must be set to draw, 0x800 gouraud, 0x1000 subdivide when
 * big (and draw when crossing the near plane), 0x2000/0x4000 push the sort depth back.
 *
 * Packets are addressed as the PS1 GPU sees them: 24-bit addresses in the tags and ordering-table
 * links, with pPoly's top byte dropped (the port keeps pPoly itself in KSEG0: it is stored back
 * with bit 31 set, and PolyBufferEnd is compared without its top byte). */
#ifndef M3DASM_PROCESSPOLYS_H
#define M3DASM_PROCESSPOLYS_H

#include "vertex.h"

/* scratchpad words the renderer shares with M3d_RenderSetup and the callers */
#define SCR32(o)  (*(volatile uint32_t *)(0x1f800000u + (o)))
#define SCR16(o)  (*(volatile uint16_t *)(0x1f800000u + (o)))
#define SCR16S(o) (*(volatile int16_t *)(0x1f800000u + (o)))

#define SCR_DPQMIN      0x284 /* fog starts at this view depth */
#define SCR_DPQSHIFT    0x294 /* fog = (z - DpqMin) << DpqShift (>> when negative) */
#define SCR_CLIPCOUNT   0x2a4 /* faces WaterClip put in ClippedFaceBuffer (halfword) */
#define SCR_PUSHBACK1   0x2b4 /* face flag 0x2000 adds this to the sort depth */
#define SCR_PUSHBACK2   0x2c4 /* face flag 0x4000 adds this */
#define SCR_MIRROR      0x2d4 /* mirrored model: front faces wind the other way (halfword) */
#define SCR_FOG         0x2e4 /* the current face's fog (halfword) */
#define SCR_FADECOLOUR  0x354 /* >= 0: fade-to-colour mode; bit 25: by subtraction */
#define SCR_REFMAPCLUT  0x364 /* reflection mapping: clut and tpage for the uv words */
#define SCR_REFMAPTPAGE 0x374
#define SCR_FADEXY      0x384 /* the current face's four screen words (fade-to-colour) */
#define SCR_CLIPWRITE   0x3c4 /* next free record in ClippedFaceBuffer */
#define SCR_CLIPVERTEX  0x3d4 /* next free ProjectedVertices slot (WaterClip's new vertices) */
#define SCR_TINT        0x3e4 /* the item's tint, as a lighting word */
#define SCR_FACEFLAGS   0x3f4 /* GlobalFaceFlags: a face's flags are (flags & hi) | lo */

/* face record */
enum {
	FACE_FLAGS = 0,
	FACE_VERTS = 1,
	FACE_COLOUR = 2,
	FACE_LIGHT = 3,
	FACE_UV0 = 4,
	FACE_UV1 = 5,
	FACE_UV23 = 6,
	FACE_TEXWINDOW = 7
};

enum {
	FF_TEXTURED = 0x1,
	FF_REFLECT = 0x2,
	FF_VERTEXLIT = 0x4,
	FF_TINTED = 0x8,
	FF_TRIANGLE = 0x10,
	FF_TEXWINDOW = 0x20,
	FF_SEMITRANS = 0x40,
	FF_DRAW = 0xc0,
	FF_ABR = 0x180,
	FF_GOURAUD = 0x800,
	FF_SUBDIVIDE = 0x1000,
	FF_PUSHBACK1 = 0x2000,
	FF_PUSHBACK2 = 0x4000
};

/* GPU primitive codes (the top byte of a packet's first command word) */
enum {
	CODE_F3 = 0x20,
	CODE_FT3 = 0x24,
	CODE_F4 = 0x28,
	CODE_FT4 = 0x2c,
	CODE_G3 = 0x30,
	CODE_GT3 = 0x34,
	CODE_G4 = 0x38,
	CODE_GT4 = 0x3c
};

#define CODE_SEMITRANS 0x02000000u
#define CODE_QUAD_BIT  0x08000000u /* F3 <-> F4, G3 <-> G4 ... */

/* GTE data registers and commands beyond vertex.h's */
enum {
	GTE_SXY0 = 12,
	GTE_SXY1 = 13,
	GTE_SXYP = 15,
	GTE_RGB0 = 20,
	GTE_RGB1 = 21,
	GTE_RGB2 = 22,
	GTE_MAC0 = 24,
	GTE_DQA = 27,
	GTE_DQB = 28
};

#define GTE_CMD_NCLIP 0x1400006u
#define GTE_CMD_DPCS  0x0780010u
#define GTE_CMD_DPCL  0x0680029u
#define GTE_CMD_DPCT  0x0f8002au

extern uint32_t *pPoly; /* next free word of the packet buffer */
extern uint8_t *PolyBufferEnd;
extern uint32_t *pColourTable; /* gouraud faces' colours */
extern uint32_t *CurrentOrderingTable;

/* A packet-buffer word by its 24-bit address. */
#define PKT(a) ((uint32_t *)(0x80000000u | (uint32_t)(a)))

/* The walk's state while one face is drawn. */
typedef struct PolyState {
	uint32_t pkt;   /* next free packet word (24-bit address) */
	uint32_t limit; /* no new face once pkt reaches this: PolyBufferEnd - 100 */
	uint32_t *ot;   /* CurrentOrderingTable */
	const uint32_t *face;
	uint32_t flags; /* the face's flags (after GlobalFaceFlags; fade mode adds 0x1c0) */
	int zmax;       /* the face's greatest vertex depth */
} PolyState;

static __inline__ void pp_begin(PolyState *ps) {
	ps->pkt = (uint32_t)pPoly & 0xffffff;
	ps->limit = ((uint32_t)PolyBufferEnd & 0xffffff) - 100;
	ps->ot = CurrentOrderingTable;
}

static __inline__ void pp_end(PolyState *ps) {
	pPoly = (uint32_t *)(ps->pkt | 0x80000000u);
}

/* The fog of a face whose farthest vertex is at depth zmax: 0 nearer than DpqMin, else the
 * distance beyond it scaled by DpqShift. In fade-to-colour mode a fogged face becomes
 * semi-transparent with blend mode 1 (added onto the fade polygon drawn under it). Loads IR0
 * (the depth-cue weight of the colour commands) and keeps the value in SCR_FOG. */
static __inline__ int pp_fog(PolyState *ps) {
	int d = ps->zmax - SCR16S(SCR_DPQMIN), shift = SCR16S(SCR_DPQSHIFT), fog = 0;

	if (d > 0) {
		if ((int32_t)SCR32(SCR_FADECOLOUR) >= 0)
			ps->flags = (ps->flags | 0x1c0) ^ 0x100;

		fog = shift < 0 ? d >> (-shift & 31) : (int)((uint32_t)d << (shift & 31));
	}

	gte_set(GTE_IR0, fog);

	SCR16(SCR_FOG) = (uint16_t)fog;
	return d;
}

/* The packet's first command word: the colour with the primitive code (and the
 * semi-transparency bit). */
static __inline__ void pp_code(PolyState *ps, uint32_t rgb, int code) {
	PKT(ps->pkt)[1] = rgb | (uint32_t)code << 24 | (ps->flags & FF_SEMITRANS) << 19;
}

/* The sort depth's ordering-table entry: the face's farthest depth, pushed back by the
 * scratchpad offsets for flags 0x2000/0x4000 (not below 0), a word per 4 units, the last entry
 * for anything beyond. */
static __inline__ uint32_t *pp_ot_entry(PolyState *ps) {
	int z = ps->zmax;
	uint32_t off;

	if (ps->flags & (FF_PUSHBACK1 | FF_PUSHBACK2)) {
		z += (ps->flags & FF_PUSHBACK1) ? SCR16S(SCR_PUSHBACK1) : SCR16S(SCR_PUSHBACK2);

		if (z < 0)
			z = 0;
	}

	off = (uint32_t)z < 0x4000 ? (uint32_t)z & 0xfffc : 0x3ffc;
	return (uint32_t *)((char *)ps->ot + off);
}

/* Fade-to-colour mode: under a fogged face, a flat polygon over the same screen words in the
 * fade colour, depth-cued toward the far colour by the fog's complement (or by the fog itself,
 * drawn subtractively, for a fade colour with bit 25 set), linked in front of the face in the
 * same ordering-table entry. The subtractive version carries a draw-mode word and a NOP before
 * the polygon, and leaves two unused words after it. */
static __inline__ void pp_add_fade_poly(PolyState *ps, uint32_t *ote, uint32_t colour, int fog,
                                        const uint32_t xy[4]) {
	uint32_t start = ps->pkt, old, *p;
	int len = 0;

	gte_set(GTE_RGBC, colour);

	if (colour >> 24 & 2) {
		PKT(start)[1] = 0xe1000240; /* draw mode: blend mode 2 (subtract), dither */
		PKT(start)[2] = 0;
		ps->pkt += 8;
		len = 2;
	} else {
		fog = 4096 - fog;
	}

	gte_set(GTE_IR0, fog);

	old = *ote;
	*ote = start;
	PC_GteCommand(GTE_CMD_DPCS);

	p = PKT(ps->pkt);
	p[2] = xy[0];
	p[3] = xy[1];
	p[4] = xy[2];
	p[5] = xy[3]; /* also for a triangle (the word after its packet) */

	if (ps->flags & FF_TRIANGLE) {
		p[1] = PC_GteGetData(GTE_RGB2) | (uint32_t)CODE_F3 << 24;
		len += 4;
	} else {
		p[1] = PC_GteGetData(GTE_RGB2) | (uint32_t)CODE_F4 << 24;
		len += 5;
	}

	PKT(start)[0] = old | (uint32_t)len << 24;
	ps->pkt += len * 4 + 4;
}

/* Links a finished face into the ordering table: the packets from `start` to `last` (one, or
 * the chain a subdivision made, each linked to the next), `last` being len words long.
 * A face with its own texture window gets three words more (a NOP, a texture-window reset and a
 * NOP) and a packet in front of it that sets its window. An untextured semi-transparent face
 * gets a draw-mode packet with its blend mode in front of it instead (linked to `last`).
 * Then, in fade-to-colour mode, the fade polygon (xy: the face's screen words). */
static __inline__ void pp_link(PolyState *ps, uint32_t start, uint32_t last, uint32_t len,
                               const uint32_t xy[4]) {
	uint32_t *ote = pp_ot_entry(ps), old = *ote, size, fade;
	int fog;

	if (ps->flags & FF_TEXWINDOW) {
		uint32_t win = ps->face[FACE_TEXWINDOW], next;

		len += 3;
		PKT(last)[0] = old | len << 24;
		next = last + len * 4 + 4;
		*ote = next;
		PKT(next)[-3] = 0;
		PKT(next)[-2] = 0xe2000000; /* texture window: none */
		PKT(next)[-1] = 0;
		PKT(next)[0] = start | 0x02000000;
		PKT(next)[1] = win;
		PKT(next)[2] = 0;
		ps->pkt = next + 12;
	} else {
		*ote = start;
		PKT(last)[0] = old | len << 24;
		size = len * 4 + 4;

		if ((ps->flags & (FF_SEMITRANS | FF_TEXTURED)) == FF_SEMITRANS) {
			uint32_t dm = last + size;

			PKT(dm)[1] = 0xe1000200 | (ps->flags & FF_ABR) >> 2; /* draw mode: blend, dither */
			PKT(dm)[0] = 0x01000000 | last;
			*ote = dm;
			ps->pkt = dm + 8;
		} else {
			ps->pkt = last + size;
		}
	}

	fog = SCR16S(SCR_FOG);
	fade = SCR32(SCR_FADECOLOUR);

	if (fog != 0 && (int32_t)fade >= 0)
		pp_add_fade_poly(ps, ote, fade, fog, xy);
}

#endif

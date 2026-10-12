/* M3dAsm_ProcessPolys__FPUiP7SVECTORi (0x80086f40) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Draws a model's faces from the ProjectedVertices cache a transform just filled: back-face and
 * outcode culling, fog, the face's colours through the GTE colour commands (flat, gouraud,
 * lit by the vertices' or the item's lighting), GPU packets (F3/FT3/G3/GT3/F4/FT4/G4/GT4) linked
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
 * done; these two hold the caller's s6 and s5, whose values only the retail code itself uses.) */
#include "processpolys.h"

int M3dMaths_SquareRoot0__Fi(int n);

extern SVECTOR WaterNormal; /* the water surface's normal (view space); .pad: its level */

/* WaterClip's output: faces made by clipping, drawn after the model's own (retail keeps this
 * buffer in the code segment, right after the subdivision code). */
#define CLIPPED_FACE_BUFFER 0x80088790u

/* GTE commands beyond processpolys.h's */
#define GTE_CMD_GPF12 0x198003du
#define GTE_CMD_GPL12 0x1a8003eu

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

/* A lighting word (r, g, b in 11, 11, 10 bits) as IR1-3, the light of DPCL. */
static void set_light(uint32_t l)
{
	gte_set(GTE_IR1, l << 5);
	gte_set(GTE_IR2, l >> 6);
	gte_set(GTE_IR3, l >> 17);
}

/* ---------------------------------------------------------------------------------------------
 * Near-plane faces */

/* M3dMaths_OriginAgainstPlaneTest (0x8008b894): which side of the plane through three view-space
 * points the eye is on. On entry RT11/RT22/RT33 hold the first point, IR1-3 the second and V0
 * the third: OP gives the plane normal (first x second, unscaled). That goes into the rotation
 * matrix twice, as rounded high halves (row 2) and low halves (row 3), and MVMVA dots both with
 * the third point: returns about (normal . third) >> 16, negative for a face turned away. */
static int M3dMaths_OriginAgainstPlaneTest(void)
{
	uint32_t nx, ny, nz;

	gte_op0();
	nx = PC_GteGetData(GTE_MAC1);
	ny = PC_GteGetData(GTE_MAC2);
	nz = PC_GteGetData(GTE_MAC3);
	gte_setc(GTE_RT + 3, (nx & 0xffff) | ny << 16); /* RT31, RT32 */
	gte_setc(GTE_RT + 4, nz);                       /* RT33 */
	gte_setc(GTE_RT + 1, nx + 0x8000);              /* RT13 (unused), RT21 */
	gte_setc(GTE_RT + 2, (ny + 0x8000) >> 16 | ((nz + 0x8000) >> 16) << 16); /* RT22, RT23 */
	gte_mvmva(0, MX_RT, MV_V0, CV_NONE, 0);
	return gte_get(GTE_MAC2) + (gte_get(GTE_MAC3) >> 16);
}

/* A face crossing the near plane has no usable screen winding: it is drawn (subdivided) if the
 * eye is on its front side, by its view-space vertices 0, 1, 2. */
static int near_face_visible(PP *pp)
{
	uint32_t xy;

	xy = RV32(pp, pp->off[0], 0);
	gte_setc(GTE_RT + 0, xy);       /* RT11 = x0 (RT12 = y0, unused) */
	gte_setc(GTE_RT + 2, xy >> 16); /* RT22 = y0 */
	gte_setc(GTE_RT + 4, PV32(pp, pp->off[0], 1)); /* RT33 = z0 */
	xy = RV32(pp, pp->off[1], 0);
	gte_set(GTE_IR1, xy);
	gte_set(GTE_IR2, xy >> 16);
	gte_set(GTE_IR3, PV32(pp, pp->off[1], 1));
	gte_set(GTE_VXY0, RV32(pp, pp->off[2], 0));
	gte_set(GTE_VZ0, PV32(pp, pp->off[2], 1));
	return M3dMaths_OriginAgainstPlaneTest() >= 0;
}

/* ---------------------------------------------------------------------------------------------
 * Subdivision
 *
 * The face is cut into segs x segs pieces. Its screen-space area (twice the NCLIP value, or a
 * huge value for near-plane faces) picks segs: sqrt(area) / 32 rounded up, at most 7. The grid's
 * vertices are worked out in the scratchpad, one 16-byte cell each, rows 128 bytes apart: the
 * GTE's rotation matrix holds the three view-space corners (one per column) and RTPS projects a
 * weighted sum of them (V0 = the weights, 4096 in all); the light and colour matrices hold the
 * corners' uvs and colours the same way, and MVMVA + GPF interpolate those into colour words.
 * A cell: +0 screen word (clamped, with outcode bits), +8 uv (as a colour word), +12 colour.
 * Every word the renderer keeps in the scratchpad is at +4 of some cell, which the grid never
 * touches; subdivision saves its own state in such words as well. */

#define GRID 0x1f800000u
#define CELL(a, o)  (*(uint32_t *)((a) + (o)))
#define CELLH(a, o) (*(uint16_t *)((a) + (o)))

#define OFFSCREEN 0x6000e000u /* screen-word bits a piece is dropped for when all corners have one */

enum { SUB_COL = 1, SUB_UV = 2 }; /* gouraud / textured kinds of packet */

typedef struct {
	int kind;          /* SUB_COL | SUB_UV: the packet's per-vertex words beyond the screen word */
	int tri;
	int segs;          /* pieces along an edge */
	int step;          /* the weight step between grid vertices, 0x20000 / sqrt(area) */
	uint32_t viewport; /* xR | yB << 16: the screen words' outcode reference */
	uint32_t start;    /* the face's packet (the first piece goes there) */
	uint32_t last;     /* size in bytes of the last piece made */
	uint32_t code;     /* flat kinds: the quad command word (code | colour) */
	uint32_t clut, tpage; /* textured kinds: their high halves, for the first two uvs */
	uint32_t and_out;  /* screen words ANDed over the piece being built (and OFFSCREEN) */
} Sub;

/* SubdivSetup (0x80087b3c): saves the walk's state in the scratchpad and picks segs and step. */
static void subdiv_setup(PP *pp, Sub *s, uint32_t area)
{
	int n;

	/* retail's registers, in words the grid leaves alone (+4 of cells 1-11), reloaded by
	 * SubdivisionDone: face, ordering table, a2, faces left, packet, scratchpad base, depth,
	 * flags, (s6, s5: the caller's), colour table */
	SCR32(0x14) = (uint32_t)pp->ps.face;
	SCR32(0x24) = (uint32_t)pp->ps.ot;
	SCR32(0x34) = pp->a2;
	SCR32(0x44) = (uint32_t)pp->left;
	SCR32(0x54) = pp->ps.pkt;
	SCR32(0x64) = 0x1f800000;
	SCR32(0x74) = (uint32_t)pp->ps.zmax;
	SCR32(0x84) = pp->ps.flags;
	SCR32(0xb4) = (uint32_t)pp->colours;
	/* the colour commands below carry this code into the cells: the quad version of the face's */
	gte_set(GTE_RGBC, PKT(pp->ps.pkt)[1] | CODE_QUAD_BIT);
	n = M3dMaths_SquareRoot0__Fi((int)area);
	if (224 - n <= 0)
		n = 224;
	s->step = n ? (int)(0x20000u / (uint32_t)n) : -1; /* (divu by zero gives all ones) */
	s->segs = (int)((uint32_t)(n + 31) >> 5);
	s->viewport = *(uint32_t *)pCurrentViewport;
	s->last = s->viewport; /* (no piece made yet) */
}

/* SetupUVs (0x80087bd8): the light matrix's rows get three corners' u and v (x16). */
static void setup_uvs(uint32_t a, uint32_t b, uint32_t c)
{
	gte_setc(GTE_LLM + 0, (a & 0xff) << 4 | (b & 0xff) << 20);
	gte_setc(GTE_LLM + 1, (c & 0xff) << 4 | (a & 0xff00) << 12);
	gte_setc(GTE_LLM + 2, (b & 0xff00) >> 4 | (c & 0xff00) << 12);
	gte_setc(GTE_LLM + 3, 0);
	gte_setc(GTE_LLM + 4, 0);
}

/* SetupRGBs (0x80087c3c): the colour matrix's rows get three corners' r, g and b (x16). */
static void setup_rgbs(uint32_t a, uint32_t b, uint32_t c)
{
	gte_setc(GTE_LCM + 0, (a & 0xff) << 4 | (b & 0xff) << 20);
	gte_setc(GTE_LCM + 1, (c & 0xff) << 4 | (a & 0xff00) << 12);
	gte_setc(GTE_LCM + 2, (b & 0xff00) >> 4 | (c & 0xff00) << 12);
	gte_setc(GTE_LCM + 3, (a >> 12 & 0xff0) | (b >> 12 & 0xff0) << 16);
	gte_setc(GTE_LCM + 4, c >> 12 & 0xff0);
}

/* SetupXYs (0x80087cc0): the rotation matrix's columns get three corners' view-space positions. */
static void setup_xys(PP *pp, uint32_t a, uint32_t b, uint32_t c)
{
	uint32_t xya = RV32(pp, a, 0), xyb = RV32(pp, b, 0), xyc = RV32(pp, c, 0);

	gte_setc(GTE_RT + 0, (xya & 0xffff) | xyb << 16);
	gte_setc(GTE_RT + 1, (xyc & 0xffff) | (xya & 0xffff0000));
	gte_setc(GTE_RT + 2, xyb >> 16 | (xyc >> 16) << 16);
	gte_setc(GTE_RT + 3, (PV32(pp, a, 1) & 0xffff) | PV32(pp, b, 1) << 16);
	gte_setc(GTE_RT + 4, PV32(pp, c, 1) & 0xffff);
}

/* The grid builder (0x80087d24): a triangle of grid vertices between the three corners in the
 * matrices (weights x, y, z of V0 for the columns a, b, c), starting at cell `out`. Row r
 * (rows .. 0 counting down) has segs - rows + 1 vertices, dcol bytes apart, from corner c's
 * weight w toward corner a; rows are drow bytes apart, w falling by step each row (0 on the
 * last). The last row's last vertex is exactly corner b. */
static void subdiv_grid(const Sub *s, uint32_t out, int w, int dcol, int drow, int rows)
{
	int rgb = s->kind & SUB_COL;

	do {
		uint32_t row = out, next;
		int a, b, cols;

		if (rows == 0)
			w = 0;
		a = 4096 - w;
		b = 0;
		gte_set(GTE_VXY0, a);
		gte_set(GTE_VZ0, w);
		out -= dcol;
		cols = s->segs - rows;
		do {
			uint32_t xy, flag, d;
			int sx, sy;

			gte_rtps();
			out += dcol;
			a -= s->step;
			b += s->step;
			next = (uint32_t)b << 16 | (uint32_t)a; /* (a negative a spills into the y weight) */
			if (((cols - 1) | rows) == 0)
				next = 0x10000000; /* the far corner exactly */

			/* the screen word, clamped to what the GPU takes */
			xy = PC_GteGetData(GTE_SXY2);
			sy = (int32_t)xy >> 16;
			sx = (int16_t)xy;
			if (sy > 510)
				sy = 510;
			if (sx > 1022)
				sx = 1022;
			if (sx < -510)
				sx = -510;
			xy = ((uint32_t)sx & 0xffff) | (uint32_t)sy << 16;
			flag = gte_getc(GTE_FLAG);
			gte_mvmva(1, MX_LLM, MV_V0, CV_NONE, 0); /* uv */
			/* outcode bits as the fast transforms make them: 14/30 x >= xR / y >= yB, 15 the
			 * projection overflowed */
			d = xy - s->viewport;
			xy = ((xy & 0xbfffbfff) | ~(d | 0xbfffbfff)) & 0xffff7fff;
			PC_GteCommand(GTE_CMD_GPF12);
			xy |= flag >> 2 & 0x8000;
			CELL(out, 0) = xy;
			CELL(out, 8) = PC_GteGetData(GTE_RGB2);
			if (rgb) {
				gte_mvmva(1, MX_LCM, MV_V0, CV_NONE, 0); /* colour */
				PC_GteCommand(GTE_CMD_GPF12);
				CELL(out, 12) = PC_GteGetData(GTE_RGB2);
			}
			gte_set(GTE_VXY0, next);
		} while (cols-- != 0);
		w -= s->step;
		out = row + drow;
	} while (rows-- != 0);
}

/* The grid of a triangle (corners 0, 1, 2), or a quad's first half; with the corners' colour and
 * uv words from the face's packet (word indices col/uv[corner], when the kind has them). */
static void subdiv_half1(PP *pp, Sub *s, const int *col, const int *uv)
{
	uint32_t *p = PKT(s->start);

	if (s->kind & SUB_UV)
		setup_uvs(p[uv[0]], p[uv[2]], p[uv[1]]);
	if (s->kind & SUB_COL)
		setup_rgbs(p[col[0]], p[col[2]], p[col[1]]);
	setup_xys(pp, pp->off[0], pp->off[2], pp->off[1]);
	subdiv_grid(s, GRID, 4096, 16, 128, s->segs);
}

/* A quad's second half (corners 3, 2, 1): the rest of the square grid, transposed. */
static void subdiv_half2(PP *pp, Sub *s, const int *col, const int *uv)
{
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
static uint32_t expand_edge(uint32_t cur, uint32_t a, uint32_t b, int stride, int segs)
{
	uint32_t d = b - a;
	uint32_t dy = ((d & 0x2000) - 4096) << 4;            /* +-1 in y */
	uint32_t dx = (((int32_t)d >> 28) & 2) - 1;          /* +-1 in x */
	uint32_t adj = dy - dx;
	int n = segs - 1;

	cur += (uint32_t)stride;
	do {
		CELL(cur, 0) += adj;
		n--;
		cur += (uint32_t)stride;
	} while (n > 0);
	return cur;
}

/* ExpandTri (0x80087f30), ExpandQuad (0x80087f74): the edges of the grid, given the corners'
 * screen words; one cursor walks all of them in turn. */
static void expand_tri(uint32_t xy0, uint32_t xy1, uint32_t xy2, int segs)
{
	uint32_t cur = GRID;

	cur = expand_edge(cur, xy1, xy2, 144, segs);
	cur = expand_edge(cur, xy2, xy0, -16, segs);
	expand_edge(cur, xy0, xy1, -128, segs);
}

static void expand_quad(uint32_t xy0, uint32_t xy1, uint32_t xy2, uint32_t xy3, int segs)
{
	uint32_t cur = GRID;

	cur = expand_edge(cur, xy1, xy3, 16, segs);
	cur = expand_edge(cur, xy3, xy2, 128, segs);
	cur = expand_edge(cur, xy2, xy0, -16, segs);
	expand_edge(cur, xy0, xy1, -128, segs);
}

/* A grid vertex as the packets take it. */
typedef struct {
	uint32_t xy, col, uv;
} GridVertex;

static GridVertex grid_vertex(uint32_t cell)
{
	GridVertex v;

	v.xy = CELL(cell, 0);
	v.col = CELL(cell, 12);
	v.uv = CELLH(cell, 8);
	return v;
}

/* Writes a vertex's words at packet word i: [colour] screen word [uv | hi << 16]. */
static int put_vertex(uint32_t *p, int i, const Sub *s, const GridVertex *v, uint32_t hi)
{
	if (s->kind & SUB_COL)
		p[i++] = v->col;
	p[i++] = v->xy;
	if (s->kind & SUB_UV)
		p[i++] = v->uv | hi;
	return i;
}

/* Ends a piece: unless all its corners share an off-screen bit, it becomes a packet of len
 * words linked to the next one. */
static void emit_piece(PP *pp, Sub *s, int len)
{
	uint32_t at = pp->ps.pkt;

	if (s->and_out != 0)
		return;
	pp->ps.pkt = at + (uint32_t)len * 4 + 4;
	PKT(at)[0] = ((uint32_t)len << 24) + pp->ps.pkt;
	s->last = (uint32_t)len * 4 + 4;
}

/* The words of a piece's first two corners (bottom-left b, top-left t). A flat piece starts with
 * its command word, a gouraud one with b's colour (the code in it); xor_code turns the quad code
 * into the triangle's. Returns the next word index. */
static int put_left(uint32_t *p, Sub *s, GridVertex *b, const GridVertex *t, uint32_t xor_code)
{
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
static int subdiv_quads(PP *pp, Sub *s, uint32_t *cur, GridVertex *b, GridVertex *t, int n)
{
	int len = (s->kind & SUB_COL ? 0 : 1) + 4 * (1 + (s->kind & SUB_COL ? 1 : 0) +
	                                              (s->kind & SUB_UV ? 1 : 0));

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
static void subdiv_row_tri(PP *pp, Sub *s, uint32_t cur, GridVertex *b, const GridVertex *t)
{
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
		len = (s->kind & SUB_COL ? 0 : 1) + 3 * (1 + (s->kind & SUB_COL ? 1 : 0) +
		                                         (s->kind & SUB_UV ? 1 : 0));
	}
	emit_piece(pp, s, len);
}

/* The pieces, row by row (0x80088024 ... SubdivideGT4). Returns 0 when the buffer fills up. */
static int subdiv_rows(PP *pp, Sub *s)
{
	uint32_t row = GRID;
	int r;

	for (r = 0; r < s->segs; r++) {
		uint32_t cur = row;
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
static uint32_t subdivide(PP *pp, int kind, int tri, uint32_t area, const int *xyw,
                          const int *colw, const int *uvw)
{
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
		s.code = PC_GteGetData(GTE_RGBC);
	subdiv_rows(pp, &s);

	/* SubdivisionDone (0x80088744) */
	gte_set(GTE_RGBC, 0);
	if (pp->ps.pkt == s.start)
		return 0;
	return s.last;
}

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
	uint32_t out;      /* the next record in ClippedFaceBuffer */
	uint32_t vtx;      /* the next free ProjectedVertices slot */
	uint32_t size;     /* the face record's size */
} Clip;

/* MIPS div: no trap on division by zero (the quotient is -1 or 1) or overflow. */
static int32_t mips_div(int32_t n, int32_t d)
{
	if (d == 0)
		return n >= 0 ? -1 : 1;
	if (d == -1 && n == INT32_MIN)
		return n;
	return n / d;
}

/* ClipEdge (0x800894b0): the point where the edge from vertex a to b meets the water, as a new
 * projected vertex (and its rotated twin), and the uv there. The GTE does it all: the edge ends'
 * heights against WaterNormal (MVMVA), the position by GPL from a toward b by the ratio, RTPS,
 * then GPL again on the uvs (left as RGB2's u, v bytes). Returns uvb << 16 | uva. */
static uint32_t clip_edge(Clip *c, uint32_t a, uint32_t b, uint32_t uva, uint32_t uvb)
{
	PP *pp = c->pp;
	uint32_t uv = uvb << 16 | uva, xya = RV32(pp, a, 0), xyb = RV32(pp, b, 0), xy, z;
	int za = (int16_t)PV32(pp, a, 1), zb = (int16_t)PV32(pp, b, 1), da, t;
	int xa = (int16_t)xya, ya = (int32_t)xya >> 16;

	gte_setc(GTE_RT + 0, xya);        /* row 1: a */
	gte_setc(GTE_RT + 1, (uint32_t)za);
	gte_setc(GTE_RT + 3, xyb);        /* row 3: b */
	gte_setc(GTE_RT + 4, (uint32_t)zb);
	gte_set(GTE_VXY0, vtx_pack(WaterNormal.vx, WaterNormal.vy));
	gte_set(GTE_VZ0, vtx_pack(WaterNormal.vz, WaterNormal.pad));
	gte_mvmva(1, MX_RT, MV_V0, CV_NONE, 0);
	da = gte_get(GTE_MAC1);
	t = mips_div((int32_t)((uint32_t)(-WaterNormal.pad - da) << 12), gte_get(GTE_MAC3) - da);

	gte_set(GTE_MAC1, xa);
	gte_set(GTE_MAC2, ya);
	gte_set(GTE_MAC3, za);
	gte_set(GTE_IR1, (int16_t)xyb - xa);
	gte_set(GTE_IR2, ((int32_t)xyb >> 16) - ya);
	gte_set(GTE_IR3, zb - za);
	gte_setc(GTE_RT + 0, 4096);       /* identity rotation, no translation */
	gte_setc(GTE_RT + 1, 0);
	gte_setc(GTE_RT + 2, 4096);
	gte_setc(GTE_RT + 3, 0);
	gte_setc(GTE_RT + 4, 4096);
	gte_setc(GTE_TRX, 0);
	gte_setc(GTE_TRY, 0);
	gte_setc(GTE_TRZ, 0);
	gte_set(GTE_IR0, t);
	PC_GteCommand(GTE_CMD_GPL12);
	xy = (PC_GteGetData(GTE_MAC1) & 0xffff) | PC_GteGetData(GTE_MAC2) << 16;
	z = PC_GteGetData(GTE_MAC3);
	gte_set(GTE_VXY0, xy);
	gte_set(GTE_VZ0, z);
	gte_rtps();
	*(uint32_t *)(c->vtx + 3200) = xy;
	*(uint32_t *)(c->vtx + 4) = (z & 0xffff) | 0xff000000; /* outcode: inside everything */
	*(uint32_t *)c->vtx = PC_GteGetData(GTE_SXY2);
	c->vtx += 8;

	gte_set(GTE_MAC1, uv << 4 & 0xff0);
	gte_set(GTE_MAC2, uv >> 4 & 0xff0);
	gte_set(GTE_IR1, (uv >> 12 & 0xff0) - (uv << 4 & 0xff0));
	gte_set(GTE_IR2, (uv >> 20 & 0xff0) - (uv >> 4 & 0xff0));
	gte_set(GTE_IR0, t);
	PC_GteCommand(GTE_CMD_GPL12);
	return uv;
}

/* The face's uv halfwords (corners 0-3) and the edges ClipEdge01 ... ClipEdge32. */
#define FACE_UV(c, i) (((const uint16_t *)(c)->face)[(i) == 3 ? 13 : 8 + 2 * (i)])
static uint32_t clip(Clip *c, int a, int b)
{
	return clip_edge(c, c->pp->off[a], c->pp->off[b], FACE_UV(c, a), FACE_UV(c, b));
}

/* The record's words other than vertices and uvs: flags (triangle or not), colour, lighting
 * offset, clut, tpage, texture window. */
static void copy_record(Clip *c, uint32_t tri)
{
	uint32_t *o = (uint32_t *)c->out;

	o[0] = tri ? c->face[0] | FF_TRIANGLE : c->face[0] & ~FF_TRIANGLE;
	o[2] = c->face[2];
	o[3] = c->face[3];
	((uint16_t *)o)[9] = ((const uint16_t *)c->face)[9];
	((uint16_t *)o)[11] = ((const uint16_t *)c->face)[11];
	o[7] = c->face[7];
}

static void set_verts(Clip *c, uint32_t a, uint32_t b, uint32_t cc, uint32_t d)
{
	uint8_t *o = (uint8_t *)c->out;

	o[4] = (uint8_t)a;
	o[5] = (uint8_t)b;
	o[6] = (uint8_t)cc;
	o[7] = (uint8_t)d;
}

#define OUT_UV(c, i) (((uint16_t *)(c)->out)[8 + (i)])

/* the index of the n-th new vertex from the end (1: the last made) */
static uint32_t new_vertex(Clip *c, int n)
{
	return (c->vtx - (uint32_t)c->pp->pv - 8 * (uint32_t)n) >> 3;
}

/* CopyTri (0x800896a0): a record for the kept triangle of a quad with one corner dropped. */
static void copy_tri(Clip *c)
{
	copy_record(c, 1);
	SCR16(SCR_CLIPCOUNT)++;
}

/* MakeClippedTri (0x80089628): the kept corner a and the two new vertices. */
static void make_clipped_tri(Clip *c, int a)
{
	uint32_t v = new_vertex(c, 2);

	set_verts(c, c->pp->off[a] >> 3, v, v + 1, v + 1);
	OUT_UV(c, 2) = (uint16_t)PC_GteGetData(GTE_RGB0); /* the two new uvs */
	OUT_UV(c, 4) = (uint16_t)PC_GteGetData(GTE_RGB2);
	copy_record(c, 1);
}

/* MakeClippedQuad (0x800896ec): the kept corners a, b and the two new vertices. */
static void make_clipped_quad(Clip *c, int a, int b)
{
	uint32_t v = new_vertex(c, 2);

	set_verts(c, c->pp->off[a] >> 3, c->pp->off[b] >> 3, v, v + 1);
	OUT_UV(c, 4) = (uint16_t)PC_GteGetData(GTE_RGB0);
	OUT_UV(c, 5) = (uint16_t)PC_GteGetData(GTE_RGB2);
	copy_record(c, 0);
}

/* A quad with corner `drop` under water and its opposite corner kept whole: the triangle of the
 * three corners other than `drop` (with `opp` moved to the dry side's outcode), then the quad
 * between that triangle's edge and the two new vertices. tri[]: the triangle's corners. */
static void clip_corner(Clip *c, int opp, const int tri[3])
{
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
static void water_clip(PP *pp, const uint32_t *face, const uint32_t *w1)
{
	Clip c;
	int o0 = (w1[0] & 0x800000) != 0, o1 = (w1[1] & 0x800000) != 0;
	int o2 = (w1[2] & 0x800000) != 0, o3 = (w1[3] & 0x800000) != 0;
	uint32_t f = pp->ps.flags;

	c.pp = pp;
	c.face = face;
	c.vtx = SCR32(SCR_CLIPVERTEX);
	c.out = SCR32(SCR_CLIPWRITE);
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
	SCR32(SCR_CLIPVERTEX) = c.vtx;
	SCR32(SCR_CLIPWRITE) = c.out;
	pp->a2 = c.out;
}

/* ---------------------------------------------------------------------------------------------
 * One face */

/* Packet word indices of the corners' screen words / colours / uvs, per packet type. */
static const int XY_F3[3] = { 2, 3, 4 }, XY_FT3[3] = { 2, 4, 6 }, XY_G3[3] = { 2, 4, 6 };
static const int XY_GT3[3] = { 2, 5, 8 }, XY_F4[4] = { 2, 3, 4, 5 }, XY_FT4[4] = { 2, 4, 6, 8 };
static const int XY_G4[4] = { 2, 4, 6, 8 }, XY_GT4[4] = { 2, 5, 8, 11 };
static const int COL_G[4] = { 1, 3, 5, 7 }, COL_GT[4] = { 1, 4, 7, 10 };
static const int UV_FT[4] = { 3, 5, 7, 9 }, UV_GT[4] = { 3, 6, 9, 12 };

/* Draws one face; returns 0 when the packet buffer is full (the walk stops). */
static int draw_face(PP *pp, const uint32_t *face)
{
	PolyState *ps = &pp->ps;
	uint32_t gff = SCR32(SCR_FACEFLAGS);
	uint32_t f = (face[FACE_FLAGS] & (uint32_t)((int32_t)gff >> 16)) | (gff & 0xffff);
	uint32_t vi = face[FACE_VERTS], w1[4], oc, z, c, col0 = 0, len, start, last, xy[4];
	int i, area, tri = (f & FF_TRIANGLE) != 0, nv = tri ? 3 : 4, gouraud, kind;
	const int *xyw, *colw = COL_G, *uvw = UV_FT;
	uint32_t *p;

	if (!(f & FF_DRAW))
		return 1;
	for (i = 0; i < 4; i++) {
		pp->off[i] = (vi >> 8 * i & 0xff) * 8;
		w1[i] = PV32(pp, pp->off[i], 1);
	}

	/* the screen winding of corners 0, 1, 2 and of 1, 2, 3 */
	gte_set(GTE_SXY0, PV32(pp, pp->off[0], 0));
	gte_set(GTE_SXY1, PV32(pp, pp->off[1], 0));
	gte_set(GTE_SXY2, PV32(pp, pp->off[2], 0));
	PC_GteCommand(GTE_CMD_NCLIP);
	gte_set(GTE_SXYP, PV32(pp, pp->off[3], 0)); /* SXY0-2 now hold corners 1, 2, 3 */
	PKT(ps->pkt)[2] = PV32(pp, pp->off[0], 0);
	area = gte_get(GTE_MAC0);
	PC_GteCommand(GTE_CMD_NCLIP);

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
		int n1 = area, n2 = gte_get(GTE_MAC0);

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
	gte_set(GTE_MAC0, area); /* SubdivTest reads it back */
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
		SCR32(0x384) = PKT(ps->pkt)[2];
		SCR32(0x394) = PC_GteGetData(GTE_SXY0);
		SCR32(0x3a4) = PC_GteGetData(GTE_SXY1);
		SCR32(0x3b4) = PC_GteGetData(GTE_SXY2);
	}
	f = ps->flags;

	/* colours (0x800877c0 ... GetAndTransferTint). Retail pipelines the per-corner commands: each
	 * DPCL lights the colour and light loaded before it, so the first works on whatever the GTE
	 * held (its result only shifts through the colour FIFO) and the last follows the loop. */
	c = face[FACE_COLOUR];
#define TABLE(i) pp->colours[c >> 8 * (i) & 0xff]
	if (!(f & FF_GOURAUD)) {
		gte_set(GTE_RGBC, c);
		if ((f & (FF_VERTEXLIT | FF_TINTED)) == (FF_VERTEXLIT | FF_TINTED)) {
			/* each corner lit by its own lighting word: gouraud after all */
			for (i = 0; i < nv; i++) {
				PC_GteCommand(GTE_CMD_DPCL);
				set_light(LIGHT(pp, pp->off[i]));
			}
			if (!tri)
				col0 = PC_GteGetData(GTE_RGB0);
			PC_GteCommand(GTE_CMD_DPCL);
		} else if (f & FF_VERTEXLIT) {
			/* lit by the lighting word of the vertex the record names */
			PC_GteCommand(GTE_CMD_DPCL);
			set_light(*(uint32_t *)((char *)pp->pv + ((const uint16_t *)face)[6] + 3204));
			PC_GteCommand(GTE_CMD_DPCL);
		} else if (f & FF_TINTED) {
			set_light(SCR32(SCR_TINT));
			PC_GteCommand(GTE_CMD_DPCL);
		} else {
			PC_GteCommand(GTE_CMD_DPCS);
		}
		gouraud = (f & (FF_VERTEXLIT | FF_TINTED)) == (FF_VERTEXLIT | FF_TINTED);
	} else {
		gouraud = 1;
		if (f & (FF_VERTEXLIT | FF_TINTED)) {
			uint32_t l = 0;

			if (!(f & FF_TINTED))
				l = *(uint32_t *)((char *)pp->pv + ((const uint16_t *)face)[6] + 3204);
			else if (!(f & FF_VERTEXLIT))
				set_light(l = SCR32(SCR_TINT));
			for (i = 0; i < nv; i++) {
				uint32_t rgbc;

				PC_GteCommand(GTE_CMD_DPCL);
				rgbc = TABLE(i);
				set_light((f & FF_VERTEXLIT) && (f & FF_TINTED) ? LIGHT(pp, pp->off[i]) : l);
				gte_set(GTE_RGBC, rgbc);
			}
			if (!tri)
				col0 = PC_GteGetData(GTE_RGB0);
			PC_GteCommand(GTE_CMD_DPCL);
		} else if (tri) {
			gte_set(GTE_RGB0, TABLE(0));
			gte_set(GTE_RGB1, TABLE(1));
			gte_set(GTE_RGB2, TABLE(2));
			PC_GteCommand(GTE_CMD_DPCT);
		} else {
			gte_set(GTE_RGBC, TABLE(0));
			PC_GteCommand(GTE_CMD_DPCS);
			gte_set(GTE_RGB0, TABLE(1));
			gte_set(GTE_RGB1, TABLE(2));
			col0 = PC_GteGetData(GTE_RGB2);
			gte_set(GTE_RGB2, TABLE(3));
			PC_GteCommand(GTE_CMD_DPCT);
		}
	}
#undef TABLE

	/* the packet: SXY0-2 hold corners 1-3, RGB0-2 the colours (of corners 0-2 of a gouraud
	 * triangle, 1-3 of a gouraud quad, whose corner 0 is col0) */
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
			uint32_t s1 = PC_GteGetData(GTE_SXY0), s2 = PC_GteGetData(GTE_SXY1);

			switch (kind) {
			case 0:
				p[3] = s1;
				p[4] = s2;
				pp_code(ps, PC_GteGetData(GTE_RGB2), CODE_F3);
				len = 4;
				xyw = XY_F3;
				break;
			case SUB_UV:
				p[4] = s1;
				p[6] = s2;
				p[3] = uv0;
				p[5] = uv1;
				p[7] = uv23;
				pp_code(ps, PC_GteGetData(GTE_RGB2), CODE_FT3);
				len = 7;
				xyw = XY_FT3;
				break;
			case SUB_COL:
				p[4] = s1;
				p[6] = s2;
				pp_code(ps, PC_GteGetData(GTE_RGB0), CODE_G3);
				p[3] = PC_GteGetData(GTE_RGB1);
				p[5] = PC_GteGetData(GTE_RGB2);
				len = 6;
				xyw = XY_G3;
				break;
			default:
				p[5] = s1;
				p[8] = s2;
				p[3] = uv0;
				p[6] = uv1;
				p[9] = uv23;
				pp_code(ps, PC_GteGetData(GTE_RGB0), CODE_GT3);
				p[4] = PC_GteGetData(GTE_RGB1);
				p[7] = PC_GteGetData(GTE_RGB2);
				len = 9;
				xyw = XY_GT3;
				colw = COL_GT;
				uvw = UV_GT;
				break;
			}
		} else {
			uint32_t s1 = PC_GteGetData(GTE_SXY0), s2 = PC_GteGetData(GTE_SXY1);
			uint32_t s3 = PC_GteGetData(GTE_SXY2);

			switch (kind) {
			case 0:
				p[3] = s1;
				p[4] = s2;
				p[5] = s3;
				pp_code(ps, PC_GteGetData(GTE_RGB2), CODE_F4);
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
				pp_code(ps, PC_GteGetData(GTE_RGB2), CODE_FT4);
				len = 9;
				xyw = XY_FT4;
				break;
			case SUB_COL:
				p[4] = s1;
				p[6] = s2;
				p[8] = s3;
				pp_code(ps, col0, CODE_G4);
				p[3] = PC_GteGetData(GTE_RGB0);
				p[5] = PC_GteGetData(GTE_RGB1);
				p[7] = PC_GteGetData(GTE_RGB2);
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
				p[4] = PC_GteGetData(GTE_RGB0);
				p[7] = PC_GteGetData(GTE_RGB1);
				p[10] = PC_GteGetData(GTE_RGB2);
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
	if ((f & FF_SUBDIVIDE) && (int32_t)(2048u - PC_GteGetData(GTE_MAC0)) < 0) {
		uint32_t size = subdivide(pp, kind, tri, PC_GteGetData(GTE_MAC0), xyw, colw, uvw);

		if (size == 0)
			return 1; /* every piece off-screen */
		last = ps->pkt - size;
		len = (size - 4) >> 2;
	}
	for (i = 0; i < 4; i++)
		xy[i] = SCR32(0x384 + 16 * i);
	pp_link(ps, start, last, len, xy);
	return 1;
}

void M3dAsm_ProcessPolys__FPUiP7SVECTORi(uint32_t *faces, SVECTOR *normals, int count)
{
	PP pp;
	const uint32_t *face = faces;
	int n = count;

	(void)normals;
	if (count == 0)
		return;
	pp_begin(&pp.ps);
	pp.pv = ProjectedVertices;
	pp.colours = pColourTable;
	pp.a2 = (uint32_t)count;
	gte_setc(GTE_TRX, 0);
	gte_setc(GTE_TRY, 0);
	gte_setc(GTE_TRZ, 0);
	gte_setc(GTE_DQB, 0x10000000); /* RTPS (subdivision) leaves IR0 = 4096 */
	gte_setc(GTE_DQA, 0);
	SCR32(SCR_CLIPWRITE) = CLIPPED_FACE_BUFFER;
	SCR16(SCR_CLIPCOUNT) = 0;
	for (;;) {
		for (; n != 0; n--) {
			const uint32_t *next = (const uint32_t *)((const char *)face +
			    (((face[FACE_FLAGS] & (uint32_t)((int32_t)SCR32(SCR_FACEFLAGS) >> 16)) |
			      (SCR32(SCR_FACEFLAGS) & 0xffff)) >> 16));

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

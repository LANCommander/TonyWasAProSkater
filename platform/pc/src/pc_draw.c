/* The game's drawing (pc_draw.h): its primitive packets (psx-spx "GPU" command words) walked
 * through its ordering tables and drawn by OpenGL (pc_gpu_gl.c): polygons, lines, rectangles and
 * fills go to its render targets; the drawing state (drawing area, offset, texture page and
 * window) is kept here. The native 3D renderer (pc_render3d.c) draws its layers where their
 * markers are in the ordering table. There is no VRAM: the screens are render targets
 * (pc_screen.h) and the textures are in the texture store (pc_texstore.h); packets that copy or
 * transfer VRAM are not supported. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <libgpu.h>

#include "pc_draw.h"
#include "pc_gpu_gl.h"
#include "pc_precise.h"
#include "pc_render3d.h"
#include "pc_runtime.h"
#include "pc_screen.h"

extern volatile uint32_t pc_heartbeat;

#define VRAM_W 1024
#define VRAM_H 512

static struct {
    uint32_t drawmode;           /* E1 bits 0-13 (texpage, abr, depth, dither, dfe, tex off, flips) */
    uint32_t e2;                 /* the texture window, as set */
    uint8_t tw_and_x, tw_and_y, tw_or_x, tw_or_y;
    int clip_l, clip_t, clip_r, clip_b;
    int ofs_x, ofs_y;
    int mask_set, mask_check;
} gs;

static inline int sext11(uint32_t v)
{
    return (int)((v & 0x7ff) ^ 0x400) - 0x400;
}

/* ---------------------------------------------------------------------------------------------
 * Primitives */

typedef struct {
    int textured, raw, semi, abr;
    int tp_x, tp_y, depth;
    int clut_x, clut_y;
} Prim;

static void prim_from_state(Prim *p, int textured, int raw, int semi, uint16_t clut)
{
    uint32_t dm = gs.drawmode;
    static int nosemi_dbg = -1;
    if (nosemi_dbg < 0)
        nosemi_dbg = getenv("THPS_DBG_NOSEMI") && atoi(getenv("THPS_DBG_NOSEMI"));
    p->textured = textured;
    p->raw = textured && raw;
    p->semi = semi && !nosemi_dbg;
    p->abr = (dm >> 5) & 3;
    p->tp_x = (dm & 0xf) * 64;
    p->tp_y = ((dm >> 4) & 1) * 256;
    p->depth = (dm >> 7) & 3;
    p->clut_x = (clut & 0x3f) * 16;
    p->clut_y = (clut >> 6) & 0x1ff;
}

typedef struct {
    int x, y;
    uint8_t r, g, b, u, v;
} Vtx;

static int tri_too_big(const Vtx *a, const Vtx *b, const Vtx *c)
{
    int minx = a->x, maxx = a->x, miny = a->y, maxy = a->y;
    if (b->x < minx) minx = b->x;
    if (b->x > maxx) maxx = b->x;
    if (c->x < minx) minx = c->x;
    if (c->x > maxx) maxx = c->x;
    if (b->y < miny) miny = b->y;
    if (b->y > maxy) maxy = b->y;
    if (c->y < miny) miny = c->y;
    if (c->y > maxy) maxy = c->y;
    int big = (maxx - minx) >= 1024 || (maxy - miny) >= 512;
    if (big && getenv("THPS_GPUSTATS")) {
        static uint32_t frame = ~0u, n;
        if (frame != pc_heartbeat) {
            if (n)
                fprintf(stderr, "gpu f%u: %u oversized triangles rejected\n", frame, n);
            frame = pc_heartbeat;
            n = 0;
        }
        n++;
        if (n <= 3)
            fprintf(stderr, "  too-big tri (%d,%d)-(%d,%d)-(%d,%d)\n", a->x, a->y, b->x, b->y, c->x,
                    c->y);
    }
    return big;
}

/* THPS_GPUDUMP=from-to: log every submitted triangle's raw vertices and final verdict for frames
 * in that (inclusive) range, regardless of which early-out rejects it. */
static int gpudump_active(void)
{
    static int from = -2, to = -2;
    if (from == -2) {
        const char *e = getenv("THPS_GPUDUMP");
        if (e && sscanf(e, "%d-%d", &from, &to) == 2) {
            /* ok */
        } else {
            from = to = -1;
        }
    }
    return from >= 0 && (int)pc_heartbeat >= from && (int)pc_heartbeat <= to;
}

/* ---------------------------------------------------------------------------------------------
 * Commands */

static void vtx_from_words(Vtx *v, uint32_t xy, uint32_t col)
{
    v->x = sext11(xy) + gs.ofs_x;
    v->y = sext11(xy >> 16) + gs.ofs_y;
    v->r = (uint8_t)col;
    v->g = (uint8_t)(col >> 8);
    v->b = (uint8_t)(col >> 16);
    v->u = v->v = 0;
}

/* The view depths of a polygon's vertices given by PC_DRAW_DEPTH up to the flat end (0: none). */
static float s_flat_z[4];

/* The OpenGL backend (pc_gpu_gl.c) gets the primitive with the state it needs. */
static void gl_prim(PC_GlPrim *o, const Prim *p)
{
    o->textured = p->textured;
    o->raw = p->raw;
    o->semi = p->semi;
    o->abr = p->abr;
    o->depth = p->depth;
    o->tp_x = p->tp_x;
    o->tp_y = p->tp_y;
    o->clut_x = p->clut_x;
    o->clut_y = p->clut_y;
    o->tw_and_x = gs.tw_and_x;
    o->tw_and_y = gs.tw_and_y;
    o->tw_or_x = gs.tw_or_x;
    o->tw_or_y = gs.tw_or_y;
    o->mask_set = gs.mask_set;
    o->mask_check = gs.mask_check;
    o->clip_l = gs.clip_l;
    o->clip_t = gs.clip_t;
    o->clip_r = gs.clip_r;
    o->clip_b = gs.clip_b;
    o->wide = PC_GlWideClip(&o->clip_l);   /* widescreen: the screen's margins too */
    o->z = s_flat_z[0];
}

static void gl_vertex(PC_GlVertex *o, const Vtx *v)
{
    o->x = v->x;
    o->y = v->y;
    o->r = v->r;
    o->g = v->g;
    o->b = v->b;
    o->u = v->u;
    o->v = v->v;
    o->px = o->py = o->w = o->z = 0;
}

/* Set while draw_chain is past the ordering table's 2D entries (is_2d_entry). */
static int s_chain_2d;
/* Set between PC_DRAW_FLAT_BEGIN (or PC_DRAW_GLOW) and PC_DRAW_FLAT_END. */
static int s_flat;
/* Set after a glow drawn from its PC_DRAW_GLOW: its polygons up to the flat end are not drawn. */
static int s_glow_drawn;

/* Precise positions and depths of the game's projections (pc_precise.h) for a polygon's vertices,
 * by the screen words in its packet. All or nothing: a polygon that isn't entirely projected (2D,
 * or vertices the game computed itself) is drawn exactly as before. The lookup goes by value, so
 * a 2D quad over dense geometry can find a projection within a pixel of every corner (and be
 * drawn with their unrelated depths: warped HUD text); the 2D entries' polygons are never looked
 * up. */
static void gl_precise(PC_GlVertex *gv, const uint32_t *xy, int n)
{
    static int stats = -1;
    static uint32_t frame, hit, miss;
    float x[4], y[4], z[4];
    int ok = !s_chain_2d && !s_flat && PC_PrecisePolygon(xy, n, x, y, z);
    if (stats < 0)
        stats = getenv("THPS_GLSTATS") && atoi(getenv("THPS_GLSTATS"));
    if (stats) {
        if (pc_heartbeat - frame >= 60) {
            fprintf(stderr, "[gl] precise vertices: %u polygons, %u without\n", hit, miss);
            frame = pc_heartbeat;
            hit = miss = 0;
        }
        if (ok) hit++; else miss++;
    }
    if (!ok)
        return;
    for (int i = 0; i < n; i++) {
        gv[i].px = x[i] + gs.ofs_x;
        gv[i].py = y[i] + gs.ofs_y;
        gv[i].w = z[i];
    }
}

static void line_out(const Prim *p, const Vtx *a, const Vtx *b)
{
    PC_GlPrim gp;
    PC_GlVertex ga, gb;
    gl_prim(&gp, p);
    gl_vertex(&ga, a);
    gl_vertex(&gb, b);
    PC_GlLine(&gp, &ga, &gb);
}

static int glows_retail(void)
{
    static int retail = -1;
    if (retail < 0) {
        const char *e = getenv("THPS_GLOWS");
        retail = e && !strcmp(e, "retail");
    }
    return retail;
}

/* A glow's radius (vertical, pixels) in direction a (radians) from its 8 sections' (rs; section i
 * at a0 + (i + 1) * step): a periodic Catmull-Rom curve through them, so the outline stays round
 * however the sections wobble. */
static float glow_radius(const float rs[8], float a0, float step, float a)
{
    float t = (a - a0) / step - 1.0f;
    t -= floorf(t / 8.0f) * 8.0f;
    int i = (int)t;
    float f = t - (float)i;
    float p0 = rs[(i + 7) & 7], p1 = rs[i & 7], p2 = rs[(i + 1) & 7], p3 = rs[(i + 2) & 7];
    float r = p1 + 0.5f * f * (p2 - p0 + f * (2 * p0 - 5 * p1 + 4 * p2 - p3 +
                                              f * (3 * (p1 - p2) + p3 - p0)));
    return r > 0 ? r : 0;
}

/* PC_DRAW_GLOW (pc_draw.h): the PS1 draws a light's glow as 8 gouraud triangles to its sections
 * and a ring of quads fading out, so up close it is an octagon with creases along the spokes and a
 * sharp edge where the narrow fade starts. Drawn here as a finely divided disc over the same area
 * with the same colours and blending: the centre colour turns into the rim's towards the
 * sections, and the brightness falls off smoothly to nothing at the fringe's outer edge. */
static void cmd_glow(const uint32_t *w)
{
    enum { SEGS = 48, RINGS = 12 };
    s_flat = 1;
    if (glows_retail())
        return;
    s_glow_drawn = 1;

    float cx = (float)(int16_t)w[1] + gs.ofs_x, cy = (float)(int16_t)(w[1] >> 16) + gs.ofs_y;
    float col[3][3];
    for (int k = 0; k < 3; k++)
        for (int c = 0; c < 3; c++)
            col[k][c] = (float)((w[2 + k] >> (8 * c)) & 0xff);
    const float turn = 6.28318531f / 4096.0f, aspect = 400.0f / 256.0f;
    float a0 = (float)(w[5] & 0xfff) * turn, step = (float)(int16_t)(w[5] >> 16) * turn;
    float fringe = (float)(w[6] & 0xffff) / 16.0f, rs[8];
    for (int i = 0; i < 8; i++)
        rs[i] = (float)((w[7 + i / 2] >> (16 * (i & 1))) & 0xffff) / 16.0f;
    if (step == 0)
        return;

    Prim p;
    prim_from_state(&p, 0, 0, 1, 0);
    p.abr = 1;   /* the glows' draw mode: additive */
    PC_GlPrim gp;
    gl_prim(&gp, &p);

    /* each ring: position and colour of its vertices */
    float xy[2][SEGS + 1][2];
    uint8_t rgb[2][SEGS + 1][3];
    float sn[SEGS + 1], cs[SEGS + 1], rad[SEGS + 1];
    for (int j = 0; j <= SEGS; j++) {
        float a = a0 + 6.28318531f * (float)j / SEGS;
        sn[j] = sinf(a);
        cs[j] = cosf(a);
        rad[j] = glow_radius(rs, a0, step, a);
    }
    for (int ring = 0; ring <= RINGS; ring++) {
        float s = (float)ring / RINGS;
        float (*pos)[2] = xy[ring & 1];
        uint8_t (*c8)[3] = rgb[ring & 1];
        for (int j = 0; j <= SEGS; j++) {
            float total = rad[j] + fringe, r = s * total;
            /* centre colour to rim colour across the inner radius */
            float h = total > 0 && rad[j] > 0 ? r / rad[j] : 1;
            h = h >= 1 ? 1 : h * h * (3 - 2 * h);
            /* brightness: 1 at the centre, 0 with a level slope at the outer edge */
            float fall = (1 - s * s) * (1 - s * s);
            for (int c = 0; c < 3; c++) {
                float hue = col[0][c] + (col[1][c] - col[0][c]) * h;
                float v = col[2][c] + (hue - col[2][c]) * fall;
                c8[j][c] = (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v + 0.5f);
            }
            pos[j][0] = cx + r * sn[j] * aspect;
            pos[j][1] = cy - r * cs[j];
        }
        if (!ring)
            continue;
        float (*in)[2] = xy[(ring - 1) & 1];
        uint8_t (*inc)[3] = rgb[(ring - 1) & 1];
        for (int j = 0; j < SEGS; j++) {
            float t[3][2];
            uint8_t tc[3][3];
            memcpy(t[0], in[j], sizeof t[0]);
            memcpy(t[1], pos[j], sizeof t[0]);
            memcpy(t[2], pos[j + 1], sizeof t[0]);
            memcpy(tc[0], inc[j], 3);
            memcpy(tc[1], c8[j], 3);
            memcpy(tc[2], c8[j + 1], 3);
            PC_GlTriangleF(&gp, (const float (*)[2])t, (const uint8_t (*)[3])tc);
            if (ring > 1) {
                memcpy(t[1], in[j + 1], sizeof t[0]);
                memcpy(tc[1], inc[j + 1], 3);
                memcpy(t[2], pos[j + 1], sizeof t[0]);
                memcpy(tc[2], c8[j + 1], 3);
                memcpy(t[0], in[j], sizeof t[0]);
                memcpy(tc[0], inc[j], 3);
                PC_GlTriangleF(&gp, (const float (*)[2])t, (const uint8_t (*)[3])tc);
            }
        }
    }
}

static void cmd_polygon(const uint32_t *w)
{
    uint32_t op = w[0] >> 24;
    int quad = (op & 8) != 0, gouraud = (op & 0x10) != 0, tex = (op & 4) != 0;
    int semi = (op & 2) != 0, raw = (op & 1) != 0;
    int n = quad ? 4 : 3, k = 1;
    uint32_t col = w[0];
    uint16_t clut = 0;
    Vtx v[4];
    uint32_t xy[4];

    for (int i = 0; i < n; i++) {
        if (i > 0 && gouraud)
            col = w[k++];
        xy[i] = w[k];
        vtx_from_words(&v[i], w[k++], col);
        if (tex) {
            uint32_t uv = w[k++];
            v[i].u = (uint8_t)uv;
            v[i].v = (uint8_t)(uv >> 8);
            if (i == 0)
                clut = (uint16_t)(uv >> 16);
            else if (i == 1)
                gs.drawmode = (gs.drawmode & ~0x9ffu) | ((uv >> 16) & 0x9ffu);
        }
    }

    if (getenv("THPS_GPUSTATS")) {
        /* per-frame polygon statistics: count, and how many lie entirely outside the area */
        static uint32_t frame = ~0u, count, outside;
        static int minx, maxx, miny, maxy;
        if (frame != pc_heartbeat) {
            if (count)
                fprintf(stderr, "gpu f%u: %u polygons (%u outside the drawing area), x %d..%d "
                                "y %d..%d, area %d,%d-%d,%d ofs %d,%d\n",
                        frame, count, outside, minx, maxx, miny, maxy, gs.clip_l, gs.clip_t,
                        gs.clip_r, gs.clip_b, gs.ofs_x, gs.ofs_y);
            frame = pc_heartbeat;
            count = outside = 0;
            minx = miny = 1 << 30;
            maxx = maxy = -(1 << 30);
        }
        count++;
        int lo_x = 1 << 30, hi_x = -(1 << 30), lo_y = 1 << 30, hi_y = -(1 << 30);
        for (int i = 0; i < n; i++) {
            int x = v[i].x, y = v[i].y;
            if (x < lo_x) lo_x = x;
            if (x > hi_x) hi_x = x;
            if (y < lo_y) lo_y = y;
            if (y > hi_y) hi_y = y;
        }
        if (hi_x < gs.clip_l || lo_x > gs.clip_r || hi_y < gs.clip_t || lo_y > gs.clip_b)
            outside++;
        if (lo_x < minx) minx = lo_x;
        if (hi_x > maxx) maxx = hi_x;
        if (lo_y < miny) miny = lo_y;
        if (hi_y > maxy) maxy = hi_y;
    }

    Prim p;
    prim_from_state(&p, tex, raw, semi, clut);
    /* THPS_GPUBT="frame,x0,y0,x1,y1": one-shot native backtrace the first time a triangle with a
     * vertex in that box is submitted on that exact heartbeat -- to identify which (possibly
     * unnamed/hand-written-MIPS) caller submits a specific piece of geometry. */
    {
        static int from = -2, bx0, by0, bx1, by1, fired;
        if (from == -2) {
            const char *e = getenv("THPS_GPUBT");
            if (!e || sscanf(e, "%d,%d,%d,%d,%d", &from, &bx0, &by0, &bx1, &by1) != 5)
                from = -1;
        }
        if (!fired && from >= 0 && (int)pc_heartbeat == from) {
            for (int vi = 0; vi < n; vi++) {
                if (v[vi].x >= bx0 && v[vi].x <= bx1 && v[vi].y >= by0 && v[vi].y <= by1) {
                    fired = 1;
                    extern void PC_Backtrace(const char *);
                    char why[128];
                    snprintf(why, sizeof why, "GPUBT hit: v%d=(%d,%d) f%u", vi, v[vi].x, v[vi].y,
                             pc_heartbeat);
                    PC_Backtrace(why);
                    break;
                }
            }
        }
    }
    if (gpudump_active())
        fprintf(stderr,
                "poly f%u quad=%d tex=%d semi=%d abr=%d raw=%d v0=(%d,%d,rgb%02x%02x%02x) "
                "v1=(%d,%d,rgb%02x%02x%02x) v2=(%d,%d,rgb%02x%02x%02x)%s\n",
                pc_heartbeat, quad, tex, semi, p.abr, raw, v[0].x, v[0].y, v[0].r, v[0].g, v[0].b,
                v[1].x, v[1].y, v[1].r, v[1].g, v[1].b, v[2].x, v[2].y, v[2].r, v[2].g, v[2].b,
                quad ? " +quad" : "");
    PC_GlPrim gp;
    PC_GlVertex gv[4];
    gl_prim(&gp, &p);
    for (int i = 0; i < n; i++) {
        gl_vertex(&gv[i], &v[i]);
        gv[i].z = s_flat_z[i];
    }
    gl_precise(gv, xy, n);
    if (!tri_too_big(&v[0], &v[1], &v[2]))
        PC_GlTriangle(&gp, &gv[0], &gv[1], &gv[2]);
    if (quad && !tri_too_big(&v[1], &v[2], &v[3]))
        PC_GlTriangle(&gp, &gv[1], &gv[2], &gv[3]);
}

static void line_prim(Prim *p, uint32_t op)
{
    prim_from_state(p, 0, 0, (op & 2) != 0, 0);
}

/* A line, or a poly-line: its vertices (and colours) follow in the packet up to the end marker.
 * Returns the words used. */
static int cmd_line(const uint32_t *w, int n)
{
    uint32_t op = w[0] >> 24;
    int gouraud = (op & 0x10) != 0, k = gouraud ? 4 : 3;
    Vtx a, b;
    Prim p;
    line_prim(&p, op);
    vtx_from_words(&a, w[1], w[0]);
    if (gouraud)
        vtx_from_words(&b, w[3], w[2]);
    else
        vtx_from_words(&b, w[2], w[0]);
    line_out(&p, &a, &b);
    if (!(op & 8))
        return k;

    int pl_x = b.x - gs.ofs_x, pl_y = b.y - gs.ofs_y, want_col = gouraud;
    uint32_t pl_col = gouraud ? w[2] : w[0], newcol = pl_col;
    while (k < n) {
        uint32_t word = w[k++];
        int vertex_start = gouraud ? want_col : 1;
        if (vertex_start && (word & 0xf000f000u) == 0x50005000u)
            break;
        if (gouraud && want_col) {
            newcol = word;
            want_col = 0;
            continue;
        }
        uint32_t col = gouraud ? newcol : pl_col;
        a.x = pl_x + gs.ofs_x;
        a.y = pl_y + gs.ofs_y;
        a.r = (uint8_t)pl_col;
        a.g = (uint8_t)(pl_col >> 8);
        a.b = (uint8_t)(pl_col >> 16);
        a.u = a.v = 0;
        vtx_from_words(&b, word, col);
        line_out(&p, &a, &b);
        pl_x = sext11(word);
        pl_y = sext11(word >> 16);
        pl_col = col;
        want_col = gouraud;
    }
    return k;
}

static void cmd_rect(const uint32_t *w)
{
    uint32_t op = w[0] >> 24;
    int tex = (op & 4) != 0, semi = (op & 2) != 0, raw = (op & 1) != 0;
    int size = (op >> 3) & 3, k = 2;
    int x = sext11((uint32_t)(sext11(w[1]) + gs.ofs_x));
    int y = sext11((uint32_t)(sext11(w[1] >> 16) + gs.ofs_y));
    unsigned u0 = 0, v0 = 0;
    uint16_t clut = 0;
    int width, height;

    if (tex) {
        u0 = w[2] & 0xff;
        v0 = (w[2] >> 8) & 0xff;
        clut = (uint16_t)(w[2] >> 16);
        k = 3;
    }
    switch (size) {
    case 0:
        width = (int)(w[k] & 0x3ff);
        height = (int)((w[k] >> 16) & 0x1ff);
        break;
    case 1: width = height = 1; break;
    case 2: width = height = 8; break;
    default: width = height = 16; break;
    }

    Prim p;
    prim_from_state(&p, tex, raw, semi, clut);
    unsigned r = w[0] & 0xff, g = (w[0] >> 8) & 0xff, b = (w[0] >> 16) & 0xff;
    int flip_x = (gs.drawmode >> 12) & 1, flip_y = (gs.drawmode >> 13) & 1;

    PC_GlPrim gp;
    gl_prim(&gp, &p);
    PC_GlRect(&gp, x, y, width, height, (uint8_t)r, (uint8_t)g, (uint8_t)b, (int)u0, (int)v0,
              flip_x, flip_y);
}

static void cmd_fill(const uint32_t *w)
{
    int x = (int)(w[1] & 0x3f0), y = (int)((w[1] >> 16) & 0x1ff);
    int width = (int)(((w[2] & 0x3ff) + 0xf) & ~0xfu), height = (int)((w[2] >> 16) & 0x1ff);
    PC_GlFill(x, y, width, height, w[0]);
}

static void set_drawmode(uint32_t v)
{
    gs.drawmode = v & 0x3fff;
}

static void set_texwindow(uint32_t v)
{
    unsigned mx = v & 0x1f, my = (v >> 5) & 0x1f, ox = (v >> 10) & 0x1f, oy = (v >> 15) & 0x1f;
    gs.e2 = v & 0xfffff;
    gs.tw_and_x = (uint8_t)~(mx * 8);
    gs.tw_and_y = (uint8_t)~(my * 8);
    gs.tw_or_x = (uint8_t)((ox & mx) * 8);
    gs.tw_or_y = (uint8_t)((oy & my) * 8);
}

/* THPS_GPUTRACE=<frame>: from that presented frame on, log VRAM transfers, copies, fills and
 * drawing-environment changes to stderr. */
extern volatile uint32_t pc_heartbeat;

static void trace_cmd(const uint32_t *w)
{
    static int from = -2;
    static uint32_t last_env[8];
    uint32_t op = w[0] >> 24;
    if (from == -2) {
        const char *e = getenv("THPS_GPUTRACE");
        from = e ? atoi(e) : -1;
    }
    if (from < 0 || (int)pc_heartbeat < from)
        return;
    if (op >= 0xe1 && op <= 0xe6) {
        if (last_env[op - 0xe0] == w[0])
            return;
        last_env[op - 0xe0] = w[0];
        fprintf(stderr, "gpu f%u: E%x %06x\n", pc_heartbeat, op & 15, w[0] & 0xffffff);
    } else if (op >> 5 == 5 || op >> 5 == 6 || op >> 5 == 4 || op == 0x02) {
        fprintf(stderr, "gpu f%u: %s %08x %08x %08x%s\n", pc_heartbeat,
                op == 0x02 ? "fill" : op >> 5 == 4 ? "copy" : op >> 5 == 5 ? "load" : "store",
                w[0], w[1], w[2], op >> 5 == 4 ? "" : "");
    }
}

static void execute(const uint32_t *w)
{
    uint32_t op = w[0] >> 24;
    trace_cmd(w);
    switch (op >> 5) {
    case 1:
        if (!s_glow_drawn)
            cmd_polygon(w);
        return;
    case 3: cmd_rect(w); return;
    default: break;
    }
    switch (op) {
    case 0x00:
        if (w[0] == PC_DRAW_FLAT_BEGIN || w[0] == PC_DRAW_FLAT_END) {
            s_flat = w[0] == PC_DRAW_FLAT_BEGIN;
            memset(s_flat_z, 0, sizeof s_flat_z);
            s_glow_drawn = 0;
        }
        break;
    case 0x02: cmd_fill(w); break;
    case 0xe1: set_drawmode(w[0]); break;
    case 0xe2: set_texwindow(w[0]); break;
    case 0xe3:
        gs.clip_l = (int)(w[0] & 0x3ff);
        gs.clip_t = (int)((w[0] >> 10) & 0x1ff);
        if (getenv("THPS_DBG_NOCLIP")) {
            gs.clip_l = 0;
            gs.clip_t = 0;
        }
        if (gpudump_active())
            fprintf(stderr, "env f%u E3 clip_l=%d clip_t=%d\n", pc_heartbeat, gs.clip_l, gs.clip_t);
        break;
    case 0xe4:
        gs.clip_r = (int)(w[0] & 0x3ff);
        gs.clip_b = (int)((w[0] >> 10) & 0x1ff);
        if (getenv("THPS_DBG_NOCLIP")) {
            gs.clip_r = VRAM_W - 1;
            gs.clip_b = VRAM_H - 1;
        }
        if (gpudump_active())
            fprintf(stderr, "env f%u E4 clip_r=%d clip_b=%d\n", pc_heartbeat, gs.clip_r, gs.clip_b);
        break;
    case 0xe5:
        gs.ofs_x = sext11(w[0]);
        gs.ofs_y = sext11(w[0] >> 11);
        if (gpudump_active())
            fprintf(stderr, "env f%u E5 ofs_x=%d ofs_y=%d\n", pc_heartbeat, gs.ofs_x, gs.ofs_y);
        break;
    case 0xe6:
        gs.mask_set = (int)(w[0] & 1);
        gs.mask_check = (int)((w[0] >> 1) & 1);
        break;
    default:
        break;  /* NOPs, cache clear */
    }
}

static int gp0_size(uint32_t op)
{
    switch (op >> 5) {
    case 1: {
        int n = (op & 8) ? 4 : 3;
        return 1 + n * ((op & 4) ? 2 : 1) + ((op & 0x10) ? n - 1 : 0);
    }
    case 2: return (op & 0x10) ? 4 : 3;
    case 3: return 2 + ((op & 4) ? 1 : 0) + ((((op >> 3) & 3) == 0) ? 1 : 0);
    case 4: return 4;
    case 5:
    case 6: return 3;
    default: return op == 0x02 ? 3 : 1;
    }
}

/* A packet's command words: each command in place, poly-lines to their end marker. A VRAM copy
 * or transfer (and what follows it in the packet) is skipped. */
static void draw_words(const uint32_t *w, int n)
{
    int i = 0;
    while (i < n) {
        uint32_t op = w[i] >> 24;
        if (op >> 5 >= 4 && op >> 5 <= 6) {
            static int warned;
            if (!warned) {
                warned = 1;
                PC_Log("draw: a VRAM %s packet (%08x) is not supported",
                       op >> 5 == 4 ? "copy" : op >> 5 == 5 ? "load" : "store", w[i]);
            }
            return;
        }
        if (w[i] == PC_DRAW_GLOW && i + PC_DRAW_GLOW_WORDS <= n) {
            cmd_glow(w + i);
            i += PC_DRAW_GLOW_WORDS;
            continue;
        }
        if (w[i] == PC_DRAW_DEPTH && i + PC_DRAW_DEPTH_WORDS <= n) {
            s_flat = 1;
            for (int k = 0; k < 4; k++)
                s_flat_z[k] = (float)(int32_t)w[i + 1 + k];
            s_glow_drawn = 0;
            i += PC_DRAW_DEPTH_WORDS;
            continue;
        }
        int size = gp0_size(op);
        if (i + size > n) {
            static int warned;
            if (!warned) {
                warned = 1;
                PC_Log("draw: command %08x cut short by its packet", w[i]);
            }
            return;
        }
        if (op >> 5 == 2)
            i += cmd_line(w + i, n - i);
        else {
            execute(w + i);
            i += size;
        }
    }
}

/* ---------------------------------------------------------------------------------------------
 * Ordering tables */

/* The ordering-table entries drawn last, with the HUD, panels and text: THPS1's display buffers
 * (SDoubleBuffer) end their ordering table with OT and OT2D (PC_DrawMark2D). */
static const void *s_2d_entries[8];

void PC_DrawMark2D(const void *entry)
{
    for (int i = 0; i < 8; i++)
        if (!s_2d_entries[i] || s_2d_entries[i] == entry) {
            s_2d_entries[i] = entry;
            return;
        }
}

static int is_2d_entry(const P_TAG *p)
{
    for (int i = 0; i < 8 && s_2d_entries[i]; i++)
        if (s_2d_entries[i] == p)
            return 1;
    return 0;
}

int PC_DrawState(int *ofs_x, int *ofs_y, int clip[4])
{
    *ofs_x = gs.ofs_x;
    *ofs_y = gs.ofs_y;
    clip[0] = gs.clip_l;
    clip[1] = gs.clip_t;
    clip[2] = gs.clip_r;
    clip[3] = gs.clip_b;
    return PC_GlWideClip(clip);
}

static void draw_chain(const P_TAG *first)
{
    static int warned, warned_len;
    const P_TAG *p = first, *slow = first;
    for (int guard = 0; guard < (1 << 20); guard++) {
        uint32_t len = (uint32_t)p->len;
        if (len > 16 && !warned_len) {
            warned_len = 1;
            fprintf(stderr, "[draw] packet at %p has %u words\n", (const void *)p, len);
        }
        if (len == 0 && is_2d_entry(p))
            s_chain_2d = 1;
        /* the native 3D renderer's layer markers, and where its layers are in the OT */
        if (!R3d_ChainPacket(p))
            draw_words((const uint32_t *)P_WORDS(p), (int)len);
        p = p->next;
        if (!p)
            return;
        if (guard & 1) {
            slow = slow->next;
            if (slow == p) {
                if (!warned) {
                    warned = 1;
                    fprintf(stderr, "[draw] cyclic ordering table at %p\n", (const void *)p);
                }
                return;
            }
        }
    }
    if (!warned) {
        warned = 1;
        fprintf(stderr, "[draw] iteration cap hit (cyclic ordering table?)\n");
    }
}

/* ---------------------------------------------------------------------------------------------
 * The game's calls (pc_draw.h) */

static void (*s_done_cb)(void);
static int s_done_due;

static void draw_ot(const P_TAG *ot)
{
    draw_chain(ot);
    s_chain_2d = 0;
    s_flat = 0;
    memset(s_flat_z, 0, sizeof s_flat_z);
    s_glow_drawn = 0;
    R3d_ChainEnd();
}

#define MAX_PENDING 32

void PC_DrawOT(const void *ot)
{
    static int s_in_draw, s_npending;
    static const P_TAG *s_pending[MAX_PENDING];
    if (!ot)
        return;
    if (s_in_draw) {
        /* from the done callback: queued, the outer call draws it */
        if (s_npending < MAX_PENDING)
            s_pending[s_npending++] = (const P_TAG *)ot;
        else
            draw_ot((const P_TAG *)ot);
        return;
    }
    s_in_draw = 1;
    draw_ot((const P_TAG *)ot);
    while (s_npending) {
        const P_TAG *next = s_pending[0];
        memmove(&s_pending[0], &s_pending[1], (size_t)(s_npending - 1) * sizeof s_pending[0]);
        s_npending--;
        draw_ot(next);
    }
    s_in_draw = 0;
    /* The PS1 draws an ordering table while the game goes on, and THPS's double buffering
     * (Db_CallBack) takes the callback for "drawing has finished": run it with the next frame,
     * before the frame handler, not before this returns (PC_DrawRunDone). */
    if (s_done_cb)
        s_done_due = 1;
}

void PC_DrawPacket(const void *p)
{
    draw_words((const uint32_t *)P_WORDS(p), (int)((const P_TAG *)p)->len);
}

void PC_DrawClearOTR(void *ot, int n)
{
    P_TAG *o = (P_TAG *)ot;
    for (int i = n - 1; i > 0; i--) {
        o[i].next = &o[i - 1];
        o[i].len = 0;
    }
    if (n > 0) {
        o[0].next = NULL;
        o[0].len = 0;
    }
}

void PC_DrawSetDone(void (*fn)(void))
{
    s_done_cb = fn;
}

void PC_DrawRunDone(void)
{
    if (!s_done_due)
        return;
    s_done_due = 0;
    if (s_done_cb)
        s_done_cb();
}

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static uint32_t word_area(uint32_t cmd, int x, int y)
{
    x = clampi(x, 0, 1023);
    y = clampi(y, 0, 511);
    return cmd | ((uint32_t)(y & 0x3ff) << 10) | (uint32_t)(x & 0x3ff);
}

/* A drawing environment's commands (its dr_env, as libgpu's SetDrawEnv builds them): drawing
 * area, offset, texture page, texture window, mask, and the background fill. */
static int env_words(DRAWENV *env)
{
    u_long *c = env->dr_env.code;
    int n = 0;
    c[n++] = word_area(0xe3000000u, env->clip.x, env->clip.y);
    c[n++] = word_area(0xe4000000u, env->clip.x + env->clip.w - 1, env->clip.y + env->clip.h - 1);
    c[n++] = 0xe5000000u | ((u_long)(env->ofs[1] & 0x7ff) << 11) | (u_long)(env->ofs[0] & 0x7ff);
    c[n++] = _get_mode(env->dfe, env->dtd, env->tpage);
    c[n++] = _get_tw(&env->tw);
    c[n++] = 0xe6000000u;
    if (env->isbg) {
        RECT r = env->clip;
        u_long col = ((u_long)env->b0 << 16) | ((u_long)env->g0 << 8) | env->r0;
        r.w = (short)clampi(r.w, 0, 1023);
        r.h = (short)clampi(r.h, 0, 511);
        if ((r.x & 0x3f) || (r.w & 0x3f)) {
            /* unaligned: a TILE in drawing coordinates (the fill command rounds to 16) */
            c[n++] = 0x60000000u | col;
            c[n++] = ((u_long)(uint16_t)(r.y - env->ofs[1]) << 16) | (uint16_t)(r.x - env->ofs[0]);
            c[n++] = ((u_long)(uint16_t)r.h << 16) | (uint16_t)r.w;
        } else {
            c[n++] = 0x02000000u | col;
            c[n++] = ((u_long)(uint16_t)r.y << 16) | (uint16_t)r.x;
            c[n++] = ((u_long)(uint16_t)r.h << 16) | (uint16_t)r.w;
        }
    }
    env->dr_env.tag.next = NULL;
    env->dr_env.tag.len = (u_long)n;
    return n;
}

void PC_DrawSetEnv(void *drawenv)
{
    DRAWENV *env = (DRAWENV *)drawenv;
    int n = env_words(env);
    /* a screen-sized drawing area is (or will be) a screen */
    PC_GlNoteScreen(env->clip.x, env->clip.y, env->clip.w, env->clip.h);
    draw_words((const uint32_t *)env->dr_env.code, n);
}

void PC_DrawReset(void)
{
    set_drawmode(0);
    set_texwindow(0);
    gs.clip_l = gs.clip_t = gs.clip_r = gs.clip_b = 0;
    gs.ofs_x = gs.ofs_y = 0;
    gs.mask_set = gs.mask_check = 0;
}

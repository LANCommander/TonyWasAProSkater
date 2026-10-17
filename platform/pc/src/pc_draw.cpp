/* The game's drawing (pc_draw.h): its primitive packets (psx-spx "GPU" command words) walked
 * through its ordering tables and drawn by OpenGL (pc_gpu_gl.cpp): polygons, lines, rectangles and
 * fills go to its render targets; the drawing state (drawing area, offset, texture page and
 * window) is kept here. The native 3D renderer (pc_render3d.cpp) draws its layers where their
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
#include "pc_render3d.h"
#include "pc_runtime.h"
#include "pc_screen.h"
#include "pc/draw.h"
#include "pc/gl_gpu.h"
#include "pc/precise.h"
#include "pc/render3d.h"
#include "pc/runtime.h"

extern volatile uint32_t pc_heartbeat;

namespace pc {

constinit OtDrawer g_otDrawer;

namespace {

constexpr int VRAM_W = 1024;
constexpr int VRAM_H = 512;

inline int sext11(uint32_t v)
{
    return (int)((v & 0x7ff) ^ 0x400) - 0x400;
}

} // namespace

/* ---------------------------------------------------------------------------------------------
 * Primitives */

struct OtDrawer::Prim {
    int textured, raw, semi, abr;
    int tp_x, tp_y, depth;
    int clut_x, clut_y;
};

void OtDrawer::PrimFromState(Prim *p, int textured, int raw, int semi, uint16_t clut) const
{
    uint32_t dm = m_gs.drawmode;
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

struct OtDrawer::Vtx {
    int x, y;
    uint8_t r, g, b, u, v;
};

int OtDrawer::TriTooBig(const Vtx *a, const Vtx *b, const Vtx *c)
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

namespace {

/* THPS_GPUDUMP=from-to: log every submitted triangle's raw vertices and final verdict for frames
 * in that (inclusive) range, regardless of which early-out rejects it. */
int gpudump_active()
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

} // namespace

/* ---------------------------------------------------------------------------------------------
 * Commands */

void OtDrawer::VtxFromWords(Vtx *v, uint32_t xy, uint32_t col) const
{
    v->x = sext11(xy) + m_gs.ofs_x;
    v->y = sext11(xy >> 16) + m_gs.ofs_y;
    v->r = (uint8_t)col;
    v->g = (uint8_t)(col >> 8);
    v->b = (uint8_t)(col >> 16);
    v->u = v->v = 0;
}

/* The OpenGL backend (pc_gpu_gl.cpp) gets the primitive with the state it needs. */
void OtDrawer::ToGlPrim(GlPrim *o, const Prim *p) const
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
    o->tw_and_x = m_gs.tw_and_x;
    o->tw_and_y = m_gs.tw_and_y;
    o->tw_or_x = m_gs.tw_or_x;
    o->tw_or_y = m_gs.tw_or_y;
    o->mask_set = m_gs.mask_set;
    o->mask_check = m_gs.mask_check;
    o->clip_l = m_gs.clip_l;
    o->clip_t = m_gs.clip_t;
    o->clip_r = m_gs.clip_r;
    o->clip_b = m_gs.clip_b;
    o->wide = g_glGpu.WideClip(&o->clip_l);   /* widescreen: the screen's margins too */
    o->z = m_flatZ[0];
}

void OtDrawer::ToGlVertex(GlVertex *o, const Vtx *v)
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

/* Precise positions and depths of the game's projections (pc_precise.h) for a polygon's vertices,
 * by the screen words in its packet. All or nothing: a polygon that isn't entirely projected (2D,
 * or vertices the game computed itself) is drawn exactly as before. The lookup goes by value, so
 * a 2D quad over dense geometry can find a projection within a pixel of every corner (and be
 * drawn with their unrelated depths: warped HUD text); the 2D entries' polygons are never looked
 * up. */
void OtDrawer::GlPrecise(GlVertex *gv, const uint32_t *xy, int n) const
{
    static int stats = -1;
    static uint32_t frame, hit, miss;
    float x[4], y[4], z[4];
    int ok = !m_chain2d && !m_flat && g_precise.Polygon(xy, n, x, y, z);
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
        gv[i].px = x[i] + m_gs.ofs_x;
        gv[i].py = y[i] + m_gs.ofs_y;
        gv[i].w = z[i];
    }
}

void OtDrawer::LineOut(const Prim *p, const Vtx *a, const Vtx *b) const
{
    GlPrim gp;
    GlVertex ga, gb;
    ToGlPrim(&gp, p);
    ToGlVertex(&ga, a);
    ToGlVertex(&gb, b);
    g_glGpu.Line(&gp, &ga, &gb);
}

namespace {

int glows_retail()
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
float glow_radius(const float rs[8], float a0, float step, float a)
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

} // namespace

/* PC_DRAW_GLOW (pc_draw.h): the PS1 draws a light's glow as 8 gouraud triangles to its sections
 * and a ring of quads fading out, so up close it is an octagon with creases along the spokes and a
 * sharp edge where the narrow fade starts. Drawn here as a finely divided disc over the same area
 * with the same colours and blending: the centre colour turns into the rim's towards the
 * sections, and the brightness falls off smoothly to nothing at the fringe's outer edge. */
void OtDrawer::CmdGlow(const uint32_t *w)
{
    constexpr int SEGS = 48, RINGS = 12;
    m_flat = 1;
    for (int k = 0; k < 4; k++)
        m_flatZ[k] = (float)(int32_t)w[11];
    if (glows_retail())
        return;
    m_glowDrawn = 1;

    float cx = (float)(int16_t)w[1] + m_gs.ofs_x, cy = (float)(int16_t)(w[1] >> 16) + m_gs.ofs_y;
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
    PrimFromState(&p, 0, 0, 1, 0);
    p.abr = 1;   /* the glows' draw mode: additive */
    GlPrim gp;
    ToGlPrim(&gp, &p);

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
            g_glGpu.TriangleF(&gp, (const float (*)[2])t, (const uint8_t (*)[3])tc);
            if (ring > 1) {
                memcpy(t[1], in[j + 1], sizeof t[0]);
                memcpy(tc[1], inc[j + 1], 3);
                memcpy(t[2], pos[j + 1], sizeof t[0]);
                memcpy(tc[2], c8[j + 1], 3);
                memcpy(t[0], in[j], sizeof t[0]);
                memcpy(tc[0], inc[j], 3);
                g_glGpu.TriangleF(&gp, (const float (*)[2])t, (const uint8_t (*)[3])tc);
            }
        }
    }
}

void OtDrawer::CmdPolygon(const uint32_t *w)
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
        VtxFromWords(&v[i], w[k++], col);
        if (tex) {
            uint32_t uv = w[k++];
            v[i].u = (uint8_t)uv;
            v[i].v = (uint8_t)(uv >> 8);
            if (i == 0)
                clut = (uint16_t)(uv >> 16);
            else if (i == 1)
                m_gs.drawmode = (m_gs.drawmode & ~0x9ffu) | ((uv >> 16) & 0x9ffu);
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
                        frame, count, outside, minx, maxx, miny, maxy, m_gs.clip_l, m_gs.clip_t,
                        m_gs.clip_r, m_gs.clip_b, m_gs.ofs_x, m_gs.ofs_y);
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
        if (hi_x < m_gs.clip_l || lo_x > m_gs.clip_r || hi_y < m_gs.clip_t || lo_y > m_gs.clip_b)
            outside++;
        if (lo_x < minx) minx = lo_x;
        if (hi_x > maxx) maxx = hi_x;
        if (lo_y < miny) miny = lo_y;
        if (hi_y > maxy) maxy = hi_y;
    }

    Prim p;
    PrimFromState(&p, tex, raw, semi, clut);
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
                    char why[128];
                    snprintf(why, sizeof why, "GPUBT hit: v%d=(%d,%d) f%u", vi, v[vi].x, v[vi].y,
                             pc_heartbeat);
                    g_diagnostics.Backtrace(why);
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
    GlPrim gp;
    GlVertex gv[4];
    ToGlPrim(&gp, &p);
    for (int i = 0; i < n; i++) {
        ToGlVertex(&gv[i], &v[i]);
        gv[i].z = m_flatZ[i];
    }
    GlPrecise(gv, xy, n);
    if (!TriTooBig(&v[0], &v[1], &v[2]))
        g_glGpu.Triangle(&gp, &gv[0], &gv[1], &gv[2]);
    if (quad && !TriTooBig(&v[1], &v[2], &v[3]))
        g_glGpu.Triangle(&gp, &gv[1], &gv[2], &gv[3]);
}

/* A line, or a poly-line: its vertices (and colours) follow in the packet up to the end marker.
 * Returns the words used. */
int OtDrawer::CmdLine(const uint32_t *w, int n)
{
    uint32_t op = w[0] >> 24;
    int gouraud = (op & 0x10) != 0, k = gouraud ? 4 : 3;
    Vtx a, b;
    Prim p;
    PrimFromState(&p, 0, 0, (op & 2) != 0, 0);
    VtxFromWords(&a, w[1], w[0]);
    if (gouraud)
        VtxFromWords(&b, w[3], w[2]);
    else
        VtxFromWords(&b, w[2], w[0]);
    LineOut(&p, &a, &b);
    if (!(op & 8))
        return k;

    int pl_x = b.x - m_gs.ofs_x, pl_y = b.y - m_gs.ofs_y, want_col = gouraud;
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
        a.x = pl_x + m_gs.ofs_x;
        a.y = pl_y + m_gs.ofs_y;
        a.r = (uint8_t)pl_col;
        a.g = (uint8_t)(pl_col >> 8);
        a.b = (uint8_t)(pl_col >> 16);
        a.u = a.v = 0;
        VtxFromWords(&b, word, col);
        LineOut(&p, &a, &b);
        pl_x = sext11(word);
        pl_y = sext11(word >> 16);
        pl_col = col;
        want_col = gouraud;
    }
    return k;
}

void OtDrawer::CmdRect(const uint32_t *w)
{
    uint32_t op = w[0] >> 24;
    int tex = (op & 4) != 0, semi = (op & 2) != 0, raw = (op & 1) != 0;
    int size = (op >> 3) & 3, k = 2;
    int x = sext11((uint32_t)(sext11(w[1]) + m_gs.ofs_x));
    int y = sext11((uint32_t)(sext11(w[1] >> 16) + m_gs.ofs_y));
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
    PrimFromState(&p, tex, raw, semi, clut);
    unsigned r = w[0] & 0xff, g = (w[0] >> 8) & 0xff, b = (w[0] >> 16) & 0xff;
    int flip_x = (m_gs.drawmode >> 12) & 1, flip_y = (m_gs.drawmode >> 13) & 1;

    GlPrim gp;
    ToGlPrim(&gp, &p);
    g_glGpu.Rect(&gp, x, y, width, height, (uint8_t)r, (uint8_t)g, (uint8_t)b, (int)u0, (int)v0,
              flip_x, flip_y);
}

namespace {

void cmd_fill(const uint32_t *w)
{
    int x = (int)(w[1] & 0x3f0), y = (int)((w[1] >> 16) & 0x1ff);
    int width = (int)(((w[2] & 0x3ff) + 0xf) & ~0xfu), height = (int)((w[2] >> 16) & 0x1ff);
    g_glGpu.Fill(x, y, width, height, w[0]);
}

} // namespace

void OtDrawer::SetDrawMode(uint32_t v)
{
    m_gs.drawmode = v & 0x3fff;
}

void OtDrawer::SetTexWindow(uint32_t v)
{
    unsigned mx = v & 0x1f, my = (v >> 5) & 0x1f, ox = (v >> 10) & 0x1f, oy = (v >> 15) & 0x1f;
    m_gs.e2 = v & 0xfffff;
    m_gs.tw_and_x = (uint8_t)~(mx * 8);
    m_gs.tw_and_y = (uint8_t)~(my * 8);
    m_gs.tw_or_x = (uint8_t)((ox & mx) * 8);
    m_gs.tw_or_y = (uint8_t)((oy & my) * 8);
}

namespace {

/* THPS_GPUTRACE=<frame>: from that presented frame on, log VRAM transfers, copies, fills and
 * drawing-environment changes to stderr. */
void trace_cmd(const uint32_t *w)
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

} // namespace

void OtDrawer::Execute(const uint32_t *w)
{
    uint32_t op = w[0] >> 24;
    trace_cmd(w);
    switch (op >> 5) {
    case 1:
        if (!m_glowDrawn)
            CmdPolygon(w);
        return;
    case 3: CmdRect(w); return;
    default: break;
    }
    switch (op) {
    case 0x00:
        if (w[0] == PC_DRAW_FLAT_BEGIN || w[0] == PC_DRAW_FLAT_END) {
            m_flat = w[0] == PC_DRAW_FLAT_BEGIN;
            memset(m_flatZ, 0, sizeof m_flatZ);
            m_glowDrawn = 0;
        }
        break;
    case 0x02: cmd_fill(w); break;
    case 0xe1: SetDrawMode(w[0]); break;
    case 0xe2: SetTexWindow(w[0]); break;
    case 0xe3:
        m_gs.clip_l = (int)(w[0] & 0x3ff);
        m_gs.clip_t = (int)((w[0] >> 10) & 0x1ff);
        if (getenv("THPS_DBG_NOCLIP")) {
            m_gs.clip_l = 0;
            m_gs.clip_t = 0;
        }
        if (gpudump_active())
            fprintf(stderr, "env f%u E3 clip_l=%d clip_t=%d\n", pc_heartbeat, m_gs.clip_l,
                    m_gs.clip_t);
        break;
    case 0xe4:
        m_gs.clip_r = (int)(w[0] & 0x3ff);
        m_gs.clip_b = (int)((w[0] >> 10) & 0x1ff);
        if (getenv("THPS_DBG_NOCLIP")) {
            m_gs.clip_r = VRAM_W - 1;
            m_gs.clip_b = VRAM_H - 1;
        }
        if (gpudump_active())
            fprintf(stderr, "env f%u E4 clip_r=%d clip_b=%d\n", pc_heartbeat, m_gs.clip_r,
                    m_gs.clip_b);
        break;
    case 0xe5:
        m_gs.ofs_x = sext11(w[0]);
        m_gs.ofs_y = sext11(w[0] >> 11);
        if (gpudump_active())
            fprintf(stderr, "env f%u E5 ofs_x=%d ofs_y=%d\n", pc_heartbeat, m_gs.ofs_x,
                    m_gs.ofs_y);
        break;
    case 0xe6:
        m_gs.mask_set = (int)(w[0] & 1);
        m_gs.mask_check = (int)((w[0] >> 1) & 1);
        break;
    default:
        break;  /* NOPs, cache clear */
    }
}

namespace {

int gp0_size(uint32_t op)
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

} // namespace

/* A packet's command words: each command in place, poly-lines to their end marker. A VRAM copy
 * or transfer (and what follows it in the packet) is skipped. */
void OtDrawer::DrawWords(const uint32_t *w, int n)
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
            CmdGlow(w + i);
            i += PC_DRAW_GLOW_WORDS;
            continue;
        }
        if (w[i] == PC_DRAW_DEPTH && i + PC_DRAW_DEPTH_WORDS <= n) {
            m_flat = 1;
            for (int k = 0; k < 4; k++)
                m_flatZ[k] = (float)(int32_t)w[i + 1 + k];
            m_glowDrawn = 0;
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
            i += CmdLine(w + i, n - i);
        else {
            Execute(w + i);
            i += size;
        }
    }
}

/* ---------------------------------------------------------------------------------------------
 * Ordering tables */

void OtDrawer::Mark2D(const void *entry)
{
    for (int i = 0; i < 8; i++)
        if (!m_2dEntries[i] || m_2dEntries[i] == entry) {
            m_2dEntries[i] = entry;
            return;
        }
}

bool OtDrawer::Is2dEntry(const P_TAG *p) const
{
    for (int i = 0; i < 8 && m_2dEntries[i]; i++)
        if (m_2dEntries[i] == p)
            return true;
    return false;
}

int OtDrawer::State(int *ofs_x, int *ofs_y, int clip[4]) const
{
    *ofs_x = m_gs.ofs_x;
    *ofs_y = m_gs.ofs_y;
    clip[0] = m_gs.clip_l;
    clip[1] = m_gs.clip_t;
    clip[2] = m_gs.clip_r;
    clip[3] = m_gs.clip_b;
    return g_glGpu.WideClip(clip);
}

void OtDrawer::DrawChain(const P_TAG *first)
{
    static int warned, warned_len;
    const P_TAG *p = first, *slow = first;
    for (int guard = 0; guard < (1 << 20); guard++) {
        uint32_t len = (uint32_t)p->len;
        if (len > 16 && !warned_len) {
            warned_len = 1;
            fprintf(stderr, "[draw] packet at %p has %u words\n", (const void *)p, len);
        }
        if (len == 0 && Is2dEntry(p))
            m_chain2d = 1;
        /* the native 3D renderer's layer markers, and where its layers are in the OT */
        if (!g_renderer3d.ChainPacket(p))
            DrawWords((const uint32_t *)P_WORDS(p), (int)len);
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
 * Drawing ordering tables and environments */

void OtDrawer::DrawOne(const P_TAG *ot)
{
    DrawChain(ot);
    m_chain2d = 0;
    m_flat = 0;
    memset(m_flatZ, 0, sizeof m_flatZ);
    m_glowDrawn = 0;
    g_renderer3d.ChainEnd();
}

void OtDrawer::DrawOT(const void *ot)
{
    if (!ot)
        return;
    if (m_inDraw) {
        /* from the done callback: queued, the outer call draws it */
        if (m_npending < MAX_PENDING)
            m_pending[m_npending++] = (const P_TAG *)ot;
        else
            DrawOne((const P_TAG *)ot);
        return;
    }
    m_inDraw = 1;
    DrawOne((const P_TAG *)ot);
    while (m_npending) {
        const P_TAG *next = m_pending[0];
        memmove(&m_pending[0], &m_pending[1], (size_t)(m_npending - 1) * sizeof m_pending[0]);
        m_npending--;
        DrawOne(next);
    }
    m_inDraw = 0;
    /* The PS1 draws an ordering table while the game goes on, and THPS's double buffering
     * (Db_CallBack) takes the callback for "drawing has finished": run it with the next frame,
     * before the frame handler, not before this returns (RunDone). */
    if (m_doneCb)
        m_doneDue = 1;
}

void OtDrawer::DrawPacket(const void *p)
{
    DrawWords((const uint32_t *)P_WORDS(p), (int)((const P_TAG *)p)->len);
}

void OtDrawer::ClearOTR(void *ot, int n)
{
    P_TAG *o = (P_TAG *)ot;
    for (int i = n - 1; i > 0; i--) {
        o[i].next = &o[i - 1];
        o[i].len = 0;
    }
    if (n > 0) {
        o[0].next = nullptr;
        o[0].len = 0;
    }
}

/* Once a frame, before the game's frame handler (pc_frame.cpp). */
void OtDrawer::RunDone()
{
    if (!m_doneDue)
        return;
    m_doneDue = 0;
    if (m_doneCb)
        m_doneCb();
}

namespace {

int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

uint32_t word_area(uint32_t cmd, int x, int y)
{
    x = clampi(x, 0, 1023);
    y = clampi(y, 0, 511);
    return cmd | ((uint32_t)(y & 0x3ff) << 10) | (uint32_t)(x & 0x3ff);
}

/* A drawing environment's commands (its dr_env, as libgpu's SetDrawEnv builds them): drawing
 * area, offset, texture page, texture window, mask, and the background fill. */
int env_words(DRAWENV *env)
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
    env->dr_env.tag.next = nullptr;
    env->dr_env.tag.len = (u_long)n;
    return n;
}

} // namespace

void OtDrawer::SetEnv(void *drawenv)
{
    DRAWENV *env = (DRAWENV *)drawenv;
    int n = env_words(env);
    /* a screen-sized drawing area is (or will be) a screen */
    g_glGpu.NoteScreen(env->clip.x, env->clip.y, env->clip.w, env->clip.h);
    DrawWords((const uint32_t *)env->dr_env.code, n);
}

void OtDrawer::Reset()
{
    SetDrawMode(0);
    SetTexWindow(0);
    m_gs.clip_l = m_gs.clip_t = m_gs.clip_r = m_gs.clip_b = 0;
    m_gs.ofs_x = m_gs.ofs_y = 0;
    m_gs.mask_set = m_gs.mask_check = 0;
}

} // namespace pc

/* ---- the game's interface (pc_draw.h) ---- */

extern "C" {

void PC_DrawOT(const void *ot) { pc::g_otDrawer.DrawOT(ot); }
void PC_DrawMark2D(const void *entry) { pc::g_otDrawer.Mark2D(entry); }
void PC_DrawPacket(const void *p) { pc::g_otDrawer.DrawPacket(p); }
void PC_DrawClearOTR(void *ot, int n) { pc::OtDrawer::ClearOTR(ot, n); }
void PC_DrawSetEnv(void *drawenv) { pc::g_otDrawer.SetEnv(drawenv); }
void PC_DrawSetDone(void (*fn)(void)) { pc::g_otDrawer.SetDone(fn); }
void PC_DrawReset(void) { pc::g_otDrawer.Reset(); }

} // extern "C"

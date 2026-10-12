/* Software PS1 GPU: GP0/GP1 command processing over a 1024x512 16-bit VRAM (psx-spx "GPU").
 *
 * The triangle rasteriser is the integer DDA of the Mednafen/DuckStation software renderer, as
 * adapted in the Vandal Hearts PC port's pc_raster.c (HalmyLyseas/VandalHearts-PcPort, GPL-2.0):
 * 32.32 fixed-point edges, attributes with 12+12 fraction bits evaluated at integer pixel positions,
 * top-left fill convention. This file extends it with Gouraud shading, lines, rectangles, mask bits
 * and the complete command set, and takes real GP0 words instead of VH's packet tokens. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_gpu.h"
#include "pc_gpu_gl.h"
#include "pc_gte.h"
#include "pc_render3d.h"

extern volatile uint32_t pc_heartbeat;

#define VRAM_W 1024
#define VRAM_H 512

static uint16_t s_vram[VRAM_W * VRAM_H];
#define VRAM(x, y) s_vram[(((y) & (VRAM_H - 1)) * VRAM_W) + ((x) & (VRAM_W - 1))]

enum { MODE_CMD, MODE_POLYLINE, MODE_TO_VRAM };

static struct {
    /* GP0 state */
    uint32_t drawmode;           /* E1 bits 0-13 (texpage, abr, depth, dither, dfe, tex off, flips) */
    uint32_t e2, e3, e4, e5;     /* last values, for GP1(10h) */
    uint8_t tw_and_x, tw_and_y, tw_or_x, tw_or_y;
    int clip_l, clip_t, clip_r, clip_b;
    int ofs_x, ofs_y;
    int mask_set, mask_check;

    /* command assembly */
    uint32_t fifo[16];
    int fifo_n, fifo_need;
    int mode;

    /* poly-line continuation */
    uint32_t pl_op;
    int pl_x, pl_y;
    uint32_t pl_col, pl_newcol;
    int pl_want_col;

    /* CPU->VRAM / VRAM->CPU transfers */
    int xfer_x, xfer_y, xfer_w, xfer_h, xfer_i, xfer_n;
    int rd_x, rd_y, rd_w, rd_h, rd_i, rd_n;
    uint32_t read_latch;

    /* GP1 state */
    int disp_off;
    int dma_dir;
    int disp_x, disp_y;
    int h_start, h_end, v_start, v_end;
    uint32_t disp_mode;          /* GP1(08h) bits 0-7 */
    int irq;
    int field;
} gs;

static int s_dither_force_off = -1;
static uint8_t s_dither_lut[4][4][512];
static int s_lut_ready;

static const int8_t k_dither[4][4] = {
    { -4, 0, -3, 1 },
    { 2, -2, 3, -1 },
    { -3, 1, -4, 0 },
    { 3, -1, 2, -2 },
};

static void init_tables(void)
{
    if (s_lut_ready)
        return;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int v = 0; v < 512; v++) {
                int d = (v + k_dither[i][j]) >> 3;
                s_dither_lut[i][j][v] = (uint8_t)(d < 0 ? 0 : d > 31 ? 31 : d);
            }
    if (s_dither_force_off < 0) {
        const char *e = getenv("THPS_GPU_DITHER");
        s_dither_force_off = (e && e[0] == '0') ? 1 : 0;
    }
    s_lut_ready = 1;
}

static inline int sext11(uint32_t v)
{
    return (int)((v & 0x7ff) ^ 0x400) - 0x400;
}

uint16_t *PC_GpuVram(void)
{
    return s_vram;
}

/* ---------------------------------------------------------------------------------------------
 * Pixel pipeline */

typedef struct {
    int textured, raw, semi, abr, dither;
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
    p->dither = 0;
}

static inline uint16_t sample_texel(const Prim *p, unsigned u, unsigned v)
{
    u = (u & gs.tw_and_x) | gs.tw_or_x;
    v = (v & gs.tw_and_y) | gs.tw_or_y;
    switch (p->depth) {
    case 0: {
        uint16_t w = VRAM(p->tp_x + (int)(u >> 2), p->tp_y + (int)v);
        unsigned idx = (w >> ((u & 3) * 4)) & 0xf;
        return VRAM(p->clut_x + (int)idx, p->clut_y);
    }
    case 1: {
        uint16_t w = VRAM(p->tp_x + (int)(u >> 1), p->tp_y + (int)v);
        unsigned idx = (w >> ((u & 1) * 8)) & 0xff;
        return VRAM(p->clut_x + (int)idx, p->clut_y);
    }
    default:
        return VRAM(p->tp_x + (int)u, p->tp_y + (int)v);
    }
}

/* Semi-transparency on packed BGR555 (blargg's parallel channel math); fg carries bit 15. */
static inline uint16_t blend(uint32_t fg, uint32_t bg, int abr)
{
    uint32_t c;
    switch (abr) {
    case 0:
        bg |= 0x8000u;
        c = ((fg + bg) - ((fg ^ bg) & 0x0421u)) >> 1;
        break;
    case 1: {
        bg &= ~0x8000u;
        uint32_t sum = fg + bg;
        uint32_t carry = (sum - ((fg ^ bg) & 0x8421u)) & 0x8420u;
        c = (sum - carry) | (carry - (carry >> 5));
        break;
    }
    case 2: {
        bg |= 0x8000u;
        fg &= ~0x8000u;
        uint32_t diff = bg - fg + 0x108420u;
        uint32_t borrow = (diff - ((bg ^ fg) & 0x108420u)) & 0x108420u;
        c = (diff - borrow) & (borrow - (borrow >> 5));
        break;
    }
    default: {
        bg &= ~0x8000u;
        fg = ((fg >> 2) & 0x1ce7u) | 0x8000u;
        uint32_t sum = fg + bg;
        uint32_t carry = (sum - ((fg ^ bg) & 0x8421u)) & 0x8420u;
        c = (sum - carry) | (carry - (carry >> 5));
        break;
    }
    }
    return (uint16_t)c;
}

/* THPS_GPUDUMP pixel-level stats: how many pixels a triangle's scanline loop attempted vs how many
 * bailed out on a transparent (zero) texel, reset per-triangle by draw_triangle. */
uint32_t g_tex_attempts, g_tex_skips;
int g_tex_stat_on;

/* THPS_GPUDUMP + THPS_GPUDUMPBOX="x0,y0,x1,y1": count every shade_pixel entry (regardless of
 * transparency outcome) whose (x,y) falls in the box, to tell "nothing ever targeted this pixel"
 * apart from "something targeted it but bailed." */
static void coverage_hit(int x, int y)
{
    static int from = -2, to = -2, bx0, by0, bx1, by1;
    static uint32_t frame = ~0u, n;
    if (from == -2) {
        const char *e = getenv("THPS_GPUDUMP");
        if (!e || sscanf(e, "%d-%d", &from, &to) != 2)
            from = to = -1;
        const char *b = getenv("THPS_GPUDUMPBOX");
        if (!b || sscanf(b, "%d,%d,%d,%d", &bx0, &by0, &bx1, &by1) != 4)
            bx0 = by0 = bx1 = by1 = 0;
    }
    if (from < 0 || (int)pc_heartbeat < from || (int)pc_heartbeat > to)
        return;
    if (frame != pc_heartbeat) {
        if (frame != ~0u)
            fprintf(stderr, "coverage f%u: %u shade_pixel calls hit box %d,%d-%d,%d\n", frame, n, bx0,
                    by0, bx1, by1);
        frame = pc_heartbeat;
        n = 0;
    }
    if (x >= bx0 && x <= bx1 && y >= by0 && y <= by1)
        n++;
}

static inline void shade_pixel(const Prim *p, int x, int y, unsigned r, unsigned g, unsigned b,
                               unsigned u, unsigned v)
{
    uint16_t color;
    int dy = p->dither ? (y & 3) : 2, dx = p->dither ? (x & 3) : 3;
    const uint8_t *lut = s_dither_lut[dy][dx];

    coverage_hit(x, y);

    if (p->textured) {
        uint16_t t = sample_texel(p, u & 0xff, v & 0xff);
        if (g_tex_stat_on) {
            g_tex_attempts++;
            if (t == 0)
                g_tex_skips++;
        }
        static int opaque_dbg = -1;
        if (opaque_dbg < 0)
            opaque_dbg = getenv("THPS_DBG_OPAQUE") && atoi(getenv("THPS_DBG_OPAQUE"));
        if (t == 0 && opaque_dbg)
            t = 0x8000u | 0x1ce7u; /* mid-gray, non-transparent: bisection probe */
        if (t == 0)
            return;
        if (p->raw) {
            color = t;
        } else {
            color = (uint16_t)(lut[((t & 31u) * r) >> 4] | (lut[(((t >> 5) & 31u) * g) >> 4] << 5) |
                               (lut[(((t >> 10) & 31u) * b) >> 4] << 10) | (t & 0x8000u));
        }
    } else {
        color = (uint16_t)(lut[r] | (lut[g] << 5) | (lut[b] << 10));
    }

    uint16_t *dst = &VRAM(x, y);
    uint16_t bg = *dst;
    if (p->semi && (!p->textured || (color & 0x8000u))) {
        color = blend((uint32_t)color | 0x8000u, bg, p->abr);
        if (!p->textured)
            color &= 0x7fffu;
    }
    if (gs.mask_check && (bg & 0x8000u))
        return;
    *dst = (uint16_t)(color | (gs.mask_set ? 0x8000u : 0));
}

/* ---------------------------------------------------------------------------------------------
 * Triangles */

typedef struct {
    int x, y;
    uint8_t r, g, b, u, v;
} Vtx;

#define FBS 12  /* attribute fraction bits */
#define FPP 12  /* extra post-padding for u/v */

typedef struct { uint32_t u, v, r, g, b; } IGroup;
typedef struct {
    uint32_t du_dx, dv_dx, dr_dx, dg_dx, db_dx;
    uint32_t du_dy, dv_dy, dr_dy, dg_dy, db_dy;
} IDeltas;

static inline void ig_step_x(IGroup *ig, const IDeltas *d, int n)
{
    uint32_t k = (uint32_t)n;
    ig->u += d->du_dx * k; ig->v += d->dv_dx * k;
    ig->r += d->dr_dx * k; ig->g += d->dg_dx * k; ig->b += d->db_dx * k;
}

static inline void ig_step_y(IGroup *ig, const IDeltas *d, int n)
{
    uint32_t k = (uint32_t)n;
    ig->u += d->du_dy * k; ig->v += d->dv_dy * k;
    ig->r += d->dr_dy * k; ig->g += d->dg_dy * k; ig->b += d->db_dy * k;
}

static inline int64_t xfp_make(int x)
{
    return (int64_t)((uint64_t)(int64_t)x << 32) + ((INT64_C(1) << 32) - (1 << 11));
}

static inline int64_t xfp_step(int dx, int dy)
{
    int64_t bias = dx < 0 ? -(int64_t)(dy - 1) : dx > 0 ? (int64_t)(dy - 1) : 0;
    return ((int64_t)((uint64_t)(int64_t)dx << 32) + bias) / dy;
}

static inline int xfp_int(int64_t v)
{
    return (int)(v >> 32);
}

static void draw_span(const Prim *p, int y, int x_start, int x_bound, IGroup ig, const IDeltas *d)
{
    int width = x_bound - x_start;
    int x = x_start;
    if (x < gs.clip_l) {
        int k = gs.clip_l - x;
        x_start += k;
        x += k;
        width -= k;
    }
    if (x + width > gs.clip_r + 1)
        width = gs.clip_r + 1 - x;
    if (width <= 0)
        return;
    ig_step_x(&ig, d, x_start);
    ig_step_y(&ig, d, y);
    do {
        shade_pixel(p, x, y, (ig.r >> FBS) & 0xff, (ig.g >> FBS) & 0xff, (ig.b >> FBS) & 0xff,
                    (ig.u >> (FBS + FPP)) & 0xff, (ig.v >> (FBS + FPP)) & 0xff);
        x++;
        ig_step_x(&ig, d, 1);
    } while (--width > 0);
}

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

static void draw_triangle(const Prim *p, int gouraud, const Vtx *v0, const Vtx *v1, const Vtx *v2)
{
    const Vtx *t;
    unsigned cv;
    int dump = gpudump_active();
    const Vtx *o0 = v0, *o1 = v1, *o2 = v2;

    if (tri_too_big(v0, v1, v2)) {
        if (dump)
            fprintf(stderr, "tri f%u (%d,%d,rgb%02x%02x%02x)-(%d,%d)-(%d,%d) -> TOO_BIG\n",
                    pc_heartbeat, o0->x, o0->y, o0->r, o0->g, o0->b, o1->x, o1->y, o2->x, o2->y);
        return;
    }

    /* sort by y, tracking which vertex is the top-left "core" vertex */
    if (v1->x <= v0->x)
        cv = (v2->x <= v1->x) ? 4 : 2;
    else if (v2->x < v0->x)
        cv = 4;
    else
        cv = 1;
    if (v2->y < v1->y) {
        t = v1; v1 = v2; v2 = t;
        cv = ((cv >> 1) & 2) | ((cv << 1) & 4) | (cv & 1);
    }
    if (v1->y < v0->y) {
        t = v0; v0 = v1; v1 = t;
        cv = ((cv >> 1) & 1) | ((cv << 1) & 2) | (cv & 4);
    }
    if (v2->y < v1->y) {
        t = v1; v1 = v2; v2 = t;
        cv = ((cv >> 1) & 2) | ((cv << 1) & 4) | (cv & 1);
    }
    cv >>= 1;

    if (v0->y == v2->y) {
        if (dump)
            fprintf(stderr, "tri f%u (%d,%d)-(%d,%d)-(%d,%d) -> FLAT_Y\n", pc_heartbeat, o0->x,
                    o0->y, o1->x, o1->y, o2->x, o2->y);
        return;
    }

    int64_t base_coord = xfp_make(v0->x);
    int64_t base_step = xfp_step(v2->x - v0->x, v2->y - v0->y);
    int64_t bound_us, bound_ls;
    int right_facing;
    if (v1->y == v0->y) {
        bound_us = 0;
        right_facing = v1->x > v0->x;
    } else {
        bound_us = xfp_step(v1->x - v0->x, v1->y - v0->y);
        right_facing = bound_us > base_step;
    }
    bound_ls = (v2->y == v1->y) ? 0 : xfp_step(v2->x - v1->x, v2->y - v1->y);

    /* attribute gradients: det(A, B) over the screen-space determinant */
#define CALCIS(A, B) ((int64_t)(v1->A - v0->A) * (v2->B - v1->B) - (int64_t)(v2->A - v1->A) * (v1->B - v0->B))
    int64_t denom = CALCIS(x, y);
    if (denom == 0) {
        if (dump)
            fprintf(stderr, "tri f%u (%d,%d)-(%d,%d)-(%d,%d) -> ZERO_AREA\n", pc_heartbeat, o0->x,
                    o0->y, o1->x, o1->y, o2->x, o2->y);
        return;
    }
    {
        static long mindenom_dbg = -1;
        if (mindenom_dbg < 0) {
            const char *e = getenv("THPS_DBG_MINDENOM");
            mindenom_dbg = e ? atol(e) : 0;
        }
        if (mindenom_dbg && (denom < mindenom_dbg && denom > -mindenom_dbg)) {
            if (dump)
                fprintf(stderr, "tri f%u (%d,%d)-(%d,%d)-(%d,%d) denom=%lld -> MINDENOM_SKIP\n",
                        pc_heartbeat, o0->x, o0->y, o1->x, o1->y, o2->x, o2->y, (long long)denom);
            return;
        }
    }
    IDeltas d;
    memset(&d, 0, sizeof d);
    if (gouraud) {
        d.dr_dx = (uint32_t)(int32_t)(CALCIS(r, y) * (1 << FBS) / denom);
        d.dg_dx = (uint32_t)(int32_t)(CALCIS(g, y) * (1 << FBS) / denom);
        d.db_dx = (uint32_t)(int32_t)(CALCIS(b, y) * (1 << FBS) / denom);
        d.dr_dy = (uint32_t)(int32_t)(CALCIS(x, r) * (1 << FBS) / denom);
        d.dg_dy = (uint32_t)(int32_t)(CALCIS(x, g) * (1 << FBS) / denom);
        d.db_dy = (uint32_t)(int32_t)(CALCIS(x, b) * (1 << FBS) / denom);
    }
    if (p->textured) {
        d.du_dx = (uint32_t)(int32_t)(CALCIS(u, y) * (1 << FBS) / denom) << FPP;
        d.dv_dx = (uint32_t)(int32_t)(CALCIS(v, y) * (1 << FBS) / denom) << FPP;
        d.du_dy = (uint32_t)(int32_t)(CALCIS(x, u) * (1 << FBS) / denom) << FPP;
        d.dv_dy = (uint32_t)(int32_t)(CALCIS(x, v) * (1 << FBS) / denom) << FPP;
        if (dump)
            fprintf(stderr,
                    "  tex f%u (%d,%d)-(%d,%d)-(%d,%d) denom=%lld du_dx=%d du_dy=%d dv_dx=%d "
                    "dv_dy=%d u=%d,%d,%d v=%d,%d,%d\n",
                    pc_heartbeat, o0->x, o0->y, o1->x, o1->y, o2->x, o2->y, (long long)denom,
                    (int32_t)d.du_dx, (int32_t)d.du_dy, (int32_t)d.dv_dx, (int32_t)d.dv_dy, o0->u,
                    o1->u, o2->u, o0->v, o1->v, o2->v);
    }
#undef CALCIS

    if (dump)
        fprintf(stderr, "tri f%u (%d,%d)-(%d,%d)-(%d,%d) clip %d,%d-%d,%d -> drawn\n", pc_heartbeat,
                o0->x, o0->y, o1->x, o1->y, o2->x, o2->y, gs.clip_l, gs.clip_t, gs.clip_r, gs.clip_b);

    const Vtx *vs[3] = { v0, v1, v2 };
    const Vtx *core = vs[cv];
    IGroup ig;
    ig.u = (((uint32_t)core->u << FBS) + (1u << (FBS - 1))) << FPP;
    ig.v = (((uint32_t)core->v << FBS) + (1u << (FBS - 1))) << FPP;
    ig.r = ((uint32_t)core->r << FBS) + (1u << (FBS - 1));
    ig.g = ((uint32_t)core->g << FBS) + (1u << (FBS - 1));
    ig.b = ((uint32_t)core->b << FBS) + (1u << (FBS - 1));
    ig_step_x(&ig, &d, -core->x);
    ig_step_y(&ig, &d, -core->y);

    struct {
        int64_t x[2], step[2];
        int y, y_bound, dec;
    } part[2];
    unsigned vo = (cv != 0) ? 1 : 0;
    unsigned vp = (cv == 2) ? 3 : 0;
    int rf = right_facing ? 1 : 0;

    part[vo].y = vs[0 ^ vo]->y;
    part[vo].y_bound = vs[1 ^ vo]->y;
    part[vo].x[rf] = xfp_make(vs[0 ^ vo]->x);
    part[vo].step[rf] = bound_us;
    part[vo].x[!rf] = base_coord + (int64_t)(vs[vo]->y - vs[0]->y) * base_step;
    part[vo].step[!rf] = base_step;
    part[vo].dec = (int)vo;

    part[vo ^ 1].y = vs[1 ^ vp]->y;
    part[vo ^ 1].y_bound = vs[2 ^ vp]->y;
    part[vo ^ 1].x[rf] = xfp_make(vs[1 ^ vp]->x);
    part[vo ^ 1].step[rf] = bound_ls;
    part[vo ^ 1].x[!rf] = base_coord + (int64_t)(vs[1 ^ vp]->y - vs[0]->y) * base_step;
    part[vo ^ 1].step[!rf] = base_step;
    part[vo ^ 1].dec = vp != 0;

    g_tex_stat_on = dump && p->textured;
    g_tex_attempts = g_tex_skips = 0;

    for (int i = 0; i < 2; i++) {
        int y = part[i].y, yb = part[i].y_bound;
        int64_t lc = part[i].x[0], ls = part[i].step[0];
        int64_t rc = part[i].x[1], rs = part[i].step[1];
        if (part[i].dec) {
            while (y > yb) {
                y--;
                lc -= ls;
                rc -= rs;
                if (y < gs.clip_t)
                    break;
                if (y > gs.clip_b)
                    continue;
                draw_span(p, y, xfp_int(lc), xfp_int(rc), ig, &d);
            }
        } else {
            while (y < yb) {
                if (y > gs.clip_b)
                    break;
                if (y >= gs.clip_t)
                    draw_span(p, y, xfp_int(lc), xfp_int(rc), ig, &d);
                y++;
                lc += ls;
                rc += rs;
            }
        }
    }

    if (g_tex_stat_on) {
        fprintf(stderr, "  pix f%u (%d,%d)-(%d,%d)-(%d,%d) attempts=%u zero_skips=%u\n", pc_heartbeat,
                o0->x, o0->y, o1->x, o1->y, o2->x, o2->y, g_tex_attempts, g_tex_skips);
        g_tex_stat_on = 0;
    }
}

/* ---------------------------------------------------------------------------------------------
 * Lines (both endpoints drawn) */

static void draw_line(const Prim *p, int gouraud, const Vtx *a, const Vtx *b)
{
    int dx = abs(b->x - a->x), dy = abs(b->y - a->y);
    int k = dx > dy ? dx : dy;
    if (dx >= 1024 || dy >= 512)
        return;
    if (a->x >= b->x && k > 0) {
        const Vtx *t = a;
        a = b;
        b = t;
    }

    int64_t sx = 0, sy = 0;
    int32_t sr = 0, sg = 0, sb = 0;
    if (k) {
        int64_t ddx = (int64_t)((uint64_t)(int64_t)(b->x - a->x) << 32);
        int64_t ddy = (int64_t)((uint64_t)(int64_t)(b->y - a->y) << 32);
        if (ddx < 0) ddx -= k - 1;
        if (ddx > 0) ddx += k - 1;
        if (ddy < 0) ddy -= k - 1;
        if (ddy > 0) ddy += k - 1;
        sx = ddx / k;
        sy = ddy / k;
        if (gouraud) {
            sr = (int32_t)((((int32_t)b->r - a->r) * (1 << 12)) / k);
            sg = (int32_t)((((int32_t)b->g - a->g) * (1 << 12)) / k);
            sb = (int32_t)((((int32_t)b->b - a->b) * (1 << 12)) / k);
        }
    }
    int64_t cx = (int64_t)((uint64_t)(int64_t)a->x << 32) | (INT64_C(1) << 31);
    int64_t cy = (int64_t)((uint64_t)(int64_t)a->y << 32) | (INT64_C(1) << 31);
    cx -= 1024;
    if (sy < 0)
        cy -= 1024;
    uint32_t cr = ((uint32_t)a->r << 12) | (1u << 11);
    uint32_t cg = ((uint32_t)a->g << 12) | (1u << 11);
    uint32_t cb = ((uint32_t)a->b << 12) | (1u << 11);

    for (int i = 0; i <= k; i++) {
        int x = (int)((cx >> 32) & 2047), y = (int)((cy >> 32) & 2047);
        if (x >= gs.clip_l && x <= gs.clip_r && y >= gs.clip_t && y <= gs.clip_b)
            shade_pixel(p, x, y, (cr >> 12) & 0xff, (cg >> 12) & 0xff, (cb >> 12) & 0xff, 0, 0);
        cx += sx;
        cy += sy;
        cr += (uint32_t)sr;
        cg += (uint32_t)sg;
        cb += (uint32_t)sb;
    }
}

/* ---------------------------------------------------------------------------------------------
 * GP0 command handlers */

static void vtx_from_words(Vtx *v, uint32_t xy, uint32_t col)
{
    v->x = sext11(xy) + gs.ofs_x;
    v->y = sext11(xy >> 16) + gs.ofs_y;
    v->r = (uint8_t)col;
    v->g = (uint8_t)(col >> 8);
    v->b = (uint8_t)(col >> 16);
    v->u = v->v = 0;
}

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
    o->px = o->py = o->w = 0;
}

/* Precise positions and depths from the GTE (pc_gte.h) for a polygon's vertices, by the SXY words
 * in its packet. All or nothing: a polygon that isn't entirely GTE-projected (2D, or vertices
 * the game computed itself) is drawn exactly as before. */
static void gl_precise(PC_GlVertex *gv, const uint32_t *xy, int n)
{
    static int stats = -1;
    static uint32_t frame, hit, miss;
    float x[4], y[4], z[4];
    int ok = PC_GtePrecisePolygon(xy, n, x, y, z);
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

static void line_out(const Prim *p, int gouraud, const Vtx *a, const Vtx *b)
{
    if (PC_GlActive()) {
        PC_GlPrim gp;
        PC_GlVertex ga, gb;
        gl_prim(&gp, p);
        gl_vertex(&ga, a);
        gl_vertex(&gb, b);
        PC_GlLine(&gp, &ga, &gb);
        return;
    }
    draw_line(p, gouraud, a, b);
}

static int dither_on(void)
{
    return ((gs.drawmode >> 9) & 1) && !s_dither_force_off;
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
    int shaded = gouraud && !p.raw;
    p.dither = dither_on() && (shaded || (tex && !p.raw));
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
    if (PC_GlActive()) {
        PC_GlPrim gp;
        PC_GlVertex gv[4];
        gl_prim(&gp, &p);
        for (int i = 0; i < n; i++)
            gl_vertex(&gv[i], &v[i]);
        gl_precise(gv, xy, n);
        if (!tri_too_big(&v[0], &v[1], &v[2]))
            PC_GlTriangle(&gp, &gv[0], &gv[1], &gv[2]);
        if (quad && !tri_too_big(&v[1], &v[2], &v[3]))
            PC_GlTriangle(&gp, &gv[1], &gv[2], &gv[3]);
        return;
    }
    draw_triangle(&p, shaded, &v[0], &v[1], &v[2]);
    if (quad)
        draw_triangle(&p, shaded, &v[1], &v[2], &v[3]);
}

static void line_prim(Prim *p, uint32_t op)
{
    prim_from_state(p, 0, 0, (op & 2) != 0, 0);
    p->dither = dither_on() && (op & 0x10);
}

static void cmd_line(const uint32_t *w)
{
    uint32_t op = w[0] >> 24;
    int gouraud = (op & 0x10) != 0;
    Vtx a, b;
    Prim p;
    line_prim(&p, op);
    vtx_from_words(&a, w[1], w[0]);
    if (gouraud)
        vtx_from_words(&b, w[3], w[2]);
    else
        vtx_from_words(&b, w[2], w[0]);
    line_out(&p, gouraud, &a, &b);

    if (op & 8) {
        gs.mode = MODE_POLYLINE;
        gs.pl_op = op;
        gs.pl_x = b.x - gs.ofs_x;
        gs.pl_y = b.y - gs.ofs_y;
        gs.pl_col = gouraud ? w[2] : w[0];
        gs.pl_want_col = gouraud;
    }
}

static void polyline_word(uint32_t word)
{
    int gouraud = (gs.pl_op & 0x10) != 0;
    int vertex_start = gouraud ? gs.pl_want_col : 1;
    if (vertex_start && (word & 0xf000f000u) == 0x50005000u) {
        gs.mode = MODE_CMD;
        return;
    }
    if (gouraud && gs.pl_want_col) {
        gs.pl_newcol = word;
        gs.pl_want_col = 0;
        return;
    }
    Prim p;
    Vtx a, b;
    line_prim(&p, gs.pl_op);
    uint32_t newcol = gouraud ? gs.pl_newcol : gs.pl_col;
    a.x = gs.pl_x + gs.ofs_x;
    a.y = gs.pl_y + gs.ofs_y;
    a.r = (uint8_t)gs.pl_col;
    a.g = (uint8_t)(gs.pl_col >> 8);
    a.b = (uint8_t)(gs.pl_col >> 16);
    a.u = a.v = 0;
    vtx_from_words(&b, word, newcol);
    line_out(&p, gouraud, &a, &b);
    gs.pl_x = sext11(word);
    gs.pl_y = sext11(word >> 16);
    gs.pl_col = newcol;
    gs.pl_want_col = gouraud;
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

    if (PC_GlActive()) {
        PC_GlPrim gp;
        gl_prim(&gp, &p);
        PC_GlRect(&gp, x, y, width, height, (uint8_t)r, (uint8_t)g, (uint8_t)b, (int)u0, (int)v0,
                  flip_x, flip_y);
        return;
    }
    for (int oy = 0; oy < height; oy++) {
        int py = y + oy;
        if (py < gs.clip_t || py > gs.clip_b)
            continue;
        unsigned tv = (flip_y ? v0 - (unsigned)oy : v0 + (unsigned)oy) & 0xff;
        for (int ox = 0; ox < width; ox++) {
            int px = x + ox;
            if (px < gs.clip_l || px > gs.clip_r)
                continue;
            unsigned tu = (flip_x ? u0 - (unsigned)ox : u0 + (unsigned)ox) & 0xff;
            shade_pixel(&p, px, py, r, g, b, tu, tv);
        }
    }
}

static inline uint16_t rgb24_to_15(uint32_t c)
{
    return (uint16_t)(((c >> 3) & 0x1f) | (((c >> 11) & 0x1f) << 5) | (((c >> 19) & 0x1f) << 10));
}

static void cmd_fill(const uint32_t *w)
{
    uint16_t c = rgb24_to_15(w[0]);
    int x = (int)(w[1] & 0x3f0), y = (int)((w[1] >> 16) & 0x1ff);
    int width = (int)(((w[2] & 0x3ff) + 0xf) & ~0xfu), height = (int)((w[2] >> 16) & 0x1ff);
    if (PC_GlActive()) {
        PC_GlFill(x, y, width, height, w[0]);
        return;
    }
    for (int j = 0; j < height; j++)
        for (int i = 0; i < width; i++)
            VRAM(x + i, y + j) = c;
}

static void cmd_copy(const uint32_t *w)
{
    static uint16_t row[VRAM_W];
    int sx = (int)(w[1] & 0x3ff), sy = (int)((w[1] >> 16) & 0x3ff);
    int dx = (int)(w[2] & 0x3ff), dy = (int)((w[2] >> 16) & 0x3ff);
    int width = (int)(((w[3] & 0xffff) - 1) & 0x3ff) + 1;
    int height = (int)((((w[3] >> 16) & 0xffff) - 1) & 0x1ff) + 1;
    uint16_t or_mask = gs.mask_set ? 0x8000 : 0;
    PC_GlSyncRead(sx, sy, width, height);
    PC_GlBeforeWrite(dx, dy, width, height);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++)
            row[i] = VRAM(sx + i, sy + j);
        for (int i = 0; i < width; i++) {
            uint16_t *d = &VRAM(dx + i, dy + j);
            if (gs.mask_check && (*d & 0x8000))
                continue;
            *d = row[i] | or_mask;
        }
    }
    PC_GlWritten(dx, dy, width, height);
}

static void xfer_rect(const uint32_t *w, int *x, int *y, int *width, int *height)
{
    *x = (int)(w[1] & 0x3ff);
    *y = (int)((w[1] >> 16) & 0x1ff);
    *width = (int)(((w[2] & 0xffff) - 1) & 0x3ff) + 1;
    *height = (int)((((w[2] >> 16) & 0xffff) - 1) & 0x1ff) + 1;
}

static void cmd_to_vram(const uint32_t *w)
{
    xfer_rect(w, &gs.xfer_x, &gs.xfer_y, &gs.xfer_w, &gs.xfer_h);
    gs.xfer_n = gs.xfer_w * gs.xfer_h;
    gs.xfer_i = 0;
    gs.mode = MODE_TO_VRAM;
    PC_GlBeforeWrite(gs.xfer_x, gs.xfer_y, gs.xfer_w, gs.xfer_h);
}

/* A CPU->VRAM transfer ended (or was cut short): its pixels are newer than the GL copy. */
static void xfer_done(void)
{
    gs.mode = MODE_CMD;
    PC_GlWritten(gs.xfer_x, gs.xfer_y, gs.xfer_w, gs.xfer_h);
}

static inline void xfer_put(uint16_t px)
{
    if (gs.xfer_i >= gs.xfer_n)
        return;
    int i = gs.xfer_i++;
    uint16_t *d = &VRAM(gs.xfer_x + i % gs.xfer_w, gs.xfer_y + i / gs.xfer_w);
    if (gs.mask_check && (*d & 0x8000))
        return;
    *d = px | (gs.mask_set ? 0x8000 : 0);
}

static void cmd_from_vram(const uint32_t *w)
{
    xfer_rect(w, &gs.rd_x, &gs.rd_y, &gs.rd_w, &gs.rd_h);
    gs.rd_n = gs.rd_w * gs.rd_h;
    gs.rd_i = 0;
    PC_GlSyncRead(gs.rd_x, gs.rd_y, gs.rd_w, gs.rd_h);
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

/* A breakpoint target for debugging (gdb: break pc_gpu_suspicious). */
void PC_Backtrace(const char *why);
static uint32_t s_ring[64];
static unsigned s_ringpos;
void pc_gpu_suspicious(const uint32_t *w)
{
    static int once;
    (void)w;
    if (!once++ && getenv("THPS_GPUTRACE")) {
        PC_Backtrace("pixel data executed as a GPU command");
        for (unsigned i = 0; i < 64; i++)
            fprintf(stderr, "%08x%c", s_ring[(s_ringpos + i) & 63], (i & 7) == 7 ? 10 : 32);
    }
}

static void execute(const uint32_t *w)
{
    uint32_t op = w[0] >> 24;
    trace_cmd(w);
    if (w[0] == 0x80008000u)
        pc_gpu_suspicious(w);
    switch (op >> 5) {
    case 1: cmd_polygon(w); return;
    case 2: cmd_line(w); return;
    case 3: cmd_rect(w); return;
    case 4: cmd_copy(w); return;
    case 5: cmd_to_vram(w); return;
    case 6: cmd_from_vram(w); return;
    default: break;
    }
    switch (op) {
    case 0x02: cmd_fill(w); break;
    case 0x1f: gs.irq = 1; break;
    case 0xe1: set_drawmode(w[0]); break;
    case 0xe2: set_texwindow(w[0]); break;
    case 0xe3:
        gs.e3 = w[0] & 0xfffff;
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
        gs.e4 = w[0] & 0xfffff;
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
        gs.e5 = w[0] & 0x3fffff;
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

void PC_GpuWriteGP0(uint32_t word)
{
    s_ring[s_ringpos++ & 63] = word;
    if (gs.mode == MODE_TO_VRAM) {
        xfer_put((uint16_t)word);
        xfer_put((uint16_t)(word >> 16));
        if (gs.xfer_i >= gs.xfer_n)
            xfer_done();
        return;
    }
    if (gs.mode == MODE_POLYLINE) {
        polyline_word(word);
        return;
    }
    if (gs.fifo_n == 0) {
        init_tables();
        gs.fifo_need = gp0_size(word >> 24);
    }
    gs.fifo[gs.fifo_n++] = word;
    if (gs.fifo_n >= gs.fifo_need) {
        gs.fifo_n = 0;
        execute(gs.fifo);
    }
}

/* Debugging: describe an unfinished command/transfer, or return NULL when the GPU is idle. */
const char *PC_GpuPendingState(void)
{
    static char buf[160];
    if (gs.mode == MODE_TO_VRAM)
        snprintf(buf, sizeof buf, "VRAM load %d/%d halfwords", gs.xfer_i, gs.xfer_n);
    else if (gs.mode == MODE_POLYLINE)
        snprintf(buf, sizeof buf, "unterminated poly-line");
    else if (gs.fifo_n)
        snprintf(buf, sizeof buf, "command %08x has %d of %d words", gs.fifo[0], gs.fifo_n,
                 gs.fifo_need);
    else
        return NULL;
    return buf;
}

uint32_t PC_GpuReadData(void)
{
    if (gs.rd_i < gs.rd_n) {
        uint32_t v = 0;
        for (int h = 0; h < 2; h++) {
            uint16_t px = 0;
            if (gs.rd_i < gs.rd_n) {
                int i = gs.rd_i++;
                px = VRAM(gs.rd_x + i % gs.rd_w, gs.rd_y + i / gs.rd_w);
            }
            v |= (uint32_t)px << (16 * h);
        }
        gs.read_latch = v;
    }
    return gs.read_latch;
}

/* ---------------------------------------------------------------------------------------------
 * GP1 */

static void gp1_reset_cmdbuf(void)
{
    gs.fifo_n = 0;
    if (gs.mode == MODE_TO_VRAM)
        xfer_done();
    gs.mode = MODE_CMD;
    gs.xfer_n = gs.xfer_i = 0;
}

static void gp1_reset(void)
{
    gp1_reset_cmdbuf();
    gs.irq = 0;
    gs.disp_off = 1;
    gs.dma_dir = 0;
    gs.disp_x = gs.disp_y = 0;
    gs.h_start = 0x200;
    gs.h_end = 0xc00;
    gs.v_start = 0x10;
    gs.v_end = 0x100;
    gs.disp_mode = 0;
    set_drawmode(0);
    set_texwindow(0);
    gs.e3 = gs.e4 = gs.e5 = 0;
    gs.clip_l = gs.clip_t = gs.clip_r = gs.clip_b = 0;
    gs.ofs_x = gs.ofs_y = 0;
    gs.mask_set = gs.mask_check = 0;
    gs.rd_n = gs.rd_i = 0;
}

void PC_GpuWriteGP1(uint32_t word)
{
    uint32_t v = word & 0xffffff;
    switch ((word >> 24) & 0x3f) {
    case 0x00: gp1_reset(); break;
    case 0x01: gp1_reset_cmdbuf(); break;
    case 0x02: gs.irq = 0; break;
    case 0x03: gs.disp_off = (int)(v & 1); break;
    case 0x04: gs.dma_dir = (int)(v & 3); break;
    case 0x05:
        gs.disp_x = (int)(v & 0x3ff);
        gs.disp_y = (int)((v >> 10) & 0x1ff);
        if (gpudump_active())
            fprintf(stderr, "env f%u GP1(05) disp_x=%d disp_y=%d\n", pc_heartbeat, gs.disp_x,
                    gs.disp_y);
        break;
    case 0x06:
        gs.h_start = (int)(v & 0xfff);
        gs.h_end = (int)((v >> 12) & 0xfff);
        break;
    case 0x07:
        gs.v_start = (int)(v & 0x3ff);
        gs.v_end = (int)((v >> 10) & 0x3ff);
        break;
    case 0x08: gs.disp_mode = v & 0xff; break;
    case 0x10:
    case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
    case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
        switch (v & 7) {
        case 2: gs.read_latch = gs.e2; break;
        case 3: gs.read_latch = gs.e3; break;
        case 4: gs.read_latch = gs.e4; break;
        case 5: gs.read_latch = gs.e5; break;
        case 7: gs.read_latch = 2; break;
        default: break;
        }
        break;
    default:
        break;
    }
}

uint32_t PC_GpuReadStat(void)
{
    uint32_t s = 0;
    uint32_t m = gs.disp_mode;
    s |= gs.drawmode & 0x7ff;
    s |= ((gs.drawmode >> 11) & 1) << 15;
    s |= (uint32_t)gs.mask_set << 11;
    s |= (uint32_t)gs.mask_check << 12;
    s |= ((m >> 6) & 1) << 16;
    s |= (m & 3) << 17;
    s |= ((m >> 2) & 1) << 19;
    s |= ((m >> 3) & 1) << 20;
    s |= ((m >> 4) & 1) << 21;
    s |= ((m >> 5) & 1) << 22;
    s |= ((m >> 7) & 1) << 14;
    s |= (uint32_t)gs.disp_off << 23;
    s |= (uint32_t)gs.irq << 24;
    s |= 1u << 26;                                   /* ready for commands */
    s |= (uint32_t)(gs.rd_i < gs.rd_n) << 27;        /* ready to send VRAM */
    s |= 1u << 28;                                   /* ready for DMA blocks */
    s |= (uint32_t)gs.dma_dir << 29;
    switch (gs.dma_dir) {
    case 1:
    case 2: s |= 1u << 25; break;
    case 3: s |= (uint32_t)(gs.rd_i < gs.rd_n) << 25; break;
    default: break;
    }
    if ((m & 0x24) == 0x24)
        s |= (uint32_t)gs.field << 13;
    s |= (uint32_t)gs.field << 31;
    return s;
}

void PC_GpuVBlank(void)
{
    gs.field ^= 1;
}

/* ---------------------------------------------------------------------------------------------
 * DMA linked list */

static void draw_chain(const uint32_t *first);

void PC_GpuDrawChain(const uint32_t *first)
{
    draw_chain(first);
    R3d_ChainEnd();
}

int PC_GpuDrawState(int *ofs_x, int *ofs_y, int clip[4])
{
    *ofs_x = gs.ofs_x;
    *ofs_y = gs.ofs_y;
    clip[0] = gs.clip_l;
    clip[1] = gs.clip_t;
    clip[2] = gs.clip_r;
    clip[3] = gs.clip_b;
    return PC_GlActive() ? PC_GlWideClip(clip) : 0;
}

static void draw_chain(const uint32_t *first)
{
    static int warned, warned_len;
    const uint32_t *p = first, *slow = first;
    for (int guard = 0; guard < (1 << 20); guard++) {
        uint32_t tag = p[0];
        uint32_t len = tag >> 24;
        if (len > 16 && !warned_len) {
            warned_len = 1;
            fprintf(stderr, "[gpu] DrawChain: packet at %08x has %u words (tag %08x)\n",
                    (unsigned)(uintptr_t)p, len, tag);
        }
        /* the native 3D renderer's layer markers, and where its layers are in the OT */
        if (!R3d_ChainPacket(p, len))
            for (uint32_t i = 1; i <= len; i++)
                PC_GpuWriteGP0(p[i]);
        uint32_t next = tag & 0xffffff;
        if (next & 0x800000)
            return;
        p = (const uint32_t *)(uintptr_t)(0x80000000u | next);
        if (guard & 1) {
            slow = (const uint32_t *)(uintptr_t)(0x80000000u | (slow[0] & 0xffffff));
            if (slow == p) {
                if (!warned) {
                    warned = 1;
                    fprintf(stderr, "[gpu] DrawChain: cyclic ordering table at %08x\n",
                            (unsigned)(uintptr_t)p);
                }
                return;
            }
        }
    }
    if (!warned) {
        warned = 1;
        fprintf(stderr, "[gpu] DrawChain: iteration cap hit (cyclic ordering table?)\n");
    }
}

/* ---------------------------------------------------------------------------------------------
 * Misc */

void PC_GpuReset(void)
{
    init_tables();
    memset(s_vram, 0, sizeof s_vram);
    PC_GlWritten(0, 0, VRAM_W, VRAM_H);
    gp1_reset();
    gs.field = 0;
}

void PC_GpuFillRect(int x, int y, int w, int h, uint32_t rgb24)
{
    uint16_t c = rgb24_to_15(rgb24);
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > VRAM_W) x1 = VRAM_W;
    if (y1 > VRAM_H) y1 = VRAM_H;
    if (PC_GlActive()) {
        if (x1 > x && y1 > y)
            PC_GlFill(x, y, x1 - x, y1 - y, rgb24);
        return;
    }
    for (int j = y; j < y1; j++)
        for (int i = x; i < x1; i++)
            s_vram[j * VRAM_W + i] = c;
}

void PC_GpuGetDisplay(PC_GpuDisplay *d)
{
    static const int hres[4] = { 256, 320, 512, 640 };
    static const int div[4] = { 10, 8, 5, 4 };
    uint32_t m = gs.disp_mode;
    int w, dv;
    if (m & 0x40) {
        w = 368;
        dv = 7;
    } else {
        w = hres[m & 3];
        dv = div[m & 3];
    }
    if (gs.h_end > gs.h_start) {
        int rw = (((gs.h_end - gs.h_start) / dv) + 2) & ~3;
        if (rw > 0 && rw < w)
            w = rw;
    }
    d->pal = (m >> 3) & 1;
    d->interlaced = (m & 0x24) == 0x24;
    int lines = gs.v_end - gs.v_start;
    int maxl = d->pal ? 288 : 256;
    if (lines <= 0 || lines > maxl)
        lines = d->pal ? 256 : 240;
    d->x = gs.disp_x;
    d->y = gs.disp_y;
    d->w = w;
    d->h = d->interlaced ? lines * 2 : lines;
    d->rgb24 = (m >> 4) & 1;
    d->enabled = !gs.disp_off;
}

static inline uint32_t c15_to_argb(uint16_t c)
{
    uint32_t r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return 0xff000000u | (r << 16) | (g << 8) | b;
}

void PC_GpuDisplayToARGB(uint32_t *dst, int pitch, int maxw, int maxh, int *ow, int *oh)
{
    PC_GpuDisplay d;
    PC_GpuGetDisplay(&d);
    int w = d.w < maxw ? d.w : maxw, h = d.h < maxh ? d.h : maxh;
    PC_GlSyncRead(d.x, d.y, d.rgb24 ? (w * 3 + 1) / 2 : w, h);
    for (int y = 0; y < h; y++) {
        uint32_t *o = dst + (size_t)y * pitch;
        int vy = (d.y + y) & (VRAM_H - 1);
        if (d.rgb24) {
            const uint8_t *row = (const uint8_t *)&s_vram[vy * VRAM_W];
            for (int x = 0; x < w; x++) {
                int bo = (d.x * 2 + x * 3);
                uint8_t r = row[bo % (VRAM_W * 2)], g = row[(bo + 1) % (VRAM_W * 2)],
                        b = row[(bo + 2) % (VRAM_W * 2)];
                o[x] = 0xff000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            }
        } else {
            for (int x = 0; x < w; x++)
                o[x] = c15_to_argb(VRAM(d.x + x, vy));
        }
    }
    *ow = w;
    *oh = h;
}

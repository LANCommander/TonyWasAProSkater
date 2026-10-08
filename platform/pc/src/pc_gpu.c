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
    p->textured = textured;
    p->raw = textured && raw;
    p->semi = semi;
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

static inline void shade_pixel(const Prim *p, int x, int y, unsigned r, unsigned g, unsigned b,
                               unsigned u, unsigned v)
{
    uint16_t color;
    int dy = p->dither ? (y & 3) : 2, dx = p->dither ? (x & 3) : 3;
    const uint8_t *lut = s_dither_lut[dy][dx];

    if (p->textured) {
        uint16_t t = sample_texel(p, u & 0xff, v & 0xff);
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
    return (maxx - minx) >= 1024 || (maxy - miny) >= 512;
}

static void draw_triangle(const Prim *p, int gouraud, const Vtx *v0, const Vtx *v1, const Vtx *v2)
{
    const Vtx *t;
    unsigned cv;

    if (tri_too_big(v0, v1, v2))
        return;

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

    if (v0->y == v2->y)
        return;

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
    if (denom == 0)
        return;
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
    }
#undef CALCIS

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

    for (int i = 0; i < n; i++) {
        if (i > 0 && gouraud)
            col = w[k++];
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

    Prim p;
    prim_from_state(&p, tex, raw, semi, clut);
    int shaded = gouraud && !p.raw;
    p.dither = dither_on() && (shaded || (tex && !p.raw));
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
    draw_line(&p, gouraud, &a, &b);

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
    draw_line(&p, gouraud, &a, &b);
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

static void execute(const uint32_t *w)
{
    uint32_t op = w[0] >> 24;
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
        break;
    case 0xe4:
        gs.e4 = w[0] & 0xfffff;
        gs.clip_r = (int)(w[0] & 0x3ff);
        gs.clip_b = (int)((w[0] >> 10) & 0x1ff);
        break;
    case 0xe5:
        gs.e5 = w[0] & 0x3fffff;
        gs.ofs_x = sext11(w[0]);
        gs.ofs_y = sext11(w[0] >> 11);
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
    if (gs.mode == MODE_TO_VRAM) {
        xfer_put((uint16_t)word);
        xfer_put((uint16_t)(word >> 16));
        if (gs.xfer_i >= gs.xfer_n)
            gs.mode = MODE_CMD;
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

void PC_GpuDrawChain(const uint32_t *first)
{
    static int warned;
    const uint32_t *p = first;
    for (int guard = 0; guard < (1 << 20); guard++) {
        uint32_t tag = p[0];
        uint32_t len = tag >> 24;
        for (uint32_t i = 1; i <= len; i++)
            PC_GpuWriteGP0(p[i]);
        uint32_t next = tag & 0xffffff;
        if (next & 0x800000)
            return;
        p = (const uint32_t *)(uintptr_t)(0x80000000u | next);
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

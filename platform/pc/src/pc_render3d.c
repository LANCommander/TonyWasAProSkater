/* Native 3D renderer for the world and models; see pc_render3d.h for the outline.
 *
 * Geometry. The retail vertex transforms (M3DASM, as C in decomp/src/m3dasm) keep running: callers use their
 * results (outcodes, culling), and shadows and collision read the projected-vertex cache they
 * fill. Each one is wrapped so that its projections also hand over every vertex unrounded
 * (PC_CaptureBegin); those are stored in a float copy of the cache, slot for slot, following
 * the stitch vertices that skater parts share. Each slot remembers the screen word retail wrote
 * there, so a slot some other code has since overwritten is never used.
 *
 * Faces. ProcessPolys and ProcessPolysFast are replaced. The face walk keeps retail's decisions
 * (which faces are visible, their colours: the same colour maths with the same fog value,
 * the same ordering-table bucket) and drops what only the PS1 needed: screen-space subdivision,
 * near-plane clipping by subdivision (OpenGL clips), packets. Faces crossing the water surface of
 * an item that tests it are clipped against the plane in view space. Fade-to-colour mode (fog
 * toward a colour instead of the far colour) is drawn as retail draws it: the face additive over
 * a flat polygon of the fade colour; only its subtractive variant still runs the retail code,
 * whose packets are drawn as before. Dreamcast (version 6) faces keep 16-bit texture coordinates
 * and an HD texture (pc_dctex.h).
 *
 * Drawing. Each M3d_RenderSetup starts a batch and links a marker packet at the far end of its
 * ordering table layer. When PC_GpuDrawChain reaches the marker, the batch's faces are sorted by
 * bucket and drawn while the chain walks that layer: before each primitive of bucket b, the faces
 * of buckets >= b, so everything the GPU still draws keeps its place in the PS1's painter order.
 * The faces write a depth buffer; projected primitives in the layer are tested against it.
 *
 * Widescreen (pc_gpu_gl.h): the transforms' outcodes compare screen x with the viewport, so for a
 * widened (full-width) viewport they are worked out again against the widened one, and so is
 * their "all off-screen" result that callers skip whole models by.
 *
 * THPS_R3DSTATS=1 prints counts once a second, THPS_R3D_VERIFY=1 checks the float vertices against
 * the integers retail computed, THPS_R3D_NEARDROP=1 drops faces crossing the near plane that
 * retail dropped (those without the subdivision flag), THPS_R3D_RETAILFADE=1 leaves calls in
 * fade-to-colour mode to retail (A/B of the native version; not for Dreamcast models),
 * THPS_R3D_NCLIP=1 culls back faces with retail's integer screen-space test, THPS_R3D_DUMP=1 writes
 * the last layer's faces next to each THPS_SHOT (with THPS_DBG_FILL=bbggrr, pc_gpu_gl.c, painting
 * the screen clears, cracks in the level stand out). */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_dctex.h"
#include "pc_gpu.h"
#include "pc_gpu_gl.h"
#include "pc_geom.h"
#include "pc_precise.h"
#include "pc_render3d.h"
#include "pc_runtime.h"

extern volatile uint32_t pc_heartbeat;

typedef uint32_t (*Fn12)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                         uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
Fn12 PC_Impl(uint32_t addr); /* pc_overrides.c */
void PC_RenderTrace(const char *what, uint32_t ret);

#define M32(a) (*(volatile uint32_t *)PSX_PTR(a))
#define M16(a) (*(volatile uint16_t *)PSX_PTR(a))
#define S16(a) (*(volatile int16_t *)PSX_PTR(a))
#define SCR(o) (0x1f800000u + (uint32_t)(o))

/* game globals */
#define G_PPOLY        PSX_DATA_ADDR(0x800d1a54u)   /* pPoly */
#define G_POLYBUFEND   PSX_DATA_ADDR(0x800d177cu)   /* PolyBufferEnd */
#define G_PROJECTED    PSX_DATA_ADDR(0x800d2160u)   /* ProjectedVertices */
#define G_ROTATED      PSX_DATA_ADDR(0x800d2164u)   /* RotatedVertices */
#define G_FREESTITCH   PSX_DATA_ADDR(0x800d2198u)   /* FreeStitchVertex */
#define G_COLOURTABLE  PSX_DATA_ADDR(0x800d219cu)   /* pColourTable */
#define G_CURRENT_OT   PSX_DATA_ADDR(0x800d21a0u)   /* CurrentOrderingTable */
#define G_TESTTHEWATER PSX_DATA_ADDR(0x800d21b8u)   /* TestTheWater */
#define G_VIEWPORT     PSX_DATA_ADDR(0x800d2178u)   /* pCurrentViewport: xR, yB, xL, yT, Hither, Yon... */
#define G_XRES         PSX_DATA_ADDR(0x800d2188u)   /* Xres */

/* scratchpad words the renderer shares with its callers */
#define S_DPQMIN     0x284   /* fog starts here (view z) */
#define S_DPQSHIFT   0x294   /* fog = (z - DpqMin) << DpqShift (>> if negative) */
#define S_PUSHBACK   0x2b4   /* face flag 0x2000 adds this to the sort z */
#define S_PUSHBACK2  0x2c4   /* face flag 0x4000 adds this */
#define S_MIRROR     0x2d4   /* mirrored model: back faces are the front ones */
#define S_FADECOLOUR 0x354   /* >= 0: fade-to-colour mode */
#define S_REFMAPCLUT 0x364
#define S_REFMAPTPAGE 0x374
#define S_CONSTLIGHT 0x3e4   /* the item's tint, as a light (11:11:10) */
#define S_FACEFLAGS  0x3f4   /* GlobalFaceFlags: face flags & hi | lo */

#define MARKER0 0x00523344u  /* GP0 NOP "R3D" */
#define MARKER1 0x7233444eu

static int s_native, s_verify, s_stats, s_neardrop, s_retailfade, s_nclip;

static struct {
    uint32_t faces, culled, badtag, layers, draws, calls, water, fade, fb_fade, fb_nolayer;
    uint32_t water_checked, water_bad;
    uint32_t vchecked[8], vbad[8], sign_checked, sign_bad, near, far;} st;

/* ---------------------------------------------------------------------------------------------
 * Float vertices */

/* A float copy of a projected-vertex cache slot: the view-space position, and the screen word
 * retail stored in the slot when this was made. */
typedef struct {
    float x, y, z;
    uint32_t tag;
} Shadow;

#define PV_SLOTS 512   /* ProjectedVertices: 400 slots before RotatedVertices (+3200) */
#define SP_SLOTS 128   /* scratchpad cache of the fast paths (1 KB) */

static Shadow s_pv[PV_SLOTS], s_sp[SP_SLOTS];
static uint32_t s_pv_base;

static Shadow *shadow_at(uint32_t a)
{
    uint32_t d = a - SCR(0);
    if (d < SP_SLOTS * 8)
        return &s_sp[d >> 3];
    d = a - s_pv_base;
    if (s_pv_base && d < PV_SLOTS * 8 && !(d & 7))
        return &s_pv[d >> 3];
    return NULL;
}

static void set_shadow(uint32_t a, float x, float y, float z)
{
    Shadow *s = shadow_at(a);
    if (s) {
        s->x = x;
        s->y = y;
        s->z = z;
        s->tag = M32(a);
    }
}

enum { T_ITEM, T_SUPER, T_SUPER_DW, T_SFAST, T_SFAST_DW, T_FAST, T_BLOCK, T_BG, T_COUNT };
static const char *const s_kind_name[T_COUNT] = { "item", "super", "super-dw", "superfast",
                                                  "superfast-dw", "fast", "block", "bg" };
static int s_last_kind = -1;   /* the transform the next ProcessPolys draws from */
static float s_cap[1024][3];

/* THPS_R3D_VERIFY: the float vertex against what retail stored (view position or screen word). */
static void verify_slot(int kind, uint32_t slot, uint32_t rot)
{
    Shadow *s = shadow_at(slot);
    int bad = 0;
    if (!s || fabsf(s->x) > 30000 || fabsf(s->y) > 30000 || fabsf(s->z) > 30000)
        return;
    if (kind == T_SFAST || kind == T_SFAST_DW || kind == T_FAST) {
        float h = (float)pc_geom.h;
        float ofx = (float)pc_geom.ofx / 65536.0f;
        int sx = (int)((int32_t)(M32(slot) << 21) >> 21);
        int z = S16(slot + 6);
        if (fabsf(s->z - z) > 1.5f)
            bad = 1;
        else if (s->z > 64 && fabsf(ofx + h * s->x / s->z - sx) > 2.5f && sx > -1000 && sx < 1000)
            bad = 2;
    } else {
        uint32_t r = M32(rot);
        int rx = (int16_t)r, ry = (int16_t)(r >> 16), z = S16(slot + 4);
        if (fabsf(s->x - rx) > 1.5f || fabsf(s->y - ry) > 1.5f || fabsf(s->z - z) > 1.5f)
            bad = 3;
    }
    st.vchecked[kind]++;
    if (bad) {
        static int shown;
        st.vbad[kind]++;
        if (shown++ < 20)
            PC_Log("r3d verify %s slot %08x: float %.2f %.2f %.2f, retail %08x %08x (%d)",
                   s_kind_name[kind], slot, s->x, s->y, s->z, M32(slot), M32(slot + 4), bad);
    }
}

/* ---------------------------------------------------------------------------------------------
 * Widescreen */

/* The widescreen margin of the current viewport: only a full-width one (one player) has it. */
static int viewport_pad(int *xl, int *xr)
{
    uint32_t vp = M32(G_VIEWPORT);
    if (!pc_wide_pad || !vp)
        return 0;
    *xr = M16(vp);
    *xl = M16(vp + 4);
    return *xl == 0 && *xr == M16(G_XRES) ? pc_wide_pad : 0;
}

/* Item-format cache (screen word, then z and the outcode halfword): outcode bits 0 (right of xR)
 * and 1 (left of xL), mirrored when behind the camera, and their "not out" copies in bits 8, 9,
 * against the widened viewport. Returns the transform's result (the AND of the outcodes) to
 * match. */
static uint32_t widen_outcodes(uint32_t out, int n, uint32_t r, int pad, int xl, int xr)
{
    uint32_t all = 0xffff;
    xl -= pad;
    xr += pad;
    for (int i = 0; i < n; i++) {
        uint32_t e = out + (uint32_t)i * 8;
        int sx = S16(e), z = S16(e + 4);
        uint32_t b = (uint32_t)(sx > xr) | (uint32_t)(sx < xl) << 1;
        if (z < 0)
            b ^= 3;
        uint32_t c = (M16(e + 6) & ~0x0303u) | b | (b ^ 3) << 8;
        M16(e + 6) = (uint16_t)c;
        all &= c;
    }
    return (r & ~0x0303u) | (all & 0x0303u);
}

/* Fast-format cache (flags inside the screen word: bit 13 is x's sign, bit 14 "x >= xR"): the
 * result's x bits again, from the widened viewport. */
static int fast_x_out(uint32_t w, int pad, int xl, int xr)
{
    int sx = (int32_t)(w << 21) >> 21;
    return sx < xl - pad ? 1 : sx >= xr + pad ? 2 : 0;
}

static uint32_t widen_fast(uint32_t out, int n, uint32_t r, int pad, int xl, int xr)
{
    int all = 3;
    for (int i = 0; i < n; i++)
        all &= fast_x_out(M32(out + (uint32_t)i * 8), pad, xl, xr);
    return (r & ~0x6000u) | (all & 1 ? 0x2000u : 0) | (all & 2 ? 0x4000u : 0);
}

/* ---------------------------------------------------------------------------------------------
 * The entire level (R3d_FarBegin) */

static struct {
    int on;
    double m[3][3], t[3];
    int fogstart, fogshift;
} s_far;

void R3d_FarBegin(const double m[3][3], const double t[3], int fogstart, int fogshift)
{
    memcpy(s_far.m, m, sizeof s_far.m);
    memcpy(s_far.t, t, sizeof s_far.t);
    s_far.fogstart = fogstart;
    s_far.fogshift = fogshift;
    s_far.on = 1;
}

void R3d_FarEnd(void)
{
    s_far.on = 0;
}

static int clamp16(double v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : (int)lrint(v);
}

/* TransformAndOutcodeItemVertices in double precision: the same cache entries (screen word,
 * depth limited to 16 bits, outcodes against the widened viewport, no far plane) and rotated
 * x, y, and the float vertices. */
static uint32_t far_transform(uint32_t a0, int n)
{
    uint32_t pv = M32(G_PROJECTED), rot = M32(G_ROTATED), vp = M32(G_VIEWPORT);
    double h = (double)pc_geom.h;
    double ofx = pc_geom.ofx / 65536.0, ofy = pc_geom.ofy / 65536.0;
    int xr = M16(vp), yb = M16(vp + 2), xl = M16(vp + 4), yt = M16(vp + 6), hither = M16(vp + 8);
    int wxl, wxr, pad = viewport_pad(&wxl, &wxr);
    uint32_t all = 0xffff;

    if (n == 0)
        return 0xff;
    st.far++;
    xl -= pad;
    xr += pad;
    s_pv_base = pv;
    s_last_kind = T_ITEM;
    for (int i = 0; i < n && i < PV_SLOTS; i++) {
        uint32_t v = a0 + (uint32_t)i * 8, dst = pv + (uint32_t)i * 8;
        double in[3] = { S16(v), S16(v + 2), S16(v + 4) }, o[3];
        for (int r = 0; r < 3; r++)
            o[r] = s_far.m[r][0] * in[0] + s_far.m[r][1] * in[1] + s_far.m[r][2] * in[2] +
                   s_far.t[r];
        int sx, sy;
        if (o[2] >= 1) {
            sx = clamp16(ofx + h * o[0] / o[2], -1024, 1023);
            sy = clamp16(ofy + h * o[1] / o[2], -1024, 1023);
        } else {
            /* the projection's divide overflows: saturated, away from the axis */
            sx = o[0] > 0 ? 1023 : -1024;
            sy = o[1] > 0 ? 1023 : -1024;
        }
        uint32_t oc = (uint32_t)(sx > xr) | (uint32_t)(sx < xl) << 1 | (uint32_t)(sy > yb) << 2 |
                      (uint32_t)(sy < yt) << 3 | (uint32_t)(o[2] < hither) << 4;
        if (o[2] < 0)
            oc ^= 0xf;
        oc |= (oc ^ 0xff) << 8;
        M32(dst) = (uint32_t)(uint16_t)sx | (uint32_t)(uint16_t)sy << 16;
        M16(dst + 4) = (uint16_t)clamp16(o[2], -32768, 32767);
        M16(dst + 6) = (uint16_t)oc;
        M32(rot + (uint32_t)i * 8) = (uint32_t)(uint16_t)clamp16(o[0], -32768, 32767) |
                                     (uint32_t)(uint16_t)clamp16(o[1], -32768, 32767) << 16;
        set_shadow(dst, (float)o[0], (float)o[1], (float)o[2]);
        all &= oc;
    }
    M32(SCR(0x3d4)) = pv + (uint32_t)n * 8;   /* SCR_PROJECTED_END */
    return all;
}

/* Run a retail vertex transform and keep its vertices unrounded. a0 = vertices, a1 = count. */
static uint32_t transform(uint32_t addr, int kind, uint32_t a0, uint32_t a1, uint32_t a2,
                          uint32_t a3)
{
    static Fn12 fn[T_COUNT];
    if (!fn[kind])
        fn[kind] = PC_Impl(addr);
    if (!s_native)
        return fn[kind](a0, a1, a2, a3, 0, 0, 0, 0, 0, 0, 0, 0);
    if (s_far.on && kind == T_ITEM)
        return far_transform(a0, (int)a1);

    uint32_t pv = M32(G_PROJECTED), rot = M32(G_ROTATED), pool = M32(G_FREESTITCH);
    int fast = kind == T_SFAST || kind == T_SFAST_DW || kind == T_FAST;
    int super = kind == T_SUPER || kind == T_SUPER_DW || kind == T_SFAST || kind == T_SFAST_DW;
    float sc = (kind == T_SUPER_DW || kind == T_SFAST_DW) ? 1.0f / 16 : 1.0f;
    uint32_t out = fast ? SCR(0) : pv;
    int n = (int)a1;

    PC_CaptureBegin(s_cap, (int)(sizeof s_cap / sizeof s_cap[0]));
    PC_PreciseZScale(sc);   /* for primitives retail still draws from these */
    uint32_t r = fn[kind](a0, a1, a2, a3, 0, 0, 0, 0, 0, 0, 0, 0);
    PC_PreciseZScale(1);
    int nc = PC_CaptureEnd();
    s_pv_base = pv;
    s_last_kind = kind;

    for (int i = 0; i < n; i++) {
        uint32_t v = a0 + (uint32_t)i * 8, dst = out + (uint32_t)i * 8;
        if (kind == T_BLOCK && (M16(v + 6) & 0x10)) {
            /* always-facing (billboard) vertices from here on: an offset of the given size from
             * a centre vertex, perpendicular to the screen direction toward a second vertex, x
             * stretched by 10485/4096, at the centre's depth. Worked out again from the float
             * vertices, as retail's whole-pixel screen positions make billboards jitter. */
            float h = (float)pc_geom.h;
            float ofx = (float)pc_geom.ofx / 65536.0f;
            float ofy = (float)pc_geom.ofy / 65536.0f;
            for (; i < n; i++) {
                v = a0 + (uint32_t)i * 8;
                dst = out + (uint32_t)i * 8;
                Shadow *c = shadow_at(pv + (M32(v) & 0xffff));
                Shadow *t = shadow_at(pv + (M32(v) >> 16));
                if (c && t && c->z >= 1 && t->z >= 1) {
                    float dx = t->x / t->z - c->x / c->z, dy = c->y / c->z - t->y / t->z;
                    float len = sqrtf(dx * dx + dy * dy);
                    if (len > 1e-6f) {
                        float size = (float)(int16_t)M32(v + 4) / len;
                        set_shadow(dst, c->x + dy * size * (10485.0f / 4096), c->y + dx * size,
                                   c->z);
                        continue;
                    }
                }
                float z = c ? c->z : (float)S16(dst + 4);
                set_shadow(dst, ((int16_t)M16(dst) - ofx) * z / h, ((int16_t)M16(dst + 2) - ofy) * z / h,
                           z);
            }
            break;
        }
        if (super && (M32(v + 4) & 0x20000)) {
            /* stitch vertex: retail copied an earlier part's vertex from the pool */
            uint32_t src = (fast ? SCR(0x3f8) : pv + 3192) - M32(v);
            Shadow *s = shadow_at(src), *d = shadow_at(dst);
            if (s && d) {
                *d = *s;
                d->tag = M32(dst);
            }
            continue;
        }
        if (i >= nc)
            break;
        set_shadow(dst, s_cap[i][0] * sc, s_cap[i][1] * sc, s_cap[i][2] * sc);
        if (super && (M32(v + 4) & 0x10000)) {
            /* exported to the pool for later parts */
            set_shadow(pool, s_cap[i][0] * sc, s_cap[i][1] * sc, s_cap[i][2] * sc);
            pool -= 8;
        }
    }
    if (s_verify && kind != T_BG)
        for (int i = 0; i < n; i++) {
            if (kind == T_BLOCK && (M16(a0 + (uint32_t)i * 8 + 6) & 0x10))
                break;   /* billboards have no rotated vertex */
            verify_slot(kind, out + (uint32_t)i * 8, rot + (uint32_t)i * 8);
        }
    int xl, xr, pad = viewport_pad(&xl, &xr);
    if (pad && n > 0)
        r = fast ? widen_fast(out, n, r, pad, xl, xr) : widen_outcodes(out, n, r, pad, xl, xr);
    return r;
}

/* TransformAndOutcode...: item, super (+ dewibbled), super fast (+ dewibbled), fast, block,
 * background */
uint32_t hleov_80085d28(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    uint32_t r = transform(0x80085d28, T_ITEM, a0, a1, a2, a3);
    PC_RenderTrace("T", r);
    return r;
}
uint32_t hleov_80085e98(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return transform(0x80085e98, T_SUPER, a0, a1, a2, a3);
}
uint32_t hleov_800860cc(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return transform(0x800860cc, T_SUPER_DW, a0, a1, a2, a3);
}
uint32_t hleov_800862d4(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return transform(0x800862d4, T_SFAST, a0, a1, a2, a3);
}
uint32_t hleov_80086404(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return transform(0x80086404, T_SFAST_DW, a0, a1, a2, a3);
}
uint32_t hleov_80089790(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return transform(0x80089790, T_FAST, a0, a1, a2, a3);
}
uint32_t hleov_80086d2c(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return transform(0x80086d2c, T_BLOCK, a0, a1, a2, a3);
}
uint32_t hleov_80086590(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return transform(0x80086590, T_BG, a0, a1, a2, a3);
}

/* ---------------------------------------------------------------------------------------------
 * Batches */

typedef struct {
    uint16_t bucket;
    uint8_t cls, nv;
    uint32_t first;
    float zmin, zmax;   /* view depth range of its corners */
    uint16_t hd;        /* its HD texture (pc_dctex.h), 0 if none */
    uint32_t src;       /* the face's address in the model (THPS_R3D_DUMP) */
} Face;

typedef struct {
    uint32_t gen, ot;
    int valid, sorted, next;
    float h, ofx, ofy, hither;
    Face *f, *sf;
    int nf, capf;
    PC_GlVert *v, *sv;
    int nv, capv;
    uint32_t pages[256];
    int npages;
} Batch;

#define NBATCH 8
static Batch s_b[NBATCH];
static int s_cur = -1;
static uint32_t s_gen;

/* M3d_RenderSetup(camera, viewport, ot): the start of a 3D layer. */
void hleov_800831f8(uint32_t cam, uint32_t vp, uint32_t ot)
{
    extern void pcfn_800831f8(uint32_t, uint32_t, uint32_t);
    pcfn_800831f8(cam, vp, ot);
    if (!s_native)
        return;
    int slot = (s_cur + 1) % NBATCH;
    Batch *b = &s_b[slot];
    b->gen = ++s_gen & 0x0fffffff;
    b->ot = ot;
    b->valid = 1;
    b->sorted = 0;
    b->nf = b->nv = b->npages = 0;
    b->h = (float)pc_geom.h;
    b->ofx = (float)pc_geom.ofx / 65536.0f;
    b->ofy = (float)pc_geom.ofy / 65536.0f;
    b->hither = (float)M16(vp + 8);
    if (b->hither < 1)
        b->hither = 1;
    s_cur = slot;

    /* the marker: 3 words after the tag, at the far end of the layer (drawn before its faces) */
    uint32_t p = M32(G_PPOLY), end = M32(G_POLYBUFEND), head = ot + 4095 * 4;
    if (((p + 16) & 0xffffff) > (end & 0xffffff)) {
        b->valid = 0;
        return;
    }
    M32(p + 4) = MARKER0;
    M32(p + 8) = MARKER1;
    M32(p + 12) = (uint32_t)slot | b->gen << 4;
    M32(p) = 3u << 24 | (M32(head) & 0xffffff);
    M32(head) = (M32(head) & 0xff000000u) | (p & 0xffffff);
    M32(G_PPOLY) = p + 16;
}

static PC_GlVert *alloc_verts(Batch *b, int n)
{
    if (b->nv + n > b->capv) {
        b->capv = b->capv ? b->capv * 2 : 65536;
        b->v = realloc(b->v, (size_t)b->capv * sizeof *b->v);
        b->sv = realloc(b->sv, (size_t)b->capv * sizeof *b->sv);
    }
    PC_GlVert *v = &b->v[b->nv];
    b->nv += n;
    return v;
}

static uint32_t s_src;   /* the face being added */

static void add_face(Batch *b, int bucket, int cls, int nv, uint32_t first, uint16_t hd)
{
    float zmin = 1e30f, zmax = -1e30f;
    for (int i = 0; i < nv; i++) {
        float z = b->v[first + (uint32_t)i].z;
        zmin = z < zmin ? z : zmin;
        zmax = z > zmax ? z : zmax;
    }
    if (b->nf == b->capf) {
        b->capf = b->capf ? b->capf * 2 : 16384;
        b->f = realloc(b->f, (size_t)b->capf * sizeof *b->f);
        b->sf = realloc(b->sf, (size_t)b->capf * sizeof *b->sf);
    }
    Face *f = &b->f[b->nf++];
    f->bucket = (uint16_t)bucket;
    f->cls = (uint8_t)cls;
    f->nv = (uint8_t)nv;
    f->first = first;
    f->zmin = zmin;
    f->zmax = zmax;
    f->hd = hd;
    f->src = s_src;
}

static void add_page(Batch *b, uint16_t tpage, uint16_t clut)
{
    uint32_t key = (uint32_t)(tpage & 0x19f) << 16 | clut;
    for (int i = b->npages - 1; i >= 0; i--)
        if (b->pages[i] == key)
            return;
    if (b->npages < (int)(sizeof b->pages / sizeof b->pages[0]))
        b->pages[b->npages++] = key;
}

/* Faces by bucket, far to near; within a bucket the last added first (the PS1 links each new
 * primitive at the front of its bucket). */
static void sort_batch(Batch *b)
{
    static int count[4096];
    memset(count, 0, sizeof count);
    for (int i = 0; i < b->nf; i++)
        count[b->f[i].bucket]++;
    int pos = 0;
    for (int k = 4095; k >= 0; k--) {
        int c = count[k];
        count[k] = pos;
        pos += c;
    }
    for (int i = b->nf - 1; i >= 0; i--)
        b->sf[count[b->f[i].bucket]++] = b->f[i];
    uint32_t vi = 0;
    for (int i = 0; i < b->nf; i++) {
        Face *f = &b->sf[i];
        memcpy(&b->sv[vi], &b->v[f->first], f->nv * sizeof *b->sv);
        f->first = vi;
        vi += f->nv;
    }
    b->sorted = 1;
}

/* ---------------------------------------------------------------------------------------------
 * Faces (ProcessPolys) */

/* the fog (0..4096) towards the far colour the colours below get, as retail's colours */
static int16_t s_fog;

static uint32_t dpcs(uint32_t rgbc)
{
    uint32_t rgb;
    geom_dpcs(&pc_geom, rgbc & 0xffffff, (uint8_t)(rgbc >> 24), s_fog, 12, 0, &rgb, NULL);
    return rgb;
}

/* colour * light (11:11:10, 128 = 1.0 in each), then the fog */
static uint32_t dpcl(uint32_t rgbc, uint32_t light)
{
    int16_t l[3];
    uint32_t rgb;
    l[0] = (int16_t)(light << 5);
    l[1] = (int16_t)(light >> 6);
    l[2] = (int16_t)(light >> 17);
    geom_dpcl(&pc_geom, rgbc, l, s_fog, 12, 0, &rgb, NULL);
    return rgb;
}

static int32_t nclip(int x0, int y0, int x1, int y1, int x2, int y2)
{
    return x0 * y1 + x1 * y2 + x2 * y0 - x0 * y2 - x1 * y0 - x2 * y1;
}

static float det3(const Shadow *a, const Shadow *b, const Shadow *c)
{
    return a->x * (b->y * c->z - b->z * c->y) - a->y * (b->x * c->z - b->z * c->x) +
           a->z * (b->x * c->y - b->y * c->x);
}

/* A face corner before it goes to the batch: position, colour, texture coordinates, and its
 * distance below the water (when the call tests the water). */
typedef struct {
    float x, y, z, r, g, b, u, v, d;
} Corner;

static void corner(Corner *o, const Shadow *s, uint32_t rgb, float u, float v)
{
    o->x = s->x;
    o->y = s->y;
    o->z = s->z;
    o->r = (float)(rgb & 0xff);
    o->g = (float)((rgb >> 8) & 0xff);
    o->b = (float)((rgb >> 16) & 0xff);
    o->u = u;
    o->v = v;
    o->d = 0;
}

/* A Dreamcast face (version 6 file, pc_dctex.h): word 4 is its Texture, and u0..u3, v0..v3 are
 * 16-bit (words 5-8, 512 = one repeat). Sets the clut and tpage words and the corners'
 * coordinates: for an HD texture (*hd = its handle) in 256ths of a repeat, for a PS1 one in its
 * texels (without tiling). 0 if the texture has no image: the face is drawn untextured. */
static int dc_face(uint32_t a0, uint32_t tex, uint32_t f, uint32_t *uvw, uint16_t *hd, float *uf,
                   float *vf)
{
    uint16_t clut = M16(tex + 2), tpage = M16(tex + 6);
    int ru = M16(a0 + 20), rv = M16(a0 + 28);
    for (int i = 0; i < 4; i++) {
        /* from corner 0, so the 16-bit values wrapping around (scrolling) don't tear a face */
        uf[i] = (float)((ru & 511) + (int16_t)(M16(a0 + 20 + 2 * i) - ru)) * 0.5f;
        vf[i] = (float)((rv & 511) + (int16_t)(M16(a0 + 28 + 2 * i) - rv)) * 0.5f;
    }
    if (PC_HD_IS_TPAGE(tpage)) {
        if (!clut)
            return 0;
        *hd = clut;
    } else {
        const uint8_t *t = PSX_PTR(tex);
        for (int i = 0; i < 4; i++) {
            float u = uf[i] / 256, v = vf[i] / 256;
            u = u < 0 ? 0 : u > 1 ? 1 : u;
            v = v < 0 ? 0 : v > 1 ? 1 : v;
            uf[i] = t[0] + u * (t[10] - t[0]);
            vf[i] = t[1] + v * (t[11] - t[1]);
        }
    }
    uvw[0] = (uint32_t)clut << 16;
    uvw[1] = (uint32_t)(tpage | (f & 0x180) >> 2) << 16;
    return 1;
}

static void put_vert(PC_GlVert *o, const Corner *c, uint16_t clut, uint16_t info, const uint8_t *tw,
                     const uint8_t *uvr)
{
    o->x = c->x;
    o->y = c->y;
    o->z = c->z;
    o->r = (uint8_t)(c->r + 0.5f);
    o->g = (uint8_t)(c->g + 0.5f);
    o->b = (uint8_t)(c->b + 0.5f);
    o->a = 0;
    o->u = c->u;
    o->v = c->v;
    o->clut = clut;
    o->info = info;
    memcpy(o->tw, tw, 4);
    memcpy(o->uvr, uvr, 4);
}

/* The water plane of a call made with TestTheWater, as FUN_80086500 tests it: the camera's world
 * y axis (pCurrentCamera->Transform row 1) and the water level relative to the camera
 * (WaterNormal.pad). d = how far below the water a view-space point is; WaterClipValue says which
 * side is drawn (0xffff the part above, 0 the part below). */
#define G_CAMERA     PSX_DATA_ADDR(0x800d217cu)   /* pCurrentCamera */
#define G_WATERNORM  PSX_DATA_ADDR(0x800d2180u)   /* WaterNormal */
#define G_WATERCLIP  PSX_DATA_ADDR(0x800d2140u)   /* WaterClipValue */

typedef struct {
    float m0, m1, m2, level;
    int keep_below;
} Water;

static float water_depth(const Water *w, float x, float y, float z)
{
    return (w->m0 * x + w->m1 * y + w->m2 * z) / 4096.0f - w->level;
}

/* Sutherland-Hodgman: the part of polygon in[0..n) that is drawn, into out; its corner count. */
static int water_clip(const Corner *in, const int *outside, int n, Corner *out)
{
    int m = 0;
    for (int i = 0; i < n; i++) {
        const Corner *a = &in[i], *b = &in[(i + 1) % n];
        int ao = outside[i], bo = outside[(i + 1) % n];
        if (!ao)
            out[m++] = *a;
        if (ao != bo) {
            /* where the edge meets the water; retail's flags decide the sides, so a corner
             * very close to the surface may sit on the "wrong" side by a hair */
            float t = a->d - b->d != 0 ? a->d / (a->d - b->d) : 0.5f;
            t = t < 0 ? 0 : t > 1 ? 1 : t;
            Corner *c = &out[m++];
            const float *pa = &a->x, *pb = &b->x;
            float *pc = &c->x;
            for (int k = 0; k < (int)(sizeof(Corner) / sizeof(float)); k++)
                pc[k] = pa[k] + (pb[k] - pa[k]) * t;
        }
    }
    return m;
}

/* THPS_R3D_PROBE="frame,x,y": log the faces whose retail screen box contains x,y (debugging). */
static void probe(const uint32_t *sl, int fast, uint32_t f, const char *what, uint32_t a, uint32_t c,
                  uint32_t z)
{
    static int on = -1, pf, px, py;
    if (on < 0) {
        const char *e = getenv("THPS_R3D_PROBE");
        on = e && sscanf(e, "%d,%d,%d", &pf, &px, &py) == 3;
    }
    if (!on || (int)pc_heartbeat != pf)
        return;
    int x0 = 99999, x1 = -99999, y0 = 99999, y1 = -99999;
    for (int i = 0; i < 4; i++) {
        uint32_t w = M32(sl[i]);
        int x = fast ? (int32_t)(w << 21) >> 21 : (int16_t)w;
        int y = fast ? (int32_t)(w << 5) >> 21 : (int16_t)(w >> 16);
        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
    }
    if (px < x0 || px > x1 || py < y0 || py > y1)
        return;
    PC_Log("r3d probe %s: flags %04x fast %d box %d,%d-%d,%d a %x col %06x z %d", what, f & 0xffff,
           fast, x0, y0, x1, y1, a, c & 0xffffff, (int)z);}

/* The face walk of ProcessPolys (fast = ProcessPolysFast), into batch b; water: the call tests
 * the water (NULL if not). */
static void faces(Batch *b, uint32_t face, int n, int fast, const Water *water)
{
    uint32_t cache = fast ? SCR(0) : M32(G_PROJECTED);
    uint32_t gff = fast ? 0xffff0000u : M32(SCR(S_FACEFLAGS));
    int mirror = M16(SCR(S_MIRROR)) != 0;
    int dpqmin = S16(SCR(S_DPQMIN)), dpqshift = S16(SCR(S_DPQSHIFT));
    int push1 = S16(SCR(S_PUSHBACK)), push2 = S16(SCR(S_PUSHBACK2));
    uint32_t ctab = M32(G_COLOURTABLE);
    int sky = !fast && s_last_kind == T_BG;
    int32_t fade = (int32_t)M32(SCR(S_FADECOLOUR));
    int wxl = 0, wxr = 0, wpad = fast ? viewport_pad(&wxl, &wxr) : 0;

    for (int k = 0; k < n; k++) {
        uint32_t w0 = M32(face);
        uint32_t f = (w0 & (uint32_t)((int32_t)gff >> 16)) | (gff & 0xffff);
        uint32_t a0 = face;
        face += f >> 16;
        s_src = a0;
        if (!(f & 0xc0))
            continue;
        uint32_t w1 = M32(a0 + 4);
        int tri = (f & 0x10) != 0, nvtx = tri ? 3 : 4;
        uint32_t idx[4] = { w1 & 0xff, (w1 >> 8) & 0xff, (w1 >> 16) & 0xff, w1 >> 24 };
        uint32_t sl[4];
        for (int i = 0; i < 4; i++)
            sl[i] = cache + idx[i] * 8;

        /* outcodes, as retail; bit 7 (all out) and 15 (not all in) are the water's */
        int near = 0, wet = 0;
        if (!fast) {
            uint32_t t7 = ((M32(sl[0] + 4) & M32(sl[1] + 4) & M32(sl[2] + 4) & M32(sl[3] + 4)) >> 16) ^
                          0xff00;
            if (t7 & 0x20ff) {
                probe(sl, fast, f, "outcode", t7, 0, 0);
                continue;
            }
            near = (t7 & 0x5000) != 0;
            if (t7 & 0x8000) {
                if (!water)
                    continue;   /* not set without the test */
                wet = 1;
            }
        } else {
            uint32_t t7 = M32(sl[0]) & M32(sl[1]) & M32(sl[2]) & M32(sl[3]);
            if (wpad) {
                /* widescreen: x against the widened viewport (y and z as retail) */
                int xo = 3;
                for (int i = 0; i < nvtx; i++)
                    xo &= fast_x_out(M32(sl[i]), wpad, wxl, wxr);
                if ((int32_t)t7 >= 0 || (t7 & 0x60008000u) || xo)
                    continue;
            } else if ((int32_t)t7 >= 0 || (t7 & 0x6000e000u)) {
                continue;
            }
        }

        const Shadow *sh[4];
        int ok = 1;
        for (int i = 0; i < 4 && ok; i++) {
            sh[i] = shadow_at(sl[i]);
            ok = sh[i] && sh[i]->tag == M32(sl[i]);
        }
        if (!ok) {
            st.badtag++;
            continue;
        }

        /* back faces: in view space, from the unrounded vertices. Retail tests the integer screen
         * positions (NCLIP), which gets faces seen almost edge-on wrong: less than a pixel high
         * on the PS1, but at higher resolutions the ones it drops leave cracks in the level
         * (Chicago's floor along the walls). THPS_R3D_NCLIP=1 uses retail's test. */
        int visible;
        float d1 = det3(sh[0], sh[1], sh[2]), d2 = tri ? 0 : det3(sh[1], sh[3], sh[2]);
        if (near) {
            st.near++;
            if (!(f & 0x1000) && s_neardrop)
                continue;
            /* retail's near-plane path tests v0, v1, v2 in view space and doesn't apply the
             * mirror flag; a quad's other half counts too here */
            visible = d1 >= 0 || d2 > 0;
        } else {
            if (mirror) {
                d1 = -d1;
                d2 = -d2;
            }
            visible = d1 > 0 || d2 > 0;
            if (s_nclip || s_verify) {
                int x[4], y[4];
                for (int i = 0; i < 4; i++) {
                    uint32_t s = M32(sl[i]);
                    if (fast) {
                        x[i] = (int32_t)(s << 21) >> 21;
                        y[i] = (int32_t)(s << 5) >> 21;
                    } else {
                        x[i] = (int16_t)s;
                        y[i] = (int16_t)(s >> 16);
                    }
                }
                int32_t n1 = nclip(x[0], y[0], x[1], y[1], x[2], y[2]);
                int32_t n2 = tri ? 0 : nclip(x[1], y[1], x[2], y[2], x[3], y[3]);
                if (mirror) {
                    n1 = -n1;
                    n2 = -n2;
                }
                if (s_verify && (n1 > 64 || n1 < -64)) {
                    /* the two tests must agree where the screen one is reliable */
                    st.sign_checked++;
                    if ((d1 > 0) != (n1 > 0))
                        st.sign_bad++;
                }
                if (s_nclip)
                    visible = n1 > 0 || n2 < 0;
            }
        }
        if (!visible) {
            st.culled++;
            probe(sl, fast, f, near ? "near-back" : "back", 0, 0, 0);
            continue;
        }

        /* sort z, fog */
        int zmax = -32768;
        for (int i = 0; i < 4; i++) {
            int z = fast ? S16(sl[i] + 6) : S16(sl[i] + 4);
            if (z > zmax)
                zmax = z;
        }
        int d = zmax - dpqmin, fog = 0;
        if (s_far.on) {
            /* the true depth: beyond 16 bits, and no far plane, so the fog stops at full */
            float fz = sh[0]->z;
            for (int i = 1; i < nvtx; i++)
                if (sh[i]->z > fz)
                    fz = sh[i]->z;
            d = s_far.fogstart == INT32_MAX ? 0 : (int)fz - s_far.fogstart;
            if (d > 0) {
                int64_t f = s_far.fogshift < 0 ? (int64_t)d >> -s_far.fogshift
                                               : (int64_t)d << s_far.fogshift;
                fog = f > 4096 ? 4096 : (int)f;
            }
        } else if (d > 0) {
            fog = dpqshift < 0 ? d >> -dpqshift : d << dpqshift;
        }
        s_fog = (int16_t)fog;
        /* fade-to-colour mode (the front end): beyond the fog distance a face is added onto a
         * flat polygon of the fade colour, which takes the share the fog would (below) */
        if (fade >= 0 && d > 0) {
            f |= 0x1c0;
            f ^= 0x100;
            st.fade++;
        }

        /* colours: the colour maths retail uses */
        uint32_t c = M32(a0 + 8), col[4];
        int gouraud = 0;
#define TABLE(i) M32(ctab + ((c >> (8 * (i))) & 0xff) * 4)
#define VLIGHT(i) M32(cache + idx[i] * 8 + 3204)
        if (fast) {
            if (!(f & 0x800)) {
                col[0] = dpcs(c);
            } else {
                gouraud = 1;
                for (int i = 0; i < nvtx; i++)
                    col[i] = dpcs(TABLE(i));
            }
        } else if (!(f & 0x800)) {
            if (f & 4) {
                if (f & 8) {
                    gouraud = 1;
                    for (int i = 0; i < nvtx; i++)
                        col[i] = dpcl(c, VLIGHT(i));
                } else {
                    col[0] = dpcl(c, M32(cache + M16(a0 + 12) + 3204));
                }
            } else if (f & 8) {
                col[0] = dpcl(c, M32(SCR(S_CONSTLIGHT)));
            } else {
                col[0] = dpcs(c);
            }
        } else {
            gouraud = 1;
            uint32_t lf = (f & 4) ? M32(cache + M16(a0 + 12) + 3204) : M32(SCR(S_CONSTLIGHT));
            for (int i = 0; i < nvtx; i++) {
                if ((f & 4) && (f & 8))
                    col[i] = dpcl(TABLE(i), VLIGHT(i));
                else if (f & 0xc)
                    col[i] = dpcl(TABLE(i), lf);
                else
                    col[i] = dpcs(TABLE(i));
            }
        }
#undef TABLE

        /* texture */
        int tex = f & 1, env = !fast && !tex && !gouraud && (f & 2);
        uint32_t uvw[3] = { 0, 0, 0 };
        uint16_t hd = 0;
        int dc = 0;            /* a Dreamcast face: coordinates in uf, vf */
        float uf[4], vf[4];
        if (tex && (M32(a0 + 16) & 0x80000000u)) {
            dc = 1;
            tex = dc_face(a0, M32(a0 + 16), f, uvw, &hd, uf, vf);
        } else if (tex) {
            uvw[0] = M32(a0 + 16);
            uvw[1] = M32(a0 + 20) | (f & 0x180) << 14;
            uvw[2] = M32(a0 + 24);
        } else if (env) {
            uvw[0] = VLIGHT(0) | M32(SCR(S_REFMAPCLUT));
            uvw[1] = VLIGHT(1) | M32(SCR(S_REFMAPTPAGE)) | (f & 0x180) << 14;
            uvw[2] = VLIGHT(2) | (tri ? 0 : VLIGHT(3) << 16);
        }
#undef VLIGHT
        int textured = tex || env;
        uint16_t clut = (uint16_t)(uvw[0] >> 16), tpage = (uint16_t)(uvw[1] >> 16);
        int abr = textured ? (tpage >> 5) & 3 : (int)(f >> 7) & 3;
        int semi = (f & 0x40) != 0;
        uint16_t info = PC_GL_I_VIEW | (uint16_t)(abr << 5);
        if (hd) {
            info |= PC_HD_TPAGE | PC_GL_I_TEXTURED;
        } else if (textured) {
            int depth = (tpage >> 7) & 3;
            info |= (uint16_t)((tpage & 15) | ((tpage >> 4) & 1) << 4 | (depth > 2 ? 2 : depth) << 7 |
                               PC_GL_I_TEXTURED);
            add_page(b, tpage, clut);
        }
        if (semi)
            info |= PC_GL_I_SEMI;
        if (sky)
            info |= PC_GL_I_SKY;
        uint8_t tw[4] = { 0xff, 0xff, 0, 0 };
        if ((f & 0x20) && !dc) {
            uint32_t w = M32(a0 + 28);
            uint32_t mx = w & 31, my = (w >> 5) & 31, ox = (w >> 10) & 31, oy = (w >> 15) & 31;
            tw[0] = (uint8_t)~(mx * 8);
            tw[1] = (uint8_t)~(my * 8);
            tw[2] = (uint8_t)((ox & mx) * 8);
            tw[3] = (uint8_t)((oy & my) * 8);
        }
        if (!dc) {
            uint32_t uv[4] = { uvw[0], uvw[1], uvw[2], uvw[2] >> 16 };
            for (int i = 0; i < 4; i++) {
                uf[i] = (float)(uv[i] & 0xff);
                vf[i] = (float)((uv[i] >> 8) & 0xff);
            }
        }
        /* the face's texels: at higher resolutions and with exact interpolation, its edges would
         * otherwise pick up the texels next to them (dark seams between skater parts). HD
         * textures are filtered and may tile: no limit. */
        uint8_t uvr[4] = { 0, 0, 255, 255 };
        if (textured && !hd) {
            uvr[0] = uvr[1] = 255;
            uvr[2] = uvr[3] = 0;
            for (int i = 0; i < nvtx; i++) {
                uint8_t u = (uint8_t)uf[i], v = (uint8_t)vf[i];
                if (u < uvr[0]) uvr[0] = u;
                if (v < uvr[1]) uvr[1] = v;
                if (u > uvr[2]) uvr[2] = u;
                if (v > uvr[3]) uvr[3] = v;
            }
        }

        /* bucket */
        int z = zmax;
        if (f & 0x6000) {
            z += (f & 0x2000) ? push1 : push2;
            if (z < 0)
                z = 0;
        }
        int bucket = (uint32_t)z < 0x4000 ? (z & 0xfffc) >> 2 : 4095;

        Corner cn[4];
        for (int i = 0; i < nvtx; i++)
            corner(&cn[i], sh[i], gouraud ? col[i] : col[0], uf[i], vf[i]);
        uint32_t first = (uint32_t)b->nv;
        int nv;
        if (!wet) {
            static const int order[2][6] = { { 0, 1, 2 }, { 0, 1, 2, 1, 3, 2 } };
            nv = tri ? 3 : 6;
            PC_GlVert *o = alloc_verts(b, nv);
            for (int i = 0; i < nv; i++)
                put_vert(&o[i], &cn[order[!tri][i]], clut, info, tw, uvr);
        } else {
            /* crosses the water: only the part on the drawn side (FUN_80086500 flagged the
             * corners on the other one), as a fan around the clipped outline */
            static const int ring[2][4] = { { 0, 1, 2 }, { 0, 1, 3, 2 } };
            Corner in[4], out[8];
            int outside[4];
            for (int i = 0; i < nvtx; i++) {
                int vi = ring[!tri][i];
                in[i] = cn[vi];
                in[i].d = water_depth(water, in[i].x, in[i].y, in[i].z);
                outside[i] = (M32(sl[vi] + 4) >> 16 & 0x80) != 0;
                if (s_verify && fabsf(in[i].d) >= 1) {
                    st.water_checked++;
                    if (outside[i] != ((in[i].d >= 0) != water->keep_below))
                        st.water_bad++;
                }
            }
            int m = water_clip(in, outside, nvtx, out);
            if (m < 3)
                continue;
            nv = (m - 2) * 3;
            PC_GlVert *o = alloc_verts(b, nv);
            for (int i = 0; i < m - 2; i++) {
                put_vert(&o[i * 3], &out[0], clut, info, tw, uvr);
                put_vert(&o[i * 3 + 1], &out[i + 1], clut, info, tw, uvr);
                put_vert(&o[i * 3 + 2], &out[i + 2], clut, info, tw, uvr);
            }
            st.water++;
        }
        add_face(b, bucket, semi ? (abr == 2 ? 2 : 1) : 0, nv, first, hd);
        st.faces++;
        if (fade >= 0 && fog) {
            /* fade-to-colour: the flat polygon under the face (added after it, so drawn first,
             * as retail links it) in the fade colour with the fog's share, DPCS(fade, 1 - fog) */
            static const uint8_t tw0[4] = { 0xff, 0xff, 0, 0 }, uvr0[4] = { 0, 0, 255, 255 };
            s_fog = (int16_t)(4096 - fog);
            uint32_t oc = dpcs((uint32_t)fade), ofirst = (uint32_t)b->nv;
            PC_GlVert *o = alloc_verts(b, nv);
            for (int i = 0; i < nv; i++) {
                Corner c;
                const PC_GlVert *src = &b->v[first + (uint32_t)i];
                memset(&c, 0, sizeof c);
                c.x = src->x;
                c.y = src->y;
                c.z = src->z;
                c.r = (float)(oc & 0xff);
                c.g = (float)((oc >> 8) & 0xff);
                c.b = (float)((oc >> 16) & 0xff);
                put_vert(&o[i], &c, 0, (uint16_t)(PC_GL_I_VIEW | (sky ? PC_GL_I_SKY : 0)), tw0,
                         uvr0);
            }
            add_face(b, bucket, 0, nv, ofirst, 0);
        }
        probe(sl, fast, f, near ? "near" : "drawn", (uint32_t)bucket, col[0], (uint32_t)zmax);
    }
}

/* ProcessPolys(faces, normals, count) and ProcessPolysFast */
static uint32_t process(uint32_t addr, int fast, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    static Fn12 fn[2];
    if (!fn[fast])
        fn[fast] = PC_Impl(addr);
    if (!fast)
        PC_RenderTrace("P", 0);
    if (!s_native)
        return fn[fast](a0, a1, a2, a3, 0, 0, 0, 0, 0, 0, 0, 0);
    st.calls++;
    Batch *b = s_cur >= 0 ? &s_b[s_cur] : NULL;
    if (!b || !b->valid || b->ot != M32(G_CURRENT_OT)) {
        st.fb_nolayer++;
        return fn[fast](a0, a1, a2, a3, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    Water water, *wp = NULL;
    if (!fast && M32(G_TESTTHEWATER)) {
        uint32_t cam = M32(G_CAMERA);
        water.m0 = (float)S16(cam + 0x3a);   /* Transform.m[1][0..2] */
        water.m1 = (float)S16(cam + 0x3c);
        water.m2 = (float)S16(cam + 0x3e);
        water.level = (float)S16(G_WATERNORM + 6);
        water.keep_below = (M32(G_WATERCLIP) & 0xffff) == 0;
        wp = &water;
    }
    if ((int32_t)M32(SCR(S_FADECOLOUR)) >= 0 &&
        (s_retailfade || (M32(SCR(S_FADECOLOUR)) & 0x02000000u))) {
        /* fade-to-colour with a subtractive polygon: retail's code */
        st.fb_fade++;
        return fn[fast](a0, a1, a2, a3, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    if (!fast) {
        /* what retail leaves behind */
        geom_load_vector(pc_geom.tr, 0, 0, 0);
        pc_geom.dqb = 0x10000000;
        pc_geom.dqa = 0;
        M32(SCR(964)) = 0x80088790u;
        M16(SCR(676)) = 0;
    }
    faces(b, a0, (int)a2, fast, wp);
    return 0;
}

uint32_t hleov_80086f40(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return process(0x80086f40, 0, a0, a1, a2, a3);
}

uint32_t hleov_800898ac(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return process(0x800898ac, 1, a0, a1, a2, a3);
}

/* CompactOTagR(ot, n): relinks the ordering table around its empty entries, which would hide
 * where each bucket starts; the native layers need every entry visited. */
uint32_t hleov_80089ddc(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    static Fn12 f;
    if (!f)
        f = PC_Impl(0x80089ddc);
    if (s_native)
        return 0;
    return f(a0, a1, a2, a3, 0, 0, 0, 0, 0, 0, 0, 0);
}

/* ---------------------------------------------------------------------------------------------
 * Drawing, while PC_GpuDrawChain walks the ordering table */

static Batch *s_layer;
static uint32_t s_bucket;

/* Faces go to the GPU in the PS1's painter order, in draws of one class. Within a draw, colour is
 * tested against the depth the earlier draws wrote, not against the faces of the same draw, so a
 * face joins the draw only if no opaque face already in it can be in front of it by more than
 * the decal tolerance (the shader tests colour at z - (z/256 + 2)); otherwise a new draw starts
 * and the face is depth-tested against them. Order alone would repeat the PS1's sorting errors
 * (objects showing through walls and floors whose far corner is further away). */
static void draw_upto(uint32_t bucket)
{
    Batch *b = s_layer;
    int i = b->next;
    if (i < b->nf && b->sf[i].bucket >= bucket) {
        /* the drawing area and offset as the GPU has them here */
        int ox, oy, clip[4], wide = PC_GpuDrawState(&ox, &oy, clip);
        PC_GlLayerArea(ox, oy, clip, wide);
    }
    while (i < b->nf && b->sf[i].bucket >= bucket) {
        int cls = b->sf[i].cls, first = (int)b->sf[i].first, count = 0, j = i;
        uint16_t hd = b->sf[i].hd;
        float front = 1e30f;   /* nearest z of the opaque faces in this draw */
        while (j < b->nf && b->sf[j].bucket >= bucket && b->sf[j].cls == cls &&
               b->sf[j].hd == hd) {
            const Face *f = &b->sf[j];
            if (front < f->zmax - (f->zmax / 256 + 2))
                break;
            if (cls == 0 && f->zmin < front)
                front = f->zmin;
            count += f->nv;
            j++;
        }
        PC_GlLayerDraw(first, count, cls, hd);
        st.draws++;
        i = j;
    }
    b->next = i;
}

static void layer_end(void)
{
    draw_upto(0);
    PC_GlLayerEnd();
    s_layer = NULL;
}

static Batch *s_last_drawn;
static int s_last_ofs[2];

/* THPS_R3D_DUMP=1: with each THPS_SHOT, the faces of the layer drawn last, as text (debugging):
 * a header line "h ofx ofy hither ofs_x ofs_y", then per face "F bucket cls src nv" and its
 * vertices "x y z u v rgb". */
void R3d_DumpLast(const char *path)
{
    Batch *b = s_last_drawn;
    FILE *fp;
    if (!b || !b->sorted || !(fp = fopen(path, "w")))
        return;
    fprintf(fp, "%g %g %g %g %d %d\n", b->h, b->ofx, b->ofy, b->hither, s_last_ofs[0],
            s_last_ofs[1]);
    for (int i = 0; i < b->nf; i++) {
        const Face *f = &b->sf[i];
        fprintf(fp, "F %d %d %08x %d\n", f->bucket, f->cls, f->src, f->nv);
        for (int k = 0; k < f->nv; k++) {
            const PC_GlVert *v = &b->sv[f->first + (uint32_t)k];
            fprintf(fp, "%.4f %.4f %.4f %g %g %02x%02x%02x\n", v->x, v->y, v->z, v->u, v->v, v->r,
                    v->g, v->b);
        }
    }
    fclose(fp);
}

static void layer_start(uint32_t id)
{
    Batch *b = &s_b[id & 15];
    if (s_layer)
        layer_end();
    if ((id & 15) >= NBATCH || !b->valid || b->gen != id >> 4)
        return;
    s_last_drawn = b;
    if (!b->sorted)
        sort_batch(b);
    for (int i = 0; i < b->npages; i++)
        PC_GlLayerSyncTexture((uint16_t)(b->pages[i] >> 16), (uint16_t)b->pages[i]);
    PC_GlLayerParams lp;
    int clip[4];
    lp.wide = PC_GpuDrawState(&lp.ofs_x, &lp.ofs_y, clip);
    lp.clip_l = clip[0];
    lp.clip_t = clip[1];
    lp.clip_r = clip[2];
    lp.clip_b = clip[3];
    lp.h = b->h;
    lp.ofx = b->ofx;
    lp.ofy = b->ofy;
    lp.hither = b->hither;
    s_last_ofs[0] = lp.ofs_x;
    s_last_ofs[1] = lp.ofs_y;
    PC_GlLayerBegin(&lp, b->sv, b->nv);
    b->next = 0;
    s_layer = b;
    s_bucket = 4095;
    st.layers++;
}

int R3d_ChainPacket(const uint32_t *p, uint32_t len)
{
    if (!s_native)
        return 0;
    if (len == 3 && p[1] == MARKER0 && p[2] == MARKER1) {
        layer_start(p[3]);
        return 1;
    }
    if (!s_layer)
        return 0;
    uint32_t a = (uint32_t)(uintptr_t)p, ot = s_layer->ot;
    if (a - ot < 4096 * 4) {
        s_bucket = (a - ot) >> 2;   /* an entry of the layer: its primitives follow */
        return 0;
    }
    if (len == 0) {
        /* the entries before (OT, OT2D) and after (the other player's layer) end it */
        if (ot - a <= 8 || a - (ot + 4096 * 4) < 4096 * 4)
            layer_end();
        return 0;
    }
    draw_upto(s_bucket);
    return 0;
}

static void stats_tick(void);

void R3d_ChainEnd(void)
{
    if (s_layer)
        layer_end();
    stats_tick();
}

/* ---------------------------------------------------------------------------------------------
 * Setup */

static void stats_tick(void)
{
    static uint32_t frame;
    if (!s_stats || pc_heartbeat - frame < 60)
        return;
    frame = pc_heartbeat;
    fprintf(stderr, "[r3d] %u faces (%u back faces culled, %u stale vertices, %u clipped by water) "
                    "in %u calls, %u layers, %u draws, %u faded; retail: %u fade, %u no layer\n",
            st.faces, st.culled, st.badtag, st.water, st.calls, st.layers, st.draws, st.fade, st.fb_fade,
            st.fb_nolayer);
    if (st.far)
        fprintf(stderr, "[r3d] %u models beyond the 16-bit view (entire level)\n", st.far);
    if (s_verify)
        fprintf(stderr, "[r3d] near-plane faces: %u\n", st.near);
    if (s_verify && st.water_checked)
        fprintf(stderr, "[r3d] verify water sides: %u of %u corners disagree\n", st.water_bad,
                st.water_checked);
    if (s_verify && st.sign_checked)
        fprintf(stderr, "[r3d] verify back faces: %u of %u disagree\n", st.sign_bad, st.sign_checked);
    if (s_verify)
        for (int k = 0; k < T_COUNT; k++)
            if (st.vchecked[k])
                fprintf(stderr, "[r3d] verify %s: %u of %u vertices off\n", s_kind_name[k],
                        st.vbad[k], st.vchecked[k]);
    memset(&st, 0, sizeof st);
}

int R3d_Native(void)
{
    return s_native;
}

void R3d_Init(int gl)
{
    const char *e = getenv("THPS_RENDER3D");
    s_native = gl && !(e && strcmp(e, "psx") == 0);
    s_stats = getenv("THPS_R3DSTATS") && atoi(getenv("THPS_R3DSTATS"));
    s_verify = getenv("THPS_R3D_VERIFY") && atoi(getenv("THPS_R3D_VERIFY"));
    s_neardrop = getenv("THPS_R3D_NEARDROP") && atoi(getenv("THPS_R3D_NEARDROP"));
    s_retailfade = getenv("THPS_R3D_RETAILFADE") && atoi(getenv("THPS_R3D_RETAILFADE"));
    s_nclip = getenv("THPS_R3D_NCLIP") && atoi(getenv("THPS_R3D_NCLIP"));
    if (s_native)
        PC_PreciseEnable(1);
    fprintf(stderr, "[r3d] 3D: %s\n", s_native ? "native" : "PS1 (THPS_RENDER3D=psx)");
}

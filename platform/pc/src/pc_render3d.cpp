/* Native 3D renderer for the world and models; see pc_render3d.h for the outline.
 *
 * Geometry. The retail vertex transforms (M3DASM, as C in game/src/m3dasm) keep running: callers use their
 * results (outcodes, culling), and shadows and collision read the projected-vertex cache they
 * fill. Each one runs through R3d_Transform so that its projections also hand
 * over every vertex unrounded (PreciseVertices::CaptureBegin); those are stored in a float copy of the cache, slot for slot, following
 * the stitch vertices that skater parts share. Each slot remembers the screen word retail wrote
 * there, so a slot some other code has since overwritten is never used.
 *
 * Faces. ProcessPolys and ProcessPolysFast hand their faces to R3d_ProcessPolys. The face walk keeps retail's decisions
 * (which faces are visible, their colours: the same colour maths with the same fog value,
 * the same ordering-table bucket) and drops what only the PS1 needed: screen-space subdivision,
 * near-plane clipping by subdivision (OpenGL clips), packets. Faces crossing the water surface of
 * an item that tests it are clipped against the plane in view space. Fade-to-colour mode (fog
 * toward a colour instead of the far colour) is drawn as retail draws it: the face additive over
 * a flat polygon of the fade colour; only its subtractive variant still runs the retail code,
 * whose packets are drawn as before. Dreamcast (version 6) faces keep 16-bit texture coordinates
 * and an HD texture (pc_dctex.h).
 *
 * Drawing. Each M3d_RenderSetup starts a batch (R3d_RenderSetup) and links a marker packet at the far end of its
 * ordering table layer. When PC_DrawOT reaches the marker, the batch's faces are sorted by
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
 * the last layer's faces next to each THPS_SHOT (with THPS_DBG_FILL=bbggrr, pc_gpu_gl.cpp, painting
 * the screen clears, cracks in the level stand out). */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <libgpu.h>

#include "pc_dctex.h"
#include "pc_draw.h"
#include "pc_geom.h"
#include "pc_render3d.h"
#include "pc_runtime.h"
#include "pc/precise.h"
#include "pc/render3d.h"
#include "pc/draw.h"
#include "pc/gl_gpu.h"
#include "pc/runtime.h"

extern volatile uint32_t pc_heartbeat;


/* the game's memory, by address: its vertex caches, faces and the scratchpad */
typedef uintptr_t Addr;
#define M32(a) (*(volatile uint32_t *)(Addr)(a))
#define M16(a) (*(volatile uint16_t *)(Addr)(a))
#define S16(a) (*(volatile int16_t *)(Addr)(a))
#define MPTR(a) (*(volatile Addr *)(Addr)(a))
extern unsigned char Scratchpad[]; /* the game's scratchpad (pc_main.cpp) */
extern uint32_t ClippedFaceBuffer[]; /* ProcessPolys' water-clipped faces (game/src/m3dasm) */
#define SCR(o) ((Addr)Scratchpad + (Addr)(o))
/* a pointer the game keeps in the scratchpad (game/scratchpad.h SCRATCH_PTR) */
extern uintptr_t ScratchPointers[];
#define SCR_PTR(o) (((volatile uintptr_t *)ScratchPointers)[(o) >> 2])

/* game globals (game/data) */
extern unsigned char *pPoly;           /* the packet buffer's next free byte */
extern unsigned char *PolyBufferEnd;
extern void *ProjectedVertices;        /* SProjectedVertex[]: the vertex cache */
extern void *RotatedVertices;          /* SRotatedVertex[] */
extern void *FreeStitchVertex;         /* the stitch pool's next slot */
extern unsigned int *pColourTable;
extern P_TAG *CurrentOrderingTable;
extern int32_t TestTheWater[1];
extern void *pCurrentViewport;         /* SViewport: xR, yB, xL, yT, Hither, Yon... */
extern uint32_t Xres[1];

/* The water plane of a call made with TestTheWater, as FUN_80086500 tests it: the camera's world
 * y axis (pCurrentCamera->Transform row 1) and the water level relative to the camera
 * (WaterNormal.pad). d = how far below the water a view-space point is; WaterClipValue says which
 * side is drawn (0xffff the part above, 0 the part below). */
extern void *pCurrentCamera;   /* SCamera */
extern int32_t WaterNormal[2];  /* SVECTOR */
extern int32_t WaterClipValue[1];

namespace pc {

Renderer3D g_renderer3d;

namespace {

/* scratchpad words the renderer shares with its callers */
constexpr Addr S_DPQMIN     = 0x284;   /* fog starts here (view z) */
constexpr Addr S_DPQSHIFT   = 0x294;   /* fog = (z - DpqMin) << DpqShift (>> if negative) */
constexpr Addr S_PUSHBACK   = 0x2b4;   /* face flag 0x2000 adds this to the sort z */
constexpr Addr S_PUSHBACK2  = 0x2c4;   /* face flag 0x4000 adds this */
constexpr Addr S_MIRROR     = 0x2d4;   /* mirrored model: back faces are the front ones */
constexpr Addr S_FADECOLOUR = 0x354;   /* >= 0: fade-to-colour mode */
constexpr Addr S_REFMAPCLUT = 0x364;
constexpr Addr S_REFMAPTPAGE = 0x374;
constexpr Addr S_CONSTLIGHT = 0x3e4;   /* the item's tint, as a light (11:11:10) */
constexpr Addr S_FACEFLAGS  = 0x3f4;   /* GlobalFaceFlags: face flags & hi | lo */

constexpr uint32_t MARKER0 = 0x00523344u;  /* GP0 NOP "R3D" */
constexpr uint32_t MARKER1 = 0x7233444eu;

enum {
    T_ITEM = R3D_T_ITEM, T_SUPER = R3D_T_SUPER, T_SUPER_DW = R3D_T_SUPER_DW, T_SFAST = R3D_T_SFAST,
    T_SFAST_DW = R3D_T_SFAST_DW, T_FAST = R3D_T_FAST, T_BLOCK = R3D_T_BLOCK, T_BG = R3D_T_BG,
    T_COUNT = R3D_T_COUNT
};
const char *const s_kind_name[T_COUNT] = { "item", "super", "super-dw", "superfast",
                                           "superfast-dw", "fast", "block", "bg" };

/* -------------------------------------------------------------------------------------------
 * Widescreen */

/* The widescreen margin of the current viewport: only a full-width one (one player) has it. */
int ViewportPad(int *xl, int *xr)
{
    Addr vp = (Addr)pCurrentViewport;
    if (!pc_wide_pad || !vp)
        return 0;
    *xr = M16(vp);
    *xl = M16(vp + 4);
    return *xl == 0 && *xr == (uint16_t)Xres[0] ? pc_wide_pad : 0;
}

/* Item-format cache (screen word, then z and the outcode halfword): outcode bits 0 (right of xR)
 * and 1 (left of xL), mirrored when behind the camera, and their "not out" copies in bits 8, 9,
 * against the widened viewport. Returns the transform's result (the AND of the outcodes) to
 * match. */
uint32_t WidenOutcodes(Addr out, int n, uint32_t r, int pad, int xl, int xr)
{
    uint32_t all = 0xffff;
    xl -= pad;
    xr += pad;
    for (int i = 0; i < n; i++) {
        Addr e = out + (Addr)i * 8;
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
int FastXOut(uint32_t w, int pad, int xl, int xr)
{
    int sx = (int32_t)(w << 21) >> 21;
    return sx < xl - pad ? 1 : sx >= xr + pad ? 2 : 0;
}

uint32_t WidenFast(Addr out, int n, uint32_t r, int pad, int xl, int xr)
{
    int all = 3;
    for (int i = 0; i < n; i++)
        all &= FastXOut(M32(out + (Addr)i * 8), pad, xl, xr);
    return (r & ~0x6000u) | (all & 1 ? 0x2000u : 0) | (all & 2 ? 0x4000u : 0);
}

int Clamp16(double v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : (int)lrint(v);
}

/* -------------------------------------------------------------------------------------------
 * Face helpers */

int32_t Nclip(int x0, int y0, int x1, int y1, int x2, int y2)
{
    return x0 * y1 + x1 * y2 + x2 * y0 - x0 * y2 - x1 * y0 - x2 * y1;
}

template <class S>
float Det3(const S *a, const S *b, const S *c)
{
    return a->x * (b->y * c->z - b->z * c->y) - a->y * (b->x * c->z - b->z * c->x) +
           a->z * (b->x * c->y - b->y * c->x);
}

/* A face corner before it goes to the batch: position, colour, texture coordinates, and its
 * distance below the water (when the call tests the water). */
struct Corner {
    float x, y, z, r, g, b, u, v, d;

    template <class S>
    void Set(const S *s, uint32_t rgb, float u, float v)
    {
        this->x = s->x;
        this->y = s->y;
        this->z = s->z;
        this->r = (float)(rgb & 0xff);
        this->g = (float)((rgb >> 8) & 0xff);
        this->b = (float)((rgb >> 16) & 0xff);
        this->u = u;
        this->v = v;
        this->d = 0;
    }
};

/* A Dreamcast face (version 6 file, pc_dctex.h): word 4 is its Texture, and u0..u3, v0..v3 are
 * 16-bit (words 5-8, 512 = one repeat). Sets the clut and tpage words and the corners'
 * coordinates: for an HD texture (*hd = its handle) in 256ths of a repeat, for a PS1 one in its
 * texels (without tiling). 0 if the texture has no image: the face is drawn untextured. */
int DcFace(Addr a0, Addr tex, uint32_t f, uint32_t *uvw, uint16_t *hd, float *uf, float *vf)
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
        const uint8_t *t = (const uint8_t *)tex;
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

void PutVert(GlVert *o, const Corner *c, uint16_t clut, uint16_t info, const uint8_t *tw,
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

/* Sutherland-Hodgman: the part of polygon in[0..n) that is drawn, into out; its corner count. */
int WaterClip(const Corner *in, const int *outside, int n, Corner *out)
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

} // namespace

/* The water plane of a call made with TestTheWater (see the game globals above). */
struct Renderer3D::Water {
    float m0, m1, m2, level;
    int keep_below;

    /* how far below the water a view-space point is */
    float Depth(float x, float y, float z) const
    {
        return (m0 * x + m1 * y + m2 * z) / 4096.0f - level;
    }
};

/* ---------------------------------------------------------------------------------------------
 * Float vertices */

Renderer3D::Shadow *Renderer3D::ShadowAt(Addr a)
{
    Addr d = a - SCR(0);
    if (d < kSpSlots * 8)
        return &m_sp[d >> 3];
    d = a - m_pvBase;
    if (m_pvBase && d < kPvSlots * 8 && !(d & 7))
        return &m_pv[d >> 3];
    return nullptr;
}

void Renderer3D::SetShadow(Addr a, float x, float y, float z)
{
    Shadow *s = ShadowAt(a);
    if (s) {
        s->x = x;
        s->y = y;
        s->z = z;
        s->tag = M32(a);
    }
}

/* THPS_R3D_VERIFY: the float vertex against what retail stored (view position or screen word). */
void Renderer3D::VerifySlot(int kind, Addr slot, Addr rot)
{
    Shadow *s = ShadowAt(slot);
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
    m_st.vchecked[kind]++;
    if (bad) {
        m_st.vbad[kind]++;
        if (m_verifyShown++ < 20)
            PC_Log("r3d verify %s slot %08x: float %.2f %.2f %.2f, retail %08x %08x (%d)",
                   s_kind_name[kind], (uint32_t)slot, s->x, s->y, s->z, M32(slot), M32(slot + 4),
                   bad);
    }
}

/* ---------------------------------------------------------------------------------------------
 * The entire level (R3d_FarBegin) */

void Renderer3D::FarBegin(const double m[3][3], const double t[3], int fogstart, int fogshift)
{
    memcpy(m_far.m, m, sizeof m_far.m);
    memcpy(m_far.t, t, sizeof m_far.t);
    m_far.fogstart = fogstart;
    m_far.fogshift = fogshift;
    m_far.on = 1;
}

void Renderer3D::FarEnd()
{
    m_far.on = 0;
}

/* TransformAndOutcodeItemVertices in double precision: the same cache entries (screen word,
 * depth limited to 16 bits, outcodes against the widened viewport, no far plane) and rotated
 * x, y, and the float vertices. */
uint32_t Renderer3D::FarTransform(Addr a0, int n)
{
    Addr pv = (Addr)ProjectedVertices, rot = (Addr)RotatedVertices, vp = (Addr)pCurrentViewport;
    double h = (double)pc_geom.h;
    double ofx = pc_geom.ofx / 65536.0, ofy = pc_geom.ofy / 65536.0;
    int xr = M16(vp), yb = M16(vp + 2), xl = M16(vp + 4), yt = M16(vp + 6), hither = M16(vp + 8);
    int wxl, wxr, pad = ViewportPad(&wxl, &wxr);
    uint32_t all = 0xffff;

    if (n == 0)
        return 0xff;
    m_st.far++;
    xl -= pad;
    xr += pad;
    m_pvBase = pv;
    m_lastKind = T_ITEM;
    for (int i = 0; i < n && i < kPvSlots; i++) {
        Addr v = a0 + (Addr)i * 8, dst = pv + (Addr)i * 8;
        double in[3] = { (double)S16(v), (double)S16(v + 2), (double)S16(v + 4) }, o[3];
        for (int r = 0; r < 3; r++)
            o[r] = m_far.m[r][0] * in[0] + m_far.m[r][1] * in[1] + m_far.m[r][2] * in[2] +
                   m_far.t[r];
        int sx, sy;
        if (o[2] >= 1) {
            sx = Clamp16(ofx + h * o[0] / o[2], -1024, 1023);
            sy = Clamp16(ofy + h * o[1] / o[2], -1024, 1023);
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
        M16(dst + 4) = (uint16_t)Clamp16(o[2], -32768, 32767);
        M16(dst + 6) = (uint16_t)oc;
        M32(rot + (Addr)i * 8) = (uint32_t)(uint16_t)Clamp16(o[0], -32768, 32767) |
                                     (uint32_t)(uint16_t)Clamp16(o[1], -32768, 32767) << 16;
        SetShadow(dst, (float)o[0], (float)o[1], (float)o[2]);
        all &= oc;
    }
    SCR_PTR(0x3d4) = pv + (Addr)n * 8;   /* SCR_PROJECTED_END */
    return all;
}

/* Run a vertex transform and keep its vertices unrounded. */
uint32_t Renderer3D::TransformVertices(int kind, R3dTransformFn fn, void *verts, int count, void *o)
{
    Addr a0 = (Addr)verts;
    if (!m_native)
        return (uint32_t)fn(verts, count, o);
    if (m_far.on && kind == T_ITEM)
        return FarTransform(a0, count);

    Addr pv = (Addr)ProjectedVertices, rot = (Addr)RotatedVertices, pool = (Addr)FreeStitchVertex;
    int fast = kind == T_SFAST || kind == T_SFAST_DW || kind == T_FAST;
    int super = kind == T_SUPER || kind == T_SUPER_DW || kind == T_SFAST || kind == T_SFAST_DW;
    float sc = (kind == T_SUPER_DW || kind == T_SFAST_DW) ? 1.0f / 16 : 1.0f;
    Addr out = fast ? SCR(0) : pv;
    int n = count;

    g_precise.CaptureBegin(m_cap, (int)(sizeof m_cap / sizeof m_cap[0]));
    g_precise.ZScale(sc);   /* for primitives retail still draws from these */
    uint32_t r = (uint32_t)fn(verts, count, o);
    g_precise.ZScale(1);
    int nc = g_precise.CaptureEnd();
    m_pvBase = pv;
    m_lastKind = kind;

    for (int i = 0; i < n; i++) {
        Addr v = a0 + (Addr)i * 8, dst = out + (Addr)i * 8;
        if (kind == T_BLOCK && (M16(v + 6) & 0x10)) {
            /* always-facing (billboard) vertices from here on: an offset of the given size from
             * a centre vertex, perpendicular to the screen direction toward a second vertex, x
             * stretched by 10485/4096, at the centre's depth. Worked out again from the float
             * vertices, as retail's whole-pixel screen positions make billboards jitter. */
            float h = (float)pc_geom.h;
            float ofx = (float)pc_geom.ofx / 65536.0f;
            float ofy = (float)pc_geom.ofy / 65536.0f;
            for (; i < n; i++) {
                v = a0 + (Addr)i * 8;
                dst = out + (Addr)i * 8;
                Shadow *c = ShadowAt(pv + (M32(v) & 0xffff));
                Shadow *t = ShadowAt(pv + (M32(v) >> 16));
                if (c && t && c->z >= 1 && t->z >= 1) {
                    float dx = t->x / t->z - c->x / c->z, dy = c->y / c->z - t->y / t->z;
                    float len = sqrtf(dx * dx + dy * dy);
                    if (len > 1e-6f) {
                        float size = (float)(int16_t)M32(v + 4) / len;
                        SetShadow(dst, c->x + dy * size * (10485.0f / 4096), c->y + dx * size,
                                  c->z);
                        continue;
                    }
                }
                float z = c ? c->z : (float)S16(dst + 4);
                SetShadow(dst, ((int16_t)M16(dst) - ofx) * z / h, ((int16_t)M16(dst + 2) - ofy) * z / h,
                          z);
            }
            break;
        }
        if (super && (M32(v + 4) & 0x20000)) {
            /* stitch vertex: retail copied an earlier part's vertex from the pool */
            Addr src = (fast ? SCR(0x3f8) : pv + 3192) - M32(v);
            Shadow *s = ShadowAt(src), *d = ShadowAt(dst);
            if (s && d) {
                *d = *s;
                d->tag = M32(dst);
            }
            continue;
        }
        if (i >= nc)
            break;
        SetShadow(dst, m_cap[i][0] * sc, m_cap[i][1] * sc, m_cap[i][2] * sc);
        if (super && (M32(v + 4) & 0x10000)) {
            /* exported to the pool for later parts */
            SetShadow(pool, m_cap[i][0] * sc, m_cap[i][1] * sc, m_cap[i][2] * sc);
            pool -= 8;
        }
    }
    if (m_verify && kind != T_BG)
        for (int i = 0; i < n; i++) {
            if (kind == T_BLOCK && (M16(a0 + (Addr)i * 8 + 6) & 0x10))
                break;   /* billboards have no rotated vertex */
            VerifySlot(kind, out + (Addr)i * 8, rot + (Addr)i * 8);
        }
    int xl, xr, pad = ViewportPad(&xl, &xr);
    if (pad && n > 0)
        r = fast ? WidenFast(out, n, r, pad, xl, xr) : WidenOutcodes(out, n, r, pad, xl, xr);
    return r;
}

int Renderer3D::Transform(int kind, R3dTransformFn fn, void *v, int n, void *o)
{
    uint32_t r = TransformVertices(kind, fn, v, n, o);
    if (kind == T_ITEM)
        g_renderTrace.Count("T", r);
    return (int)r;
}

/* ---------------------------------------------------------------------------------------------
 * Batches */

void Renderer3D::RenderSetup(const void *viewport, P_TAG *ot)
{
    Addr vp = (Addr)viewport;
    if (!m_native)
        return;
    int slot = (m_cur + 1) % kBatches;
    Batch *b = &m_b[slot];
    b->gen = ++m_gen & 0x0fffffff;
    b->ot = ot;
    b->valid = 1;
    b->sorted = 0;
    b->nf = b->nv = 0;
    b->h = (float)pc_geom.h;
    b->ofx = (float)pc_geom.ofx / 65536.0f;
    b->ofy = (float)pc_geom.ofy / 65536.0f;
    b->hither = (float)M16(vp + 8);
    if (b->hither < 1)
        b->hither = 1;
    m_cur = slot;

    /* the marker: a packet of 3 words at the far end of the layer (drawn before its faces), from
     * the game's packet buffer */
    P_TAG *m = (P_TAG *)pPoly, *head = &ot[4095];
    uint8_t *end = PolyBufferEnd;
    if ((uint8_t *)(m + 1) + 12 > end) {
        b->valid = 0;
        return;
    }
    P_WORDS(m)[0] = MARKER0;
    P_WORDS(m)[1] = MARKER1;
    P_WORDS(m)[2] = (uint32_t)slot | b->gen << 4;
    m->len = 3;
    addPrim(head, m);
    pPoly = (uint8_t *)(m + 1) + 12;
}

GlVert *Renderer3D::Batch::AllocVerts(int n)
{
    if (nv + n > capv) {
        capv = capv ? capv * 2 : 65536;
        v = (GlVert *)realloc(v, (size_t)capv * sizeof *v);
        sv = (GlVert *)realloc(sv, (size_t)capv * sizeof *sv);
    }
    GlVert *o = &v[nv];
    nv += n;
    return o;
}

/* src: the face's address in the model (THPS_R3D_DUMP) */
void Renderer3D::Batch::AddFace(int bucket, int cls, int count, uint32_t first, uint16_t hd,
                                uint32_t src)
{
    float zmin = 1e30f, zmax = -1e30f;
    for (int i = 0; i < count; i++) {
        float z = v[first + (uint32_t)i].z;
        zmin = z < zmin ? z : zmin;
        zmax = z > zmax ? z : zmax;
    }
    if (nf == capf) {
        capf = capf ? capf * 2 : 16384;
        f = (Face *)realloc(f, (size_t)capf * sizeof *f);
        sf = (Face *)realloc(sf, (size_t)capf * sizeof *sf);
    }
    Face *o = &f[nf++];
    o->bucket = (uint16_t)bucket;
    o->cls = (uint8_t)cls;
    o->nv = (uint8_t)count;
    o->first = first;
    o->zmin = zmin;
    o->zmax = zmax;
    o->hd = hd;
    o->src = src;
}

/* Faces by bucket, far to near; within a bucket the last added first (the PS1 links each new
 * primitive at the front of its bucket). */
void Renderer3D::Batch::Sort()
{
    static int count[4096];   /* scratch */
    memset(count, 0, sizeof count);
    for (int i = 0; i < nf; i++)
        count[f[i].bucket]++;
    int pos = 0;
    for (int k = 4095; k >= 0; k--) {
        int c = count[k];
        count[k] = pos;
        pos += c;
    }
    for (int i = nf - 1; i >= 0; i--)
        sf[count[f[i].bucket]++] = f[i];
    uint32_t vi = 0;
    for (int i = 0; i < nf; i++) {
        Face *o = &sf[i];
        memcpy(&sv[vi], &v[o->first], o->nv * sizeof *sv);
        o->first = vi;
        vi += o->nv;
    }
    sorted = 1;
}

/* ---------------------------------------------------------------------------------------------
 * Faces (ProcessPolys) */

/* colour with the fog m_fog, as retail's colours */
uint32_t Renderer3D::Dpcs(uint32_t rgbc)
{
    uint32_t rgb;
    geom_dpcs(&pc_geom, rgbc & 0xffffff, (uint8_t)(rgbc >> 24), m_fog, 12, 0, &rgb, NULL);
    return rgb;
}

/* colour * light (11:11:10, 128 = 1.0 in each), then the fog */
uint32_t Renderer3D::Dpcl(uint32_t rgbc, uint32_t light)
{
    int16_t l[3];
    uint32_t rgb;
    l[0] = (int16_t)(light << 5);
    l[1] = (int16_t)(light >> 6);
    l[2] = (int16_t)(light >> 17);
    geom_dpcl(&pc_geom, rgbc, l, m_fog, 12, 0, &rgb, NULL);
    return rgb;
}

/* THPS_R3D_PROBE="frame,x,y": log the faces whose retail screen box contains x,y (debugging). */
void Renderer3D::Probe(const Addr *sl, int fast, uint32_t f, const char *what, uint32_t a,
                       uint32_t c, uint32_t z)
{
    if (m_probeOn < 0) {
        const char *e = getenv("THPS_R3D_PROBE");
        m_probeOn = e && sscanf(e, "%d,%d,%d", &m_probeFrame, &m_probeX, &m_probeY) == 3;
    }
    if (!m_probeOn || (int)pc_heartbeat != m_probeFrame)
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
    if (m_probeX < x0 || m_probeX > x1 || m_probeY < y0 || m_probeY > y1)
        return;
    PC_Log("r3d probe %s: flags %04x fast %d box %d,%d-%d,%d a %x col %06x z %d", what, f & 0xffff,
           fast, x0, y0, x1, y1, a, c & 0xffffff, (int)z);}

/* The face walk of ProcessPolys (fast = ProcessPolysFast), into batch b; water: the call tests
 * the water (nullptr if not). */
void Renderer3D::Faces(Batch *b, Addr face, int n, int fast, const Water *water)
{
    Addr cache = fast ? SCR(0) : (Addr)ProjectedVertices;
    uint32_t gff = fast ? 0xffff0000u : M32(SCR(S_FACEFLAGS));
    int mirror = M16(SCR(S_MIRROR)) != 0;
    int dpqmin = S16(SCR(S_DPQMIN)), dpqshift = S16(SCR(S_DPQSHIFT));
    int push1 = S16(SCR(S_PUSHBACK)), push2 = S16(SCR(S_PUSHBACK2));
    Addr ctab = (Addr)pColourTable;
    int sky = !fast && m_lastKind == T_BG;
    int32_t fade = (int32_t)M32(SCR(S_FADECOLOUR));
    int wxl = 0, wxr = 0, wpad = fast ? ViewportPad(&wxl, &wxr) : 0;

    for (int k = 0; k < n; k++) {
        uint32_t w0 = M32(face);
        uint32_t f = (w0 & (uint32_t)((int32_t)gff >> 16)) | (gff & 0xffff);
        Addr a0 = face;
        face += f >> 16;
        m_src = (uint32_t)a0;
        if (!(f & 0xc0))
            continue;
        uint32_t w1 = M32(a0 + 4);
        int tri = (f & 0x10) != 0, nvtx = tri ? 3 : 4;
        uint32_t idx[4] = { w1 & 0xff, (w1 >> 8) & 0xff, (w1 >> 16) & 0xff, w1 >> 24 };
        Addr sl[4];
        for (int i = 0; i < 4; i++)
            sl[i] = cache + idx[i] * 8;

        /* outcodes, as retail; bit 7 (all out) and 15 (not all in) are the water's */
        int near = 0, wet = 0;
        if (!fast) {
            uint32_t t7 = ((M32(sl[0] + 4) & M32(sl[1] + 4) & M32(sl[2] + 4) & M32(sl[3] + 4)) >> 16) ^
                          0xff00;
            if (t7 & 0x20ff) {
                Probe(sl, fast, f, "outcode", t7, 0, 0);
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
                    xo &= FastXOut(M32(sl[i]), wpad, wxl, wxr);
                if ((int32_t)t7 >= 0 || (t7 & 0x60008000u) || xo)
                    continue;
            } else if ((int32_t)t7 >= 0 || (t7 & 0x6000e000u)) {
                continue;
            }
        }

        const Shadow *sh[4];
        int ok = 1;
        for (int i = 0; i < 4 && ok; i++) {
            sh[i] = ShadowAt(sl[i]);
            ok = sh[i] && sh[i]->tag == M32(sl[i]);
        }
        if (!ok) {
            m_st.badtag++;
            continue;
        }

        /* back faces: in view space, from the unrounded vertices. Retail tests the integer screen
         * positions (NCLIP), which gets faces seen almost edge-on wrong: less than a pixel high
         * on the PS1, but at higher resolutions the ones it drops leave cracks in the level
         * (Chicago's floor along the walls). THPS_R3D_NCLIP=1 uses retail's test. */
        int visible;
        float d1 = Det3(sh[0], sh[1], sh[2]), d2 = tri ? 0 : Det3(sh[1], sh[3], sh[2]);
        if (near) {
            m_st.near++;
            if (!(f & 0x1000) && m_nearDrop)
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
            if (m_nclip || m_verify) {
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
                int32_t n1 = Nclip(x[0], y[0], x[1], y[1], x[2], y[2]);
                int32_t n2 = tri ? 0 : Nclip(x[1], y[1], x[2], y[2], x[3], y[3]);
                if (mirror) {
                    n1 = -n1;
                    n2 = -n2;
                }
                if (m_verify && (n1 > 64 || n1 < -64)) {
                    /* the two tests must agree where the screen one is reliable */
                    m_st.sign_checked++;
                    if ((d1 > 0) != (n1 > 0))
                        m_st.sign_bad++;
                }
                if (m_nclip)
                    visible = n1 > 0 || n2 < 0;
            }
        }
        if (!visible) {
            m_st.culled++;
            Probe(sl, fast, f, near ? "near-back" : "back", 0, 0, 0);
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
        if (m_far.on) {
            /* the true depth: beyond 16 bits, and no far plane, so the fog stops at full */
            float fz = sh[0]->z;
            for (int i = 1; i < nvtx; i++)
                if (sh[i]->z > fz)
                    fz = sh[i]->z;
            d = m_far.fogstart == INT32_MAX ? 0 : (int)fz - m_far.fogstart;
            if (d > 0) {
                int64_t f = m_far.fogshift < 0 ? (int64_t)d >> -m_far.fogshift
                                               : (int64_t)d << m_far.fogshift;
                fog = f > 4096 ? 4096 : (int)f;
            }
        } else if (d > 0) {
            fog = dpqshift < 0 ? d >> -dpqshift : d << dpqshift;
        }
        m_fog = (int16_t)fog;
        /* fade-to-colour mode (the front end): beyond the fog distance a face is added onto a
         * flat polygon of the fade colour, which takes the share the fog would (below) */
        if (fade >= 0 && d > 0) {
            f |= 0x1c0;
            f ^= 0x100;
            m_st.fade++;
        }

        /* colours: the colour maths retail uses */
        uint32_t c = M32(a0 + 8), col[4];
        int gouraud = 0;
#define TABLE(i) M32(ctab + ((c >> (8 * (i))) & 0xff) * 4)
#define VLIGHT(i) M32(cache + idx[i] * 8 + 3204)
        if (fast) {
            if (!(f & 0x800)) {
                col[0] = Dpcs(c);
            } else {
                gouraud = 1;
                for (int i = 0; i < nvtx; i++)
                    col[i] = Dpcs(TABLE(i));
            }
        } else if (!(f & 0x800)) {
            if (f & 4) {
                if (f & 8) {
                    gouraud = 1;
                    for (int i = 0; i < nvtx; i++)
                        col[i] = Dpcl(c, VLIGHT(i));
                } else {
                    col[0] = Dpcl(c, M32(cache + M16(a0 + 12) + 3204));
                }
            } else if (f & 8) {
                col[0] = Dpcl(c, M32(SCR(S_CONSTLIGHT)));
            } else {
                col[0] = Dpcs(c);
            }
        } else {
            gouraud = 1;
            uint32_t lf = (f & 4) ? M32(cache + M16(a0 + 12) + 3204) : M32(SCR(S_CONSTLIGHT));
            for (int i = 0; i < nvtx; i++) {
                if ((f & 4) && (f & 8))
                    col[i] = Dpcl(TABLE(i), VLIGHT(i));
                else if (f & 0xc)
                    col[i] = Dpcl(TABLE(i), lf);
                else
                    col[i] = Dpcs(TABLE(i));
            }
        }
#undef TABLE

        /* texture */
        int tex = f & 1, env = !fast && !tex && !gouraud && (f & 2);
        uint32_t uvw[3] = { 0, 0, 0 };
        uint16_t hd = 0;
        int dc = 0;            /* a Dreamcast face: coordinates in uf, vf */
        float uf[4], vf[4];
        if (tex && (M32(a0 + 16) & PC_DC_FACE_TEX)) {
            dc = 1;
            tex = DcFace(a0, (Addr)PC_DcFaceTex(M32(a0 + 16)), f, uvw, &hd, uf, vf);
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
            cn[i].Set(sh[i], gouraud ? col[i] : col[0], uf[i], vf[i]);
        uint32_t first = (uint32_t)b->nv;
        int nv;
        if (!wet) {
            static const int order[2][6] = { { 0, 1, 2 }, { 0, 1, 2, 1, 3, 2 } };
            nv = tri ? 3 : 6;
            GlVert *o = b->AllocVerts(nv);
            for (int i = 0; i < nv; i++)
                PutVert(&o[i], &cn[order[!tri][i]], clut, info, tw, uvr);
        } else {
            /* crosses the water: only the part on the drawn side (FUN_80086500 flagged the
             * corners on the other one), as a fan around the clipped outline */
            static const int ring[2][4] = { { 0, 1, 2 }, { 0, 1, 3, 2 } };
            Corner in[4], out[8];
            int outside[4];
            for (int i = 0; i < nvtx; i++) {
                int vi = ring[!tri][i];
                in[i] = cn[vi];
                in[i].d = water->Depth(in[i].x, in[i].y, in[i].z);
                outside[i] = (M32(sl[vi] + 4) >> 16 & 0x80) != 0;
                if (m_verify && fabsf(in[i].d) >= 1) {
                    m_st.water_checked++;
                    if (outside[i] != ((in[i].d >= 0) != water->keep_below))
                        m_st.water_bad++;
                }
            }
            int m = WaterClip(in, outside, nvtx, out);
            if (m < 3)
                continue;
            nv = (m - 2) * 3;
            GlVert *o = b->AllocVerts(nv);
            for (int i = 0; i < m - 2; i++) {
                PutVert(&o[i * 3], &out[0], clut, info, tw, uvr);
                PutVert(&o[i * 3 + 1], &out[i + 1], clut, info, tw, uvr);
                PutVert(&o[i * 3 + 2], &out[i + 2], clut, info, tw, uvr);
            }
            m_st.water++;
        }
        b->AddFace(bucket, semi ? (abr == 2 ? 2 : 1) : 0, nv, first, hd, m_src);
        m_st.faces++;
        if (fade >= 0 && fog) {
            /* fade-to-colour: the flat polygon under the face (added after it, so drawn first,
             * as retail links it) in the fade colour with the fog's share, DPCS(fade, 1 - fog) */
            static const uint8_t tw0[4] = { 0xff, 0xff, 0, 0 }, uvr0[4] = { 0, 0, 255, 255 };
            m_fog = (int16_t)(4096 - fog);
            uint32_t oc = Dpcs((uint32_t)fade), ofirst = (uint32_t)b->nv;
            GlVert *o = b->AllocVerts(nv);
            for (int i = 0; i < nv; i++) {
                Corner c;
                const GlVert *src = &b->v[first + (uint32_t)i];
                memset(&c, 0, sizeof c);
                c.x = src->x;
                c.y = src->y;
                c.z = src->z;
                c.r = (float)(oc & 0xff);
                c.g = (float)((oc >> 8) & 0xff);
                c.b = (float)((oc >> 16) & 0xff);
                PutVert(&o[i], &c, 0, (uint16_t)(PC_GL_I_VIEW | (sky ? PC_GL_I_SKY : 0)), tw0,
                        uvr0);
            }
            b->AddFace(bucket, 0, nv, ofirst, 0, m_src);
        }
        Probe(sl, fast, f, near ? "near" : "drawn", (uint32_t)bucket, col[0], (uint32_t)zmax);
    }
}

int Renderer3D::ProcessPolys(int fast, uint32_t *list, int count)
{
    if (!fast)
        g_renderTrace.Count("P", 0);
    if (!m_native)
        return 0;
    m_st.calls++;
    Batch *b = m_cur >= 0 ? &m_b[m_cur] : nullptr;
    if (!b || !b->valid || b->ot != CurrentOrderingTable) {
        m_st.fb_nolayer++;
        return 0;
    }
    Water water, *wp = nullptr;
    if (!fast && TestTheWater[0]) {
        Addr cam = (Addr)pCurrentCamera;
        water.m0 = (float)S16(cam + 0x3a);   /* Transform.m[1][0..2] */
        water.m1 = (float)S16(cam + 0x3c);
        water.m2 = (float)S16(cam + 0x3e);
        water.level = (float)((int16_t *)WaterNormal)[3];
        water.keep_below = (WaterClipValue[0] & 0xffff) == 0;
        wp = &water;
    }
    if ((int32_t)M32(SCR(S_FADECOLOUR)) >= 0 &&
        (m_retailFade || (M32(SCR(S_FADECOLOUR)) & 0x02000000u))) {
        /* fade-to-colour with a subtractive polygon: retail's code */
        m_st.fb_fade++;
        return 0;
    }
    if (!fast) {
        /* what retail leaves behind */
        geom_load_vector(pc_geom.tr, 0, 0, 0);
        pc_geom.dqb = 0x10000000;
        pc_geom.dqa = 0;
        SCR_PTR(0x3c4) = (Addr)ClippedFaceBuffer;   /* SCR_CLIPWRITE */
        M16(SCR(676)) = 0;
    }
    Faces(b, (Addr)list, count, fast, wp);
    return 1;
}

/* ---------------------------------------------------------------------------------------------
 * Drawing, while PC_DrawOT walks the ordering table */

/* Faces go to the GPU in the PS1's painter order, in draws of one class. Within a draw, colour is
 * tested against the depth the earlier draws wrote, not against the faces of the same draw, so a
 * face joins the draw only if no opaque face already in it can be in front of it by more than
 * the decal tolerance (the shader tests colour at z - (z/256 + 2)); otherwise a new draw starts
 * and the face is depth-tested against them. Order alone would repeat the PS1's sorting errors
 * (objects showing through walls and floors whose far corner is further away). */
void Renderer3D::DrawUpto(uint32_t bucket)
{
    Batch *b = m_layer;
    int i = b->next;
    if (i < b->nf && b->sf[i].bucket >= bucket) {
        /* the drawing area and offset as the GPU has them here */
        int ox, oy, clip[4], wide = g_otDrawer.State(&ox, &oy, clip);
        g_glGpu.LayerArea(ox, oy, clip, wide);
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
        g_glGpu.LayerDraw(first, count, cls, hd);
        m_st.draws++;
        i = j;
    }
    b->next = i;
}

void Renderer3D::LayerEnd()
{
    DrawUpto(0);
    g_glGpu.LayerEnd();
    m_layer = nullptr;
}

/* THPS_R3D_DUMP=1: with each THPS_SHOT, the faces of the layer drawn last, as text (debugging):
 * a header line "h ofx ofy hither ofs_x ofs_y", then per face "F bucket cls src nv" and its
 * vertices "x y z u v rgb". */
void Renderer3D::DumpLast(const char *path)
{
    Batch *b = m_lastDrawn;
    FILE *fp;
    if (!b || !b->sorted || !(fp = fopen(path, "w")))
        return;
    fprintf(fp, "%g %g %g %g %d %d\n", b->h, b->ofx, b->ofy, b->hither, m_lastOfs[0],
            m_lastOfs[1]);
    for (int i = 0; i < b->nf; i++) {
        const Face *f = &b->sf[i];
        fprintf(fp, "F %d %d %08x %d\n", f->bucket, f->cls, f->src, f->nv);
        for (int k = 0; k < f->nv; k++) {
            const GlVert *v = &b->sv[f->first + (uint32_t)k];
            fprintf(fp, "%.4f %.4f %.4f %g %g %02x%02x%02x\n", v->x, v->y, v->z, v->u, v->v, v->r,
                    v->g, v->b);
        }
    }
    fclose(fp);
}

void Renderer3D::LayerStart(uint32_t id)
{
    Batch *b = &m_b[id & 15];
    if (m_layer)
        LayerEnd();
    if ((id & 15) >= kBatches || !b->valid || b->gen != id >> 4)
        return;
    m_lastDrawn = b;
    if (!b->sorted)
        b->Sort();
    GlLayerParams lp;
    int clip[4];
    lp.wide = g_otDrawer.State(&lp.ofs_x, &lp.ofs_y, clip);
    lp.clip_l = clip[0];
    lp.clip_t = clip[1];
    lp.clip_r = clip[2];
    lp.clip_b = clip[3];
    lp.h = b->h;
    lp.ofx = b->ofx;
    lp.ofy = b->ofy;
    lp.hither = b->hither;
    m_lastOfs[0] = lp.ofs_x;
    m_lastOfs[1] = lp.ofs_y;
    g_glGpu.LayerBegin(&lp, b->sv, b->nv);
    b->next = 0;
    m_layer = b;
    m_bucket = 4095;
    m_st.layers++;
}

bool Renderer3D::ChainPacket(const P_TAG *p)
{
    const u_long *w = P_WORDS(p);
    if (!m_native)
        return false;
    if (p->len == 3 && w[0] == MARKER0 && w[1] == MARKER1) {
        LayerStart(w[2]);
        return true;
    }
    if (!m_layer)
        return false;
    const P_TAG *ot = m_layer->ot;
    if (p >= ot && p < ot + 4096) {
        m_bucket = (uint32_t)(p - ot);   /* an entry of the layer: its primitives follow */
        return false;
    }
    if (p->len == 0) {
        /* the entries before (OT, OT2D) and after (the other player's layer) end it */
        if ((p < ot && ot - p <= 2) || (p >= ot + 4096 && p < ot + 8192))
            LayerEnd();
        return false;
    }
    DrawUpto(m_bucket);
    return false;
}

void Renderer3D::ChainEnd()
{
    if (m_layer)
        LayerEnd();
    StatsTick();
}

/* ---------------------------------------------------------------------------------------------
 * Setup */

void Renderer3D::StatsTick()
{
    if (!m_showStats || pc_heartbeat - m_statsFrame < 60)
        return;
    m_statsFrame = pc_heartbeat;
    fprintf(stderr, "[r3d] %u faces (%u back faces culled, %u stale vertices, %u clipped by water) "
                    "in %u calls, %u layers, %u draws, %u faded; retail: %u fade, %u no layer\n",
            m_st.faces, m_st.culled, m_st.badtag, m_st.water, m_st.calls, m_st.layers, m_st.draws,
            m_st.fade, m_st.fb_fade, m_st.fb_nolayer);
    if (m_st.far)
        fprintf(stderr, "[r3d] %u models beyond the 16-bit view (entire level)\n", m_st.far);
    if (m_verify)
        fprintf(stderr, "[r3d] near-plane faces: %u\n", m_st.near);
    if (m_verify && m_st.water_checked)
        fprintf(stderr, "[r3d] verify water sides: %u of %u corners disagree\n", m_st.water_bad,
                m_st.water_checked);
    if (m_verify && m_st.sign_checked)
        fprintf(stderr, "[r3d] verify back faces: %u of %u disagree\n", m_st.sign_bad,
                m_st.sign_checked);
    if (m_verify)
        for (int k = 0; k < T_COUNT; k++)
            if (m_st.vchecked[k])
                fprintf(stderr, "[r3d] verify %s: %u of %u vertices off\n", s_kind_name[k],
                        m_st.vbad[k], m_st.vchecked[k]);
    memset(&m_st, 0, sizeof m_st);
}

void Renderer3D::Init(int gl)
{
    const char *e = getenv("THPS_RENDER3D");
    m_native = gl && !(e && strcmp(e, "psx") == 0);
    m_showStats = getenv("THPS_R3DSTATS") && atoi(getenv("THPS_R3DSTATS"));
    m_verify = getenv("THPS_R3D_VERIFY") && atoi(getenv("THPS_R3D_VERIFY"));
    m_nearDrop = getenv("THPS_R3D_NEARDROP") && atoi(getenv("THPS_R3D_NEARDROP"));
    m_retailFade = getenv("THPS_R3D_RETAILFADE") && atoi(getenv("THPS_R3D_RETAILFADE"));
    m_nclip = getenv("THPS_R3D_NCLIP") && atoi(getenv("THPS_R3D_NCLIP"));
    if (m_native)
        g_precise.Enable(true);
    fprintf(stderr, "[r3d] 3D: %s\n", m_native ? "native" : "PS1 (THPS_RENDER3D=psx)");
}

} // namespace pc

/* ---- the game's interface (pc_render3d.h) ---- */

int R3d_Native(void) { return pc::g_renderer3d.Native(); }
void R3d_FarBegin(const double m[3][3], const double t[3], int fogstart, int fogshift) { pc::g_renderer3d.FarBegin(m, t, fogstart, fogshift); }
void R3d_FarEnd(void) { pc::g_renderer3d.FarEnd(); }
int R3d_Transform(int kind, R3dTransformFn fn, void *v, int n, void *o) { return pc::g_renderer3d.Transform(kind, fn, v, n, o); }
int R3d_ProcessPolys(int fast, uint32_t *list, int count) { return pc::g_renderer3d.ProcessPolys(fast, list, count); }
void R3d_RenderSetup(const void *viewport, P_TAG *ot) { pc::g_renderer3d.RenderSetup(viewport, ot); }

/* Psy-Q libgpu reimplemented on the software GPU (pc_gpu.h). Every operation runs synchronously:
 * the command queue is always empty, DrawSync returns at once, and a DrawSync callback runs after
 * each DrawOTag. Packets are real GP0 words in PS1 RAM (mapped at 0x80000000), linked by 24-bit
 * addresses, exactly as the hardware DMA walks them. */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PC_LIBGPU_NO_FNTPRINT_DECL
#include <sys/types.h>
#include <libgpu.h>
#include "pc_gpu.h"
#include "pc_gpu_gl.h"
#include "pc_hle.h"

#define LINK(p) ((u_long)(uintptr_t)(p) & 0xffffffu)
#define END_LINK 0x00ffffffu

static int s_debug;
static void (*s_drawsync_cb)(void);
static DRAWENV s_drawenv;
static DISPENV s_dispenv;
static int s_cb_scheduled;

/* ---- packet word builders (shared with SetDrawEnv and the DR_* setters) ---- */

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static u_long word_area(uint32_t cmd, int x, int y)
{
    x = clampi(x, 0, 1023);
    y = clampi(y, 0, 511);
    return cmd | ((u_long)(y & 0x3ff) << 10) | (u_long)(x & 0x3ff);
}

static u_long word_offset(int x, int y)
{
    return 0xe5000000u | ((u_long)(y & 0x7ff) << 11) | (u_long)(x & 0x7ff);
}

static u_long word_mode(int dfe, int dtd, int tpage)
{
    return _get_mode(dfe, dtd, tpage);
}

static u_long word_texwindow(const RECT *tw)
{
    return tw ? _get_tw(tw) : 0;
}

static void tag_len(void *p, int n)
{
    u_long *t = (u_long *)p;
    *t = (*t & 0xffffffu) | ((u_long)n << 24);
}

static void send_words(const u_long *w, int n)
{
    for (int i = 0; i < n; i++)
        PC_GpuWriteGP0((uint32_t)w[i]);
}

/* ---- reset / sync ---- */

int ResetGraph(int mode)
{
    if (s_debug)
        fprintf(stderr, "[libgpu] ResetGraph(%d)\n", mode);
    switch (mode & 3) {
    case 1:
        PC_GpuWriteGP1(0x01000000);
        break;
    default:
        PC_GpuWriteGP1(0x00000000);
        memset(&s_drawenv, 0xff, sizeof s_drawenv);
        memset(&s_dispenv, 0xff, sizeof s_dispenv);
        break;
    }
    return 0;
}

int SetGraphDebug(int level)
{
    int old = s_debug;
    s_debug = level;
    return old;
}

int GetGraphDebug(void)
{
    return s_debug;
}

u_long DrawSyncCallback(void (*func)(void))
{
    void (*old)(void) = s_drawsync_cb;
    s_drawsync_cb = func;
    if (getenv("THPS_TRACE_CULL"))
        fprintf(stderr, "DrawSyncCallback(%p) old=%p\n", (void *)func, (void *)old);
    return (u_long)(uintptr_t)old;
}

void SetDispMask(int mask)
{
    PC_GpuWriteGP1(0x03000000u | (mask ? 0 : 1));
}

int DrawSync(int mode)
{
    (void)mode;
    return 0;
}

/* The interlace field flips every vblank; games spin on it, so let time pass while they do. */
int GetODE(void)
{
    PC_TickVBlank();
    return (int)(PC_VBlankCount() & 1);
}

int GetVideoMode(void)
{
    return 0;
}

int SetVideoMode(int mode)
{
    (void)mode;
    return 0;
}

/* ---- VRAM transfers ---- */

static int rect_ok(const RECT *r)
{
    return r && r->w > 0 && r->h > 0;
}

int ClearImage(RECT *rect, u_char r, u_char g, u_char b)
{
    if (!rect_ok(rect))
        return 0;
    PC_GpuFillRect(rect->x, rect->y, rect->w, rect->h,
                   ((uint32_t)b << 16) | ((uint32_t)g << 8) | r);
    return 0;
}

int ClearImage2(RECT *rect, u_char r, u_char g, u_char b)
{
    return ClearImage(rect, r, g, b);
}

static u_long xfer_xy(const RECT *r)
{
    return ((u_long)(uint16_t)r->y << 16) | (uint16_t)r->x;
}

static u_long xfer_wh(const RECT *r)
{
    return ((u_long)(uint16_t)r->h << 16) | (uint16_t)r->w;
}

int LoadImage(RECT *rect, u_long *p)
{
    if (!rect_ok(rect) || !p)
        return 0;
    int words = (rect->w * rect->h + 1) / 2;
    {
        extern const char *PC_GpuPendingState(void);
        const char *st = PC_GpuPendingState();
        if (st)
            fprintf(stderr, "LoadImage(%d,%d %dx%d): GPU busy: %s\n", rect->x, rect->y, rect->w,
                    rect->h, st);
    }
    PC_GpuWriteGP0(0x01000000u);
    PC_GpuWriteGP0(0xa0000000u);
    PC_GpuWriteGP0((uint32_t)xfer_xy(rect));
    PC_GpuWriteGP0((uint32_t)xfer_wh(rect));
    for (int i = 0; i < words; i++)
        PC_GpuWriteGP0((uint32_t)p[i]);
    return 0;
}

void PC_GpuLoadImageHiRes(int x, int y, int w, int h, const uint16_t *src, int sw, int sh)
{
    if (PC_GlActive()) {
        PC_GlHiResImage(x, y, w, h, src, sw, sh);
        return;
    }
    uint16_t *pix = malloc((size_t)w * h * 2 + 2);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            pix[j * w + i] = src[(j * sh / h) * sw + i * sw / w];
    RECT r = { (short)x, (short)y, (short)w, (short)h };
    LoadImage(&r, (u_long *)pix);
    free(pix);
}

int StoreImage(RECT *rect, u_long *p)
{
    if (!rect_ok(rect) || !p)
        return 0;
    int words = (rect->w * rect->h + 1) / 2;
    PC_GpuWriteGP0(0x01000000u);
    PC_GpuWriteGP0(0xc0000000u);
    PC_GpuWriteGP0((uint32_t)xfer_xy(rect));
    PC_GpuWriteGP0((uint32_t)xfer_wh(rect));
    for (int i = 0; i < words; i++)
        p[i] = PC_GpuReadData();
    return 0;
}

int MoveImage(RECT *rect, int x, int y)
{
    if (!rect || rect->w == 0 || rect->h == 0)
        return -1;
    PC_GpuWriteGP0(0x01000000u);
    PC_GpuWriteGP0(0x80000000u);
    PC_GpuWriteGP0((uint32_t)xfer_xy(rect));
    PC_GpuWriteGP0(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
    PC_GpuWriteGP0((uint32_t)xfer_wh(rect));
    return 0;
}

/* ---- ordering tables and drawing ---- */

u_long *ClearOTag(u_long *ot, int n)
{
    for (int i = 0; i < n - 1; i++)
        ot[i] = LINK(&ot[i + 1]);
    if (n > 0)
        ot[n - 1] = END_LINK;
    return ot;
}

u_long *ClearOTagR(u_long *ot, int n)
{
    for (int i = n - 1; i > 0; i--)
        ot[i] = LINK(&ot[i - 1]);
    if (n > 0)
        ot[0] = END_LINK;
    return ot;
}

#define MAX_PENDING 32
#define MAX_CB_ROUNDS 64

static int s_in_draw;
static u_long *s_pending[MAX_PENDING];
static int s_npending;

void DrawOTag(u_long *p)
{
    if (!p)
        return;
    if (s_in_draw) {
        /* re-entered from the DrawSync callback: queue, the outer call drains it */
        if (s_npending < MAX_PENDING)
            s_pending[s_npending++] = p;
        else
            PC_GpuDrawChain((const uint32_t *)p);
        return;
    }
    s_in_draw = 1;
    PC_GpuDrawChain((const uint32_t *)p);
    while (s_npending) {
        u_long *next = s_pending[0];
        memmove(&s_pending[0], &s_pending[1], (size_t)(s_npending - 1) * sizeof s_pending[0]);
        s_npending--;
        PC_GpuDrawChain((const uint32_t *)next);
    }
    s_in_draw = 0;
    /* Real GPU drawing is asynchronous DMA: games that register a DrawSyncCallback (like THPS1's
     * double-buffer state machine, Db_CallBack) expect it to fire once the GPU goes idle, roughly
     * a vblank after this submission -- not before this function even returns. Calling it
     * synchronously here collapses that multi-tick pipeline into one tick, which desyncs any game
     * logic gated on "is the GPU still busy" (see PC_GpuRunDueCallback, called once per vblank). */
    if (s_drawsync_cb)
        s_cb_scheduled = 1;
}

/* Fire any DrawSyncCallback scheduled by a DrawOTag since the last tick -- called once per vblank,
 * before the game's own VSync callback runs, so state it depends on (e.g. Db_CallBack clearing
 * THPS1's "Rendering" flag) is settled by the time game logic checks it that tick. */
void PC_GpuRunDueCallback(void)
{
    if (!s_cb_scheduled)
        return;
    s_cb_scheduled = 0;
    if (s_drawsync_cb)
        s_drawsync_cb();
}

void DrawOTagEnv(u_long *p, DRAWENV *env)
{
    PutDrawEnv(env);
    DrawOTag(p);
}

void DrawPrim(void *p)
{
    const u_long *w = (const u_long *)p;
    send_words(w + 1, (int)(w[0] >> 24));
}

void AddPrim(void *ot, void *p)
{
    addPrim(ot, p);
}

void AddPrims(void *ot, void *p0, void *p1)
{
    addPrims(ot, p0, p1);
}

void CatPrim(void *p0, void *p1)
{
    catPrim(p0, p1);
}

void TermPrim(void *p)
{
    termPrim(p);
}

void *NextPrim(void *p)
{
    return nextPrim(p);
}

int IsEndPrim(void *p)
{
    return isendprim(p);
}

/* ---- environments ---- */

void SetDrawEnv(DR_ENV *dr_env, DRAWENV *env)
{
    u_long *c = dr_env->code;
    int n = 0;
    c[n++] = word_area(0xe3000000u, env->clip.x, env->clip.y);
    c[n++] = word_area(0xe4000000u, env->clip.x + env->clip.w - 1, env->clip.y + env->clip.h - 1);
    c[n++] = word_offset(env->ofs[0], env->ofs[1]);
    c[n++] = word_mode(env->dfe, env->dtd, env->tpage);
    c[n++] = word_texwindow(&env->tw);
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
    dr_env->tag = (dr_env->tag & 0xffffffu) | ((u_long)n << 24);
}

DRAWENV *PutDrawEnv(DRAWENV *env)
{
    SetDrawEnv(&env->dr_env, env);
    env->dr_env.tag |= END_LINK;
    /* a screen-sized drawing area is (or will be) a screen buffer */
    PC_GlNoteScreen(env->clip.x, env->clip.y, env->clip.w, env->clip.h);
    send_words(env->dr_env.code, (int)(env->dr_env.tag >> 24));
    memcpy(&s_drawenv, env, sizeof s_drawenv);
    return env;
}

DRAWENV *GetDrawEnv(DRAWENV *env)
{
    memcpy(env, &s_drawenv, sizeof *env);
    return env;
}

DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h)
{
    env->clip.x = (short)x;
    env->clip.y = (short)y;
    env->clip.w = (short)w;
    env->clip.h = (short)h;
    env->ofs[0] = (short)x;
    env->ofs[1] = (short)y;
    env->tw.x = env->tw.y = env->tw.w = env->tw.h = 0;
    env->tpage = 0x0a;
    env->dtd = 1;
    env->dfe = (h > 256) ? 1 : 0;
    env->isbg = 0;
    env->r0 = env->g0 = env->b0 = 0;
    return env;
}

DISPENV *PutDispEnv(DISPENV *env)
{
    int pal = GetVideoMode() == 1;
    uint32_t mode = 0;

    PC_GpuWriteGP1(0x05000000u | ((uint32_t)(env->disp.y & 0x3ff) << 10) | (env->disp.x & 0x3ff));

    int hs = 0x260 + env->screen.x * 10;
    int he = hs + (env->screen.w ? env->screen.w * 10 : 2560);
    int vs = (pal ? 0x13 : 0x10) + env->screen.y;
    int ve = vs + (env->screen.h ? env->screen.h : (pal ? 256 : 240));
    hs = clampi(hs, 500, 3290);
    he = clampi(he, hs + 80, 3290);
    vs = clampi(vs, 0x10, pal ? 0x136 : 0x106);
    ve = clampi(ve, vs + 2, pal ? 0x138 : 0x108);
    PC_GpuWriteGP1(0x06000000u | ((uint32_t)he << 12) | (uint32_t)hs);
    PC_GpuWriteGP1(0x07000000u | ((uint32_t)ve << 10) | (uint32_t)vs);

    if (env->isinter)
        mode |= 0x20;
    if (env->isrgb24)
        mode |= 0x10;
    if (pal)
        mode |= 0x08;
    if (env->disp.w > 280) {
        if (env->disp.w <= 352)
            mode |= 0x01;
        else if (env->disp.w <= 400)
            mode |= 0x40;
        else if (env->disp.w <= 560)
            mode |= 0x02;
        else
            mode |= 0x03;
    }
    if (env->disp.h > (pal ? 288 : 256))
        mode |= 0x24;
    PC_GpuWriteGP1(0x08000000u | mode);
    if (!env->isrgb24)
        PC_GlNoteScreen(env->disp.x, env->disp.y, env->disp.w, env->disp.h);

    memcpy(&s_dispenv, env, sizeof s_dispenv);
    return env;
}

DISPENV *GetDispEnv(DISPENV *env)
{
    memcpy(env, &s_dispenv, sizeof *env);
    return env;
}

DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h)
{
    env->disp.x = (short)x;
    env->disp.y = (short)y;
    env->disp.w = (short)w;
    env->disp.h = (short)h;
    env->screen.x = env->screen.y = env->screen.w = env->screen.h = 0;
    env->isinter = 0;
    env->isrgb24 = 0;
    env->pad0 = env->pad1 = 0;
    return env;
}

/* ---- state packets ---- */

void SetDrawArea(DR_AREA *p, RECT *r)
{
    tag_len(p, 2);
    p->code[0] = word_area(0xe3000000u, r->x, r->y);
    p->code[1] = word_area(0xe4000000u, r->x + r->w - 1, r->y + r->h - 1);
}

void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw)
{
    tag_len(p, 2);
    p->code[0] = word_mode(dfe, dtd, tpage);
    p->code[1] = word_texwindow(tw);
}

void SetDrawTPage(DR_TPAGE *p, int dfe, int dtd, int tpage)
{
    tag_len(p, 1);
    p->code[0] = word_mode(dfe, dtd, tpage);
}

void SetDrawOffset(DR_OFFSET *p, u_short *ofs)
{
    tag_len(p, 2);
    p->code[0] = word_offset((short)ofs[0], (short)ofs[1]);
    p->code[1] = 0;
}

void SetDrawMove(DR_MOVE *p, RECT *rect, int x, int y)
{
    tag_len(p, (rect->w && rect->h) ? 5 : 0);
    p->code[0] = 0x01000000u;
    p->code[1] = 0x80000000u;
    p->code[2] = xfer_xy(rect);
    p->code[3] = ((u_long)(uint16_t)y << 16) | (uint16_t)x;
    p->code[4] = xfer_wh(rect);
}

void SetDrawLoad(DR_LOAD *p, RECT *rect)
{
    int len = (rect->w * rect->h + 1) / 2 + 4;
    if (len > 16) {
        tag_len(p, 0);
        return;
    }
    tag_len(p, len);
    p->code[0] = 0xa0000000u;
    p->code[1] = xfer_xy(rect);
    p->code[2] = xfer_wh(rect);
    p->p[len - 4] = 0x01000000u;
}

void SetDrawStp(DR_STP *p, int pbw)
{
    tag_len(p, 2);
    p->code[0] = 0xe6000000u | (pbw ? 1u : 0u);
    p->code[1] = 0;
}

void SetTexWindow(DR_TWIN *p, RECT *tw)
{
    tag_len(p, 2);
    p->code[0] = word_texwindow(tw);
    p->code[1] = 0;
}

/* ---- texture page / CLUT helpers ---- */

u_short GetTPage(int tp, int abr, int x, int y)
{
    return (u_short)getTPage(tp, abr, x, y);
}

u_short GetClut(int x, int y)
{
    return (u_short)getClut(x, y);
}

u_short LoadTPage(u_long *pix, int tp, int abr, int x, int y, int w, int h)
{
    RECT r;
    r.x = (short)x;
    r.y = (short)y;
    r.w = (short)(tp == 0 ? w / 4 : tp == 1 ? w / 2 : w);
    r.h = (short)h;
    LoadImage(&r, pix);
    return GetTPage(tp, abr, x, y);
}

u_short LoadClut(u_long *clut, int x, int y)
{
    RECT r;
    setRECT(&r, x, y, 256, 1);
    LoadImage(&r, clut);
    return GetClut(x, y);
}

u_short LoadClut2(u_long *clut, int x, int y)
{
    RECT r;
    setRECT(&r, x, y, 16, 1);
    LoadImage(&r, clut);
    return GetClut(x, y);
}

/* ---- primitive initialisers ---- */

void SetPolyF3(POLY_F3 *p) { setPolyF3(p); }
void SetPolyF4(POLY_F4 *p) { setPolyF4(p); }
void SetPolyFT3(POLY_FT3 *p) { setPolyFT3(p); }
void SetPolyFT4(POLY_FT4 *p) { setPolyFT4(p); }
void SetPolyG3(POLY_G3 *p) { setPolyG3(p); }
void SetPolyG4(POLY_G4 *p) { setPolyG4(p); }
void SetPolyGT3(POLY_GT3 *p) { setPolyGT3(p); }
void SetPolyGT4(POLY_GT4 *p) { setPolyGT4(p); }
void SetLineF2(LINE_F2 *p) { setLineF2(p); }
void SetLineF3(LINE_F3 *p) { setLineF3(p); }
void SetLineF4(LINE_F4 *p) { setLineF4(p); }
void SetLineG2(LINE_G2 *p) { setLineG2(p); }
void SetLineG3(LINE_G3 *p) { setLineG3(p); }
void SetLineG4(LINE_G4 *p) { setLineG4(p); }
void SetSprt(SPRT *p) { setSprt(p); }
void SetSprt16(SPRT_16 *p) { setSprt16(p); }
void SetSprt8(SPRT_8 *p) { setSprt8(p); }
void SetTile(TILE *p) { setTile(p); }
void SetTile1(TILE_1 *p) { setTile1(p); }
void SetTile8(TILE_8 *p) { setTile8(p); }
void SetTile16(TILE_16 *p) { setTile16(p); }
void SetSemiTrans(void *p, int abe) { setSemiTrans(p, abe); }
void SetShadeTex(void *p, int tge) { setShadeTex(p, tge); }

/* ---- debug font: text goes to stderr on FntFlush (deduplicated per stream) ---- */

#define FNT_BUF 1024

static struct {
    int open;
    int x, y, w, h;
    char buf[FNT_BUF];
    int len;
    char last[FNT_BUF];
} s_fnt[FNT_MAX_ID];
static int s_fnt_count;
static int s_fnt_default = -1;
static int s_fnt_dump = -1;
static u_long s_fnt_ot = END_LINK;

void FntLoad(int tx, int ty)
{
    (void)tx;
    (void)ty;
    s_fnt_count = 0;
    s_fnt_default = -1;
    memset(s_fnt, 0, sizeof s_fnt);
}

int FntOpen(int x, int y, int w, int h, int isbg, int n)
{
    (void)isbg;
    (void)n;
    if (s_fnt_count >= FNT_MAX_ID)
        return -1;
    int id = s_fnt_count++;
    s_fnt[id].open = 1;
    s_fnt[id].x = x;
    s_fnt[id].y = y;
    s_fnt[id].w = w;
    s_fnt[id].h = h;
    s_fnt[id].len = 0;
    s_fnt[id].last[0] = 0;
    s_fnt_default = id;
    return id;
}

void SetDumpFnt(int id)
{
    if (id >= 0 && id < FNT_MAX_ID)
        s_fnt_dump = id;
}

/* Accepts FntPrint(fmt, ...) and FntPrint(id, fmt, ...): a first argument below FNT_MAX_ID is a
 * stream id (PS1 pointers are 0x80xxxxxx, native ones are far above it too). */
int FntPrint(int a0, ...)
{
    va_list ap;
    int id;
    const char *fmt;
    va_start(ap, a0);
    if ((unsigned)a0 < FNT_MAX_ID) {
        id = a0;
        fmt = va_arg(ap, const char *);
    } else {
        id = s_fnt_dump >= 0 ? s_fnt_dump : s_fnt_default;
        fmt = (const char *)(uintptr_t)(unsigned)a0;
    }
    int n = 0;
    if (id >= 0 && id < FNT_MAX_ID && s_fnt[id].open && fmt) {
        int room = FNT_BUF - 1 - s_fnt[id].len;
        if (room > 0) {
            n = vsnprintf(s_fnt[id].buf + s_fnt[id].len, (size_t)room + 1, fmt, ap);
            if (n > room)
                n = room;
            if (n > 0)
                s_fnt[id].len += n;
        }
    }
    va_end(ap);
    return n;
}

u_long *FntFlush(int id)
{
    if (id < 0 || id >= FNT_MAX_ID)
        id = s_fnt_dump >= 0 ? s_fnt_dump : s_fnt_default;
    if (id >= 0 && id < FNT_MAX_ID && s_fnt[id].open) {
        s_fnt[id].buf[s_fnt[id].len] = 0;
        if (s_fnt[id].len && strcmp(s_fnt[id].buf, s_fnt[id].last) != 0) {
            fprintf(stderr, "[fnt%d] %s%s", id, s_fnt[id].buf,
                    s_fnt[id].buf[s_fnt[id].len - 1] == '\n' ? "" : "\n");
            memcpy(s_fnt[id].last, s_fnt[id].buf, (size_t)s_fnt[id].len + 1);
        }
        s_fnt[id].len = 0;
    }
    s_fnt_ot = END_LINK;
    return &s_fnt_ot;
}

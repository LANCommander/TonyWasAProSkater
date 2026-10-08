/* Standalone test of the software GPU and the libgpu HLE. Maps PS1 RAM at 0x80000000, builds
 * ordering tables and primitives there exactly as the game does, draws, checks exact pixels and
 * writes BMPs to build/pc/test_gpu/. Runs headless unless THPS_HEADLESS=0 is set.
 *
 * Build (from the repo root, MSYS2 mingw32 first on PATH):
 *   gcc -m32 -std=gnu11 -O2 -Wl,--large-address-aware -Iplatform/pc/include
 *       -Iplatform/pc/include/psyq platform/pc/tests/test_gpu.c platform/pc/src/pc_gpu.c
 *       platform/pc/src/pc_video.c platform/pc/src/hle_libgpu.c $(sdl2-config --cflags --static-libs)
 *       -mconsole -o build/pc/test_gpu/test_gpu.exe */
#include <direct.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <libgpu.h>
#include "pc_gpu.h"

#define OUT_DIR "build/pc/test_gpu"

/* windows.h is not included: its RECT and LoadImage clash with libgpu's. */
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define PAGE_READWRITE 0x04
__declspec(dllimport) void *__stdcall VirtualAlloc(void *addr, unsigned long size, unsigned long type,
                                                   unsigned long protect);
__declspec(dllimport) unsigned long __stdcall GetLastError(void);
__declspec(dllimport) void __stdcall Sleep(unsigned long ms);

static int s_fails, s_checks;

#define CHECK(cond, ...)                                         \
    do {                                                         \
        s_checks++;                                              \
        if (!(cond)) {                                           \
            s_fails++;                                           \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);          \
            printf(__VA_ARGS__);                                 \
            printf("\n");                                        \
        }                                                        \
    } while (0)

static uint8_t *s_heap;

static void *palloc(size_t n)
{
    void *p = s_heap;
    s_heap += (n + 3) & ~(size_t)3;
    memset(p, 0, n);
    return p;
}

static uint16_t px(int x, int y)
{
    return PC_GpuVram()[y * 1024 + x];
}

static uint16_t c15(int r5, int g5, int b5)
{
    return (uint16_t)(r5 | (g5 << 5) | (b5 << 10));
}

static void tpage(int tp, int abr, int x, int y)
{
    DR_TPAGE *p = palloc(sizeof *p);
    SetDrawTPage(p, 0, 0, GetTPage(tp, abr, x, y));
    DrawPrim(p);
}

static void tile(int x, int y, int w, int h, int r, int g, int b, int semi)
{
    TILE *t = palloc(sizeof *t);
    setTile(t);
    setXY0(t, x, y);
    setWH(t, w, h);
    setRGB0(t, r, g, b);
    setSemiTrans(t, semi);
    DrawPrim(t);
}

static int s_cb_count;
static int s_cb_redraw;
static u_long *s_cb_ot;

static void drawsync_cb(void)
{
    s_cb_count++;
    if (s_cb_redraw) {
        s_cb_redraw = 0;
        DrawOTag(s_cb_ot);
    }
}

/* Texture sources in VRAM: 4-bit 16x16 at (640,0) with CLUT (0,480); 8-bit 32x32 at (704,0)
 * with CLUT (0,481); 15-bit 32x32 at (768,0). */
static void load_textures(void)
{
    RECT r;

    u_long *t4 = palloc(16 * 16 / 2);
    uint16_t *h4 = (uint16_t *)t4;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            int idx = ((x / 4) + (y / 4)) % 4;
            if (x == 7 && y == 9)
                idx = 5;
            h4[y * 4 + x / 4] |= (uint16_t)(idx << ((x & 3) * 4));
        }
    setRECT(&r, 640, 0, 4, 16);
    LoadImage(&r, t4);

    u_long *clut4 = palloc(16 * 2);
    uint16_t *hc4 = (uint16_t *)clut4;
    hc4[0] = 0;
    hc4[1] = c15(31, 0, 0);
    hc4[2] = c15(0, 31, 0);
    hc4[3] = c15(31, 31, 0);
    hc4[5] = c15(3, 7, 11);
    LoadClut2(clut4, 0, 480);

    u_long *t8 = palloc(32 * 32);
    uint8_t *b8 = (uint8_t *)t8;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            b8[y * 32 + x] = (uint8_t)(((x ^ y) * 8) & 0xff);
    setRECT(&r, 704, 0, 16, 32);
    LoadImage(&r, t8);

    u_long *clut8 = palloc(256 * 2);
    uint16_t *hc8 = (uint16_t *)clut8;
    for (int i = 0; i < 256; i++)
        hc8[i] = c15(i >> 3, 31 - (i >> 3), (i >> 4) + 8);
    LoadClut(clut8, 0, 481);

    u_long *t15 = palloc(32 * 32 * 2);
    uint16_t *h15 = (uint16_t *)t15;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            h15[y * 32 + x] = c15(x, y, 31 - x);
    h15[0] = c15(20, 10, 31);
    h15[1] = c15(31, 0, 0) | 0x8000;
    setRECT(&r, 768, 0, 32, 32);
    LoadImage(&r, t15);
}

static void scene_display(void)
{
    u_long *ot = palloc(8 * sizeof(u_long));
    ClearOTagR(ot, 8);

    POLY_G4 *g4 = palloc(sizeof *g4);
    setPolyG4(g4);
    setXY4(g4, 10, 10, 150, 10, 10, 110, 150, 110);
    setRGB0(g4, 255, 0, 0);
    setRGB1(g4, 0, 255, 0);
    setRGB2(g4, 0, 0, 255);
    setRGB3(g4, 255, 255, 255);
    addPrim(&ot[7], g4);

    POLY_F3 *f3 = palloc(sizeof *f3);
    setPolyF3(f3);
    setXY3(f3, 170, 10, 310, 10, 240, 110);
    setRGB0(f3, 255, 220, 0);
    addPrim(&ot[6], f3);

    POLY_FT4 *ft4a = palloc(sizeof *ft4a);
    setPolyFT4(ft4a);
    setXYWH(ft4a, 10, 120, 64, 64);
    setUVWH(ft4a, 0, 0, 16, 16);
    setRGB0(ft4a, 128, 128, 128);
    ft4a->tpage = GetTPage(0, 0, 640, 0);
    ft4a->clut = GetClut(0, 480);
    addPrim(&ot[5], ft4a);

    POLY_FT4 *ft4b = palloc(sizeof *ft4b);
    setPolyFT4(ft4b);
    setXY4(ft4b, 84, 120, 148, 130, 90, 184, 150, 178);
    setUVWH(ft4b, 0, 0, 31, 31);
    setRGB0(ft4b, 255, 160, 96);
    ft4b->tpage = GetTPage(1, 0, 704, 0);
    ft4b->clut = GetClut(0, 481);
    addPrim(&ot[5], ft4b);

    POLY_GT4 *gt4 = palloc(sizeof *gt4);
    setPolyGT4(gt4);
    setXYWH(gt4, 158, 120, 64, 64);
    setUVWH(gt4, 0, 0, 31, 31);
    setRGB0(gt4, 255, 255, 255);
    setRGB1(gt4, 128, 128, 128);
    setRGB2(gt4, 128, 128, 128);
    setRGB3(gt4, 32, 32, 32);
    gt4->tpage = GetTPage(2, 0, 768, 0);
    addPrim(&ot[5], gt4);

    SPRT *sp = palloc(sizeof *sp);
    setSprt(sp);
    setXY0(sp, 232, 120);
    setWH(sp, 32, 32);
    setUV0(sp, 0, 0);
    setRGB0(sp, 128, 128, 128);
    setShadeTex(sp, 1);
    addPrim(&ot[4], sp);
    DR_TPAGE *sptp = palloc(sizeof *sptp);
    SetDrawTPage(sptp, 0, 1, GetTPage(2, 0, 768, 0));
    addPrim(&ot[4], sptp);

    /* additive white-ish tile over the gouraud quad; DR_MODE added last so it runs first */
    TILE *st = palloc(sizeof *st);
    setTile(st);
    setXY0(st, 40, 40);
    setWH(st, 80, 40);
    setRGB0(st, 64, 64, 64);
    setSemiTrans(st, 1);
    addPrim(&ot[3], st);
    DR_MODE *dm = palloc(sizeof *dm);
    SetDrawMode(dm, 0, 1, GetTPage(2, 1, 768, 0), NULL);
    addPrim(&ot[3], dm);

    LINE_G2 *lg2 = palloc(sizeof *lg2);
    setLineG2(lg2);
    setXY2(lg2, 10, 200, 150, 230);
    setRGB0(lg2, 255, 0, 0);
    setRGB1(lg2, 0, 0, 255);
    addPrim(&ot[2], lg2);

    LINE_F3 *lf3 = palloc(sizeof *lf3);
    setLineF3(lf3);
    setXY3(lf3, 170, 200, 240, 230, 310, 200);
    setRGB0(lf3, 255, 255, 255);
    addPrim(&ot[2], lf3);

    LINE_G4 *lg4 = palloc(sizeof *lg4);
    setLineG4(lg4);
    setXY4(lg4, 270, 120, 310, 120, 310, 180, 270, 180);
    setRGB0(lg4, 255, 0, 0);
    setRGB1(lg4, 0, 255, 0);
    setRGB2(lg4, 0, 0, 255);
    setRGB3(lg4, 255, 255, 0);
    addPrim(&ot[2], lg4);

    for (int i = 0; i < 8; i++) {
        TILE_1 *t1 = palloc(sizeof *t1);
        setTile1(t1);
        setXY0(t1, 20 + i * 4, 190);
        setRGB0(t1, 255, 255, 255);
        addPrim(&ot[1], t1);
    }

    SPRT_8 *s8 = palloc(sizeof *s8);
    setSprt8(s8);
    setXY0(s8, 80, 188);
    setUV0(s8, 0, 0);
    setRGB0(s8, 128, 128, 128);
    s8->clut = GetClut(0, 480);
    addPrim(&ot[1], s8);
    DR_TPAGE *s8tp = palloc(sizeof *s8tp);
    SetDrawTPage(s8tp, 0, 1, GetTPage(0, 0, 640, 0));
    addPrim(&ot[1], s8tp);

    int before = s_cb_count;
    DrawOTag(&ot[7]);
    DrawSync(0);
    CHECK(s_cb_count == before + 1, "DrawSync callback ran %d times", s_cb_count - before);

    /* front-to-back order: the semi tile (ot[3]) blends over the gouraud quad (ot[7]) */
    uint16_t under = px(80, 60);
    CHECK((under & 0x7fff) != 0, "gouraud quad under tile is empty");
}

static void exact_tests(void)
{
    DRAWENV *de = palloc(sizeof *de);
    RECT r;

    SetDefDrawEnv(de, 0, 0, 1024, 512);
    de->dtd = 0;
    PutDrawEnv(de);
    CHECK((PC_GpuReadStat() & 0x200) == 0, "dither bit still set");

    /* ClearOTagR layout */
    u_long *ot = palloc(8 * sizeof(u_long));
    ClearOTagR(ot, 8);
    CHECK(ot[0] == 0x00ffffff, "ot[0] = %08lx", ot[0]);
    for (int i = 1; i < 8; i++)
        CHECK(ot[i] == ((u_long)(uintptr_t)&ot[i - 1] & 0xffffff), "ot[%d] = %08lx", i, ot[i]);

    /* flat triangle coverage: top-left rule, no last row/column */
    setRECT(&r, 390, 290, 40, 40);
    ClearImage(&r, 0, 0, 0);
    POLY_F3 *t = palloc(sizeof *t);
    setPolyF3(t);
    setXY3(t, 400, 300, 410, 300, 400, 310);
    setRGB0(t, 255, 255, 255);
    DrawPrim(t);
    POLY_F3 *t2 = palloc(sizeof *t2);
    setPolyF3(t2);
    setXY3(t2, 420, 300, 420, 310, 410, 310);
    setRGB0(t2, 255, 255, 255);
    DrawPrim(t2);
    int bad = 0, count = 0;
    for (int y = 290; y < 330; y++)
        for (int x = 390; x < 430; x++) {
            int j = y - 300;
            int in1 = j >= 0 && j < 10 && x >= 400 && x < 410 - j;
            int in2 = j >= 0 && j < 10 && x >= 420 - j && x < 420;
            int want = in1 || in2;
            int got = px(x, y) == 0x7fff;
            count += got;
            if (want != got && bad++ < 5)
                printf("  coverage mismatch at (%d,%d): want %d got %d\n", x, y, want, got);
        }
    CHECK(bad == 0, "triangle coverage: %d mismatches (%d px drawn, want 100)", bad, count);

    /* modulated texel via a sprite (rectangles never dither): texel (20,10,31) * (64,128,255) */
    tpage(2, 0, 768, 0);
    SPRT *s = palloc(sizeof *s);
    setSprt(s);
    setXY0(s, 400, 350);
    setWH(s, 1, 1);
    setUV0(s, 0, 0);
    setRGB0(s, 64, 128, 255);
    DrawPrim(s);
    CHECK(px(400, 350) == c15(10, 10, 31), "modulated texel %04x want %04x", px(400, 350),
          c15(10, 10, 31));

    /* textured quad, colour 128, dither off: texels copied unchanged */
    POLY_FT4 *q = palloc(sizeof *q);
    setPolyFT4(q);
    setXYWH(q, 400, 360, 8, 8);
    setUVWH(q, 0, 0, 8, 8);
    setRGB0(q, 128, 128, 128);
    q->tpage = GetTPage(2, 0, 768, 0);
    DrawPrim(q);
    CHECK(px(400, 360) == c15(20, 10, 31), "FT4 texel (0,0) = %04x", px(400, 360));
    CHECK(px(403, 365) == c15(3, 5, 28), "FT4 texel (3,5) = %04x", px(403, 365));
    CHECK(px(408, 360) == 0, "FT4 drew its last column");

    /* 4-bit CLUT lookup, raw texture */
    tpage(0, 0, 640, 0);
    SPRT *s4 = palloc(sizeof *s4);
    setSprt(s4);
    setXY0(s4, 420, 350);
    setWH(s4, 16, 16);
    setUV0(s4, 0, 0);
    s4->clut = GetClut(0, 480);
    setShadeTex(s4, 1);
    DrawPrim(s4);
    CHECK(px(420 + 7, 350 + 9) == c15(3, 7, 11), "4-bit texel = %04x", px(427, 359));
    CHECK(px(420 + 4, 350) == c15(31, 0, 0), "4-bit texel idx1 = %04x", px(424, 350));
    CHECK(px(420, 350) == 0, "index 0 (colour 0000) must be transparent");

    /* semi-transparency, all four modes, untextured tile over (10,20,30) */
    setRECT(&r, 450, 350, 8, 8);
    ClearImage(&r, 80, 160, 240);
    uint16_t expect[4] = { c15(20, 10, 15), c15(31, 20, 30), c15(0, 20, 30), c15(17, 20, 30) };
    for (int abr = 0; abr < 4; abr++) {
        tpage(2, abr, 768, 0);
        tile(450 + abr, 350, 1, 1, 248, 0, 0, 1);
        CHECK(px(450 + abr, 350) == expect[abr], "abr %d: %04x want %04x", abr,
              px(450 + abr, 350), expect[abr]);
    }

    /* textured semi: STP texel blends (and keeps bit 15), non-STP texel is opaque */
    tpage(2, 0, 768, 0);
    SPRT *ss = palloc(sizeof *ss);
    setSprt(ss);
    setXY0(ss, 450, 352);
    setWH(ss, 2, 1);
    setUV0(ss, 0, 0);
    setShadeTex(ss, 1);
    setSemiTrans(ss, 1);
    DrawPrim(ss);
    CHECK(px(450, 352) == c15(20, 10, 31), "non-STP texel blended: %04x", px(450, 352));
    CHECK(px(451, 352) == (c15(20, 10, 15) | 0x8000), "STP texel: %04x", px(451, 352));

    /* lines include both endpoints */
    setRECT(&r, 495, 295, 40, 30);
    ClearImage(&r, 0, 0, 0);
    LINE_F2 *l = palloc(sizeof *l);
    setLineF2(l);
    setXY2(l, 500, 300, 510, 300);
    setRGB0(l, 255, 255, 255);
    DrawPrim(l);
    CHECK(px(500, 300) == 0x7fff && px(510, 300) == 0x7fff, "line endpoints missing");
    CHECK(px(499, 300) == 0 && px(511, 300) == 0, "line overshoots");
    LINE_F2 *ld = palloc(sizeof *ld);
    setLineF2(ld);
    setXY2(ld, 505, 315, 500, 310);
    setRGB0(ld, 255, 255, 255);
    DrawPrim(ld);
    int diag = 0;
    for (int i = 0; i <= 5; i++)
        diag += px(500 + i, 310 + i) == 0x7fff;
    CHECK(diag == 6, "diagonal line drew %d/6", diag);

    /* poly-line ends at the terminator; the next command is parsed normally */
    LINE_F3 *pl = palloc(sizeof *pl);
    setLineF3(pl);
    setXY3(pl, 520, 300, 530, 300, 530, 310);
    setRGB0(pl, 255, 255, 255);
    DrawPrim(pl);
    tile(532, 300, 1, 1, 248, 0, 0, 0);
    CHECK(px(530, 310) == 0x7fff && px(525, 300) == 0x7fff, "poly-line segments missing");
    CHECK(px(532, 300) == c15(31, 0, 0), "command after poly-line lost");

    /* GP0(02h) fill: x rounded down to 16, width rounded up to 16, ignores clip */
    setRECT(&r, 580, 395, 60, 10);
    ClearImage(&r, 0, 0, 0);
    PC_GpuWriteGP0(0x020000ff);
    PC_GpuWriteGP0((400u << 16) | 600u);
    PC_GpuWriteGP0((4u << 16) | 20u);
    CHECK(px(592, 400) == c15(31, 0, 0) && px(623, 403) == c15(31, 0, 0), "fill body");
    CHECK(px(591, 400) == 0 && px(624, 400) == 0 && px(600, 404) == 0, "fill bounds");

    /* VRAM->VRAM copy */
    setRECT(&r, 592, 400, 32, 4);
    MoveImage(&r, 700, 450);
    CHECK(px(700, 450) == c15(31, 0, 0) && px(731, 453) == c15(31, 0, 0), "MoveImage");

    /* StoreImage round trip */
    u_long *buf = palloc(32 * 2);
    setRECT(&r, 768, 0, 32, 2);
    StoreImage(&r, buf);
    CHECK(((uint16_t *)buf)[0] == c15(20, 10, 31) && ((uint16_t *)buf)[33] == c15(1, 1, 30),
          "StoreImage %04x %04x", ((uint16_t *)buf)[0], ((uint16_t *)buf)[33]);

    /* mask bits: set-mask marks pixels, check-mask protects them */
    DR_STP *stp = palloc(sizeof *stp);
    setDrawStp(stp, 1);
    DrawPrim(stp);
    tile(540, 300, 1, 1, 0, 248, 0, 0);
    CHECK(px(540, 300) == (c15(0, 31, 0) | 0x8000), "mask set: %04x", px(540, 300));
    PC_GpuWriteGP0(0xe6000002);
    tile(540, 300, 1, 1, 248, 0, 0, 0);
    CHECK(px(540, 300) == (c15(0, 31, 0) | 0x8000), "mask check: %04x", px(540, 300));
    PC_GpuWriteGP0(0xe6000000);

    /* OT order: ClearOTagR + DrawOTag(&ot[n-1]) draws high slots first */
    u_long *ot2 = palloc(8 * sizeof(u_long));
    ClearOTagR(ot2, 8);
    TILE *red = palloc(sizeof *red), *green = palloc(sizeof *green);
    setTile(red);
    setXY0(red, 600, 300);
    setWH(red, 10, 10);
    setRGB0(red, 248, 0, 0);
    setTile(green);
    setXY0(green, 605, 305);
    setWH(green, 10, 10);
    setRGB0(green, 0, 248, 0);
    addPrim(&ot2[1], green);
    addPrim(&ot2[5], red);
    DrawOTag(&ot2[7]);
    CHECK(px(607, 307) == c15(0, 31, 0) && px(601, 301) == c15(31, 0, 0), "OT order");

    /* re-entrant DrawOTag from the DrawSync callback is queued, not recursed */
    u_long *ot3 = palloc(2 * sizeof(u_long));
    ClearOTagR(ot3, 2);
    TILE *blue = palloc(sizeof *blue);
    setTile(blue);
    setXY0(blue, 620, 300);
    setWH(blue, 2, 2);
    setRGB0(blue, 0, 0, 248);
    addPrim(&ot3[1], blue);
    s_cb_ot = &ot3[1];
    s_cb_redraw = 1;
    int before = s_cb_count;
    DrawOTag(&ot2[7]);
    CHECK(px(620, 300) == c15(0, 0, 31), "callback-queued DrawOTag not drawn");
    CHECK(s_cb_count == before + 2, "callback ran %d times (want 2)", s_cb_count - before);

    /* oversized polygons are discarded */
    setRECT(&r, 0, 490, 1024, 20);
    ClearImage(&r, 0, 0, 0);
    POLY_F3 *big = palloc(sizeof *big);
    setPolyF3(big);
    setXY3(big, 0, 490, 1024, 490, 0, 500);
    setRGB0(big, 255, 255, 255);
    DrawPrim(big);
    CHECK(px(10, 492) == 0, "1024-wide triangle was drawn");

    /* gouraud: constant-colour G3 equals flat, gradient is monotonic */
    POLY_G3 *g3 = palloc(sizeof *g3);
    setPolyG3(g3);
    setXY3(g3, 300, 490, 340, 490, 300, 510);
    setRGB0(g3, 0, 0, 0);
    setRGB1(g3, 255, 0, 0);
    setRGB2(g3, 0, 0, 0);
    DrawPrim(g3);
    int mono = 1;
    for (int x = 301; x < 320; x++)
        if ((px(x, 492) & 31) < (px(x - 1, 492) & 31))
            mono = 0;
    CHECK(mono && (px(318, 492) & 31) > 10, "gouraud ramp not monotonic/too dark");

    /* clipping to the drawing area */
    DR_AREA *da = palloc(sizeof *da);
    setRECT(&r, 640, 300, 10, 10);
    SetDrawArea(da, &r);
    DrawPrim(da);
    tile(630, 295, 30, 30, 0, 248, 248, 0);
    CHECK(px(640, 300) == c15(0, 31, 31) && px(649, 309) == c15(0, 31, 31), "clip inside");
    CHECK(px(639, 300) != c15(0, 31, 31) && px(650, 300) != c15(0, 31, 31) &&
              px(640, 310) != c15(0, 31, 31), "clip outside");
    PutDrawEnv(de);
}

static void display_env_tests(void)
{
    DISPENV *dd = palloc(sizeof *dd);
    PC_GpuDisplay d;

    SetDefDispEnv(dd, 0, 0, 320, 240);
    PutDispEnv(dd);
    SetDispMask(1);
    PC_GpuGetDisplay(&d);
    uint32_t st = PC_GpuReadStat();
    CHECK(((st >> 17) & 3) == 1 && !(st & (1u << 23)), "GPUSTAT %08x", st);
    CHECK(d.w == 320 && d.h == 240 && !d.rgb24 && d.enabled, "display %dx%d", d.w, d.h);

    DISPENV hi;
    SetDefDispEnv(&hi, 0, 0, 640, 480);
    hi.isinter = 1;
    PutDispEnv(&hi);
    PC_GpuGetDisplay(&d);
    CHECK(d.w == 640 && d.h == 480 && d.interlaced, "640x480i -> %dx%d", d.w, d.h);

    DISPENV rgb;
    SetDefDispEnv(&rgb, 0, 0, 320, 240);
    rgb.isrgb24 = 1;
    PutDispEnv(&rgb);
    PC_GpuGetDisplay(&d);
    CHECK(d.rgb24 && (PC_GpuReadStat() & (1u << 21)), "24-bit mode");

    DISPENV out;
    PutDispEnv(dd);
    GetDispEnv(&out);
    CHECK(memcmp(&out, dd, sizeof out) == 0, "GetDispEnv");
}

int main(void)
{
    if (!getenv("THPS_HEADLESS"))
        _putenv("THPS_HEADLESS=1");
    setvbuf(stdout, NULL, _IONBF, 0);

    void *ram = VirtualAlloc((void *)0x80000000, 0x200000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (ram != (void *)0x80000000) {
        printf("cannot map PS1 RAM at 0x80000000 (got %p, error %lu)\n", ram, GetLastError());
        return 2;
    }
    s_heap = (uint8_t *)0x80010000;

    _mkdir("build");
    _mkdir("build/pc");
    _mkdir(OUT_DIR);

    PC_GpuInit(2);
    ResetGraph(0);
    DrawSyncCallback(drawsync_cb);

    CHECK(sizeof(DRAWENV) == 0x5c, "sizeof(DRAWENV) = %u", (unsigned)sizeof(DRAWENV));
    CHECK(sizeof(DISPENV) == 20, "sizeof(DISPENV) = %u", (unsigned)sizeof(DISPENV));
    CHECK(sizeof(POLY_GT4) == 52 && sizeof(P_TAG) == 8, "primitive sizes");

    DRAWENV *de = palloc(sizeof *de);
    SetDefDrawEnv(de, 0, 0, 320, 240);
    de->isbg = 1;
    setRGB0(de, 0, 0, 64);
    PutDrawEnv(de);
    CHECK(px(5, 5) == c15(0, 0, 8), "isbg clear: %04x", px(5, 5));

    display_env_tests();
    load_textures();
    scene_display();

    PC_GpuDumpDisplay(OUT_DIR "/display.bmp");

    exact_tests();
    PC_GpuDumpVram(OUT_DIR "/vram.bmp");

    if (!PC_GpuIsHeadless()) {
        for (int i = 0; i < 120; i++) {
            PC_GpuPresent();
            Sleep(16);
        }
    }
    PC_GpuShutdown();

    printf("%d/%d checks passed\n", s_checks - s_fails, s_checks);
    return s_fails ? 1 : 0;
}

/* The screens and what the window shows (pc_screen.h). */
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <libgpu.h>

#include "pc_gpu_gl.h"
#include "pc_runtime.h"
#include "pc_screen.h"

#define SPACE_W 1024
#define SPACE_H 512

void PC_ScreenFill(int x, int y, int w, int h, uint32_t rgb) {
    int x1 = x + w, y1 = y + h;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x1 > SPACE_W)
        x1 = SPACE_W;
    if (y1 > SPACE_H)
        y1 = SPACE_H;
    if (x1 > x && y1 > y)
        PC_GlFill(x, y, x1 - x, y1 - y, rgb);
}

void PC_ScreenBlit(int x, int y, int w, int h, const void *src) {
    if (w > 0 && h > 0)
        PC_GlBlit(x, y, w, h, (const uint16_t *)src);
}

void PC_ScreenBlitHiRes(int x, int y, int w, int h, const void *src, int sw, int sh) {
    PC_GlHiResImage(x, y, w, h, (const uint16_t *)src, sw, sh);
}

void PC_ScreenRead(int x, int y, int w, int h, void *dst) {
    if (w > 0 && h > 0)
        PC_GlRead(x, y, w, h, (uint16_t *)dst);
}

static uint32_t *s_image;
static int s_w, s_h, s_cap, s_on;

void PC_ScreenShowImage(const uint32_t *argb, int w, int h) {
    if (w <= 0 || h <= 0)
        return;
    if (w * h > s_cap) {
        s_cap = w * h;
        s_image = realloc(s_image, (size_t)s_cap * 4);
    }
    s_w = w;
    s_h = h;
    memcpy(s_image, argb, (size_t)w * h * 4);
    s_on = 1;
}

void PC_ScreenShowGame(void) {
    s_on = 0;
}

const uint32_t *PC_ScreenImage(int *w, int *h) {
    if (!s_on)
        return NULL;
    *w = s_w;
    *h = s_h;
    return s_image;
}

/* ---- the display ---- */

static PcDisplay s_disp;

static int ClampI(int v, int lo, int hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

/* The picture an NTSC TV shows of a display mode (libgpu's PutDispEnv): the horizontal resolution
 * class of the width, the visible width and lines from the screen rectangle (0: the whole
 * picture, 2560 GPU clocks and 240 lines), doubled lines interlaced. */
void PC_ScreenShow(const void *dispenv) {
    static const int hres[4] = {256, 320, 512, 640};
    static const int div[4] = {10, 8, 5, 4};
    const DISPENV *env = (const DISPENV *)dispenv;
    int hs = 0x260 + env->screen.x * 10;
    int he = hs + (env->screen.w ? env->screen.w * 10 : 2560);
    int vs = 0x10 + env->screen.y;
    int ve = vs + (env->screen.h ? env->screen.h : 240);
    int w, dv, rw, lines;
    hs = ClampI(hs, 500, 3290);
    he = ClampI(he, hs + 80, 3290);
    vs = ClampI(vs, 0x10, 0x106);
    ve = ClampI(ve, vs + 2, 0x108);
    if (env->disp.w <= 280) {
        w = hres[0];
        dv = div[0];
    } else if (env->disp.w <= 352) {
        w = hres[1];
        dv = div[1];
    } else if (env->disp.w <= 400) {
        w = 368;
        dv = 7;
    } else if (env->disp.w <= 560) {
        w = hres[2];
        dv = div[2];
    } else {
        w = hres[3];
        dv = div[3];
    }
    rw = ((he - hs) / dv + 2) & ~3;
    if (rw > 0 && rw < w)
        w = rw;
    lines = ve - vs;
    if (lines <= 0 || lines > 256)
        lines = 240;
    s_disp.interlaced = env->disp.h > 256;
    s_disp.x = env->disp.x & 0x3ff;
    s_disp.y = env->disp.y & 0x1ff;
    s_disp.w = w;
    s_disp.h = s_disp.interlaced ? lines * 2 : lines;
    if (env->isrgb24) {
        static int warned;
        if (!warned++)
            PC_Log("screen: a 24-bit display is not supported (movies show a picture instead)");
    } else {
        PC_GlNoteScreen(env->disp.x, env->disp.y, env->disp.w, env->disp.h);
    }
}

void PC_ScreenEnable(int on) {
    s_disp.enabled = on != 0;
}

void PC_ScreenReset(void) {
    memset(&s_disp, 0, sizeof s_disp);
    s_disp.w = 256;
    s_disp.h = 240;
}

void PC_ScreenGetDisplay(PcDisplay *d) {
    *d = s_disp;
}

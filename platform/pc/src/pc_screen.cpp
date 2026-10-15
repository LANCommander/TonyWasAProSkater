/* The screens and what the window shows (pc_screen.h). */
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <libgpu.h>

#include "pc_runtime.h"
#include "pc_screen.h"
#include "pc/gl_gpu.h"
#include "pc/screen.h"

namespace pc {

constexpr int SPACE_W = 1024;
constexpr int SPACE_H = 512;

Screens g_screens;

void Screens::Fill(int x, int y, int w, int h, uint32_t rgb) {
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
        g_glGpu.Fill(x, y, x1 - x, y1 - y, rgb);
}

void Screens::Blit(int x, int y, int w, int h, const void *src) {
    if (w > 0 && h > 0)
        g_glGpu.Blit(x, y, w, h, (const uint16_t *)src);
}

void Screens::BlitHiRes(int x, int y, int w, int h, const void *src, int sw, int sh) {
    g_glGpu.HiResImage(x, y, w, h, (const uint16_t *)src, sw, sh);
}

void Screens::Read(int x, int y, int w, int h, void *dst) {
    if (w > 0 && h > 0)
        g_glGpu.Read(x, y, w, h, (uint16_t *)dst);
}

void Screens::ShowImage(const uint32_t *argb, int w, int h) {
    if (w <= 0 || h <= 0)
        return;
    if (w * h > m_imageCap) {
        m_imageCap = w * h;
        m_image = (uint32_t *)realloc(m_image, (size_t)m_imageCap * 4);
    }
    m_imageW = w;
    m_imageH = h;
    memcpy(m_image, argb, (size_t)w * h * 4);
    m_imageOn = 1;
}

const uint32_t *Screens::Image(int *w, int *h) const {
    if (!m_imageOn)
        return NULL;
    *w = m_imageW;
    *h = m_imageH;
    return m_image;
}

/* ---- the display ---- */

static int ClampI(int v, int lo, int hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

/* The picture an NTSC TV shows of a display mode (libgpu's PutDispEnv): the horizontal resolution
 * class of the width, the visible width and lines from the screen rectangle (0: the whole
 * picture, 2560 GPU clocks and 240 lines), doubled lines interlaced. */
void Screens::Show(const void *dispenv) {
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
    m_disp.interlaced = env->disp.h > 256;
    m_disp.x = env->disp.x & 0x3ff;
    m_disp.y = env->disp.y & 0x1ff;
    m_disp.w = w;
    m_disp.h = m_disp.interlaced ? lines * 2 : lines;
    if (env->isrgb24) {
        if (!m_warned24++)
            PC_Log("screen: a 24-bit display is not supported (movies show a picture instead)");
    } else {
        g_glGpu.NoteScreen(env->disp.x, env->disp.y, env->disp.w, env->disp.h);
    }
}

void Screens::Reset() {
    memset(&m_disp, 0, sizeof m_disp);
    m_disp.w = 256;
    m_disp.h = 240;
}

}   // namespace pc

/* ---- the game's interface (pc_screen.h) ---- */

extern "C" {

void PC_ScreenFill(int x, int y, int w, int h, uint32_t rgb) { pc::g_screens.Fill(x, y, w, h, rgb); }
void PC_ScreenBlit(int x, int y, int w, int h, const void *src) { pc::g_screens.Blit(x, y, w, h, src); }
void PC_ScreenBlitHiRes(int x, int y, int w, int h, const void *src, int sw, int sh) { pc::g_screens.BlitHiRes(x, y, w, h, src, sw, sh); }
void PC_ScreenRead(int x, int y, int w, int h, void *dst) { pc::g_screens.Read(x, y, w, h, dst); }
void PC_ScreenShow(const void *dispenv) { pc::g_screens.Show(dispenv); }
void PC_ScreenEnable(int on) { pc::g_screens.Enable(on); }
void PC_ScreenReset(void) { pc::g_screens.Reset(); }

}

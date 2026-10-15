/* The game window and presentation of the PS1 display area, 4:3 letterboxed, plus BMP dumps of
 * VRAM and the display for headless debugging. The window has an OpenGL 3.3 context and the GPU
 * draws through pc_gpu_gl.cpp, the screens at the window's resolution (or at THPS_GL_SCALE times the
 * PS1's). Windowed, borderless or fullscreen comes from Settings.ini (pc_video.h). OpenGL 3.3 is
 * required. THPS_HEADLESS=1 draws into a hidden window of THPS_SCALE times 320x240, whatever
 * Settings.ini says. THPS_FILTER=1 selects linear scaling. Event pumping is the input layer's
 * job; it hands window events to Video::Event. */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "pc_render3d.h"
#include "pc_draw.h"
#include "pc_runtime.h"
#include "pc_screen.h"
#include "pc_texstore.h"
#include "pc_video.h"
#include "pc/video.h"
#include "pc/gl_gpu.h"
#include "pc/screen.h"
#include "pc/settings.h"
#include "pc/texture_store.h"

extern volatile uint32_t pc_heartbeat;

namespace pc {

Video g_video;

namespace {

const char *const s_modeNames[PC_VIDEO_MODES] = { "Windowed", "Borderless", "Fullscreen" };
const char *const s_fogNames[PC_FOG_MODES] = { "Normal", "Light", "Off" };

int ClampDistance(int d)
{
    return d < PC_DISTANCE_MIN ? PC_DISTANCE_MIN : d > PC_DISTANCE_MAX ? PC_DISTANCE_MAX : d;
}

/* a draw distance: percent, or "Full" for the entire level */
int ParseDrawDistance(const char *v)
{
    while (*v == ' ' || *v == '\t')
        v++;
    return SDL_strncasecmp(v, "full", 4) ? ClampDistance(atoi(v)) : PC_DISTANCE_FULL;
}

/* The display mode w*h, preferring 60 Hz (the game's rate), else the fastest. */
bool FindMode(int display, int w, int h, SDL_DisplayMode *out)
{
    int n = SDL_GetNumDisplayModes(display);
    bool found = false;
    for (int i = 0; i < n; i++) {
        SDL_DisplayMode m;
        if (SDL_GetDisplayMode(display, i, &m) || m.w != w || m.h != h)
            continue;
        if (!found || (m.refresh_rate == 60 && out->refresh_rate != 60) ||
            (out->refresh_rate != 60 && m.refresh_rate > out->refresh_rate))
            *out = m;
        found = true;
    }
    return found;
}

/* ---- BMP dumps (24-bit, bottom-up) ---- */

void PutLe(uint8_t *p, uint32_t v, int n)
{
    for (int i = 0; i < n; i++)
        p[i] = (uint8_t)(v >> (8 * i));
}

int WriteBmp(const char *path, const uint32_t *argb, int pitch, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "[video] cannot write %s\n", path);
        return -1;
    }
    int stride = (w * 3 + 3) & ~3;
    uint8_t hdr[54];
    memset(hdr, 0, sizeof hdr);
    hdr[0] = 'B';
    hdr[1] = 'M';
    PutLe(hdr + 2, (uint32_t)(54 + stride * h), 4);
    PutLe(hdr + 10, 54, 4);
    PutLe(hdr + 14, 40, 4);
    PutLe(hdr + 18, (uint32_t)w, 4);
    PutLe(hdr + 22, (uint32_t)h, 4);
    PutLe(hdr + 26, 1, 2);
    PutLe(hdr + 28, 24, 2);
    PutLe(hdr + 34, (uint32_t)(stride * h), 4);
    PutLe(hdr + 38, 2835, 4);
    PutLe(hdr + 42, 2835, 4);
    fwrite(hdr, 1, sizeof hdr, f);

    uint8_t *row = (uint8_t *)calloc((size_t)stride, 1);
    for (int y = h - 1; y >= 0; y--) {
        const uint32_t *s = argb + (size_t)y * pitch;
        for (int x = 0; x < w; x++) {
            row[x * 3 + 0] = (uint8_t)s[x];
            row[x * 3 + 1] = (uint8_t)(s[x] >> 8);
            row[x * 3 + 2] = (uint8_t)(s[x] >> 16);
        }
        fwrite(row, 1, (size_t)stride, f);
    }
    free(row);
    fclose(f);
    return 0;
}

inline uint32_t C15ToArgb(uint32_t c)
{
    uint32_t r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return 0xff000000u | (r << 16) | (g << 8) | b;
}

}   // namespace

/* ---- settings ---- */

void Video::SettingsDefaults()
{
    memset(&m_set, 0, sizeof m_set);
    m_set.mode = PC_VIDEO_WINDOWED;
    m_set.win_w = 960;
    m_set.win_h = 720;
    m_set.widescreen = 1;
    m_set.draw_distance = 100;
    m_set.object_distance = 100;
    m_set.fog = PC_FOG_NORMAL;
}

int Video::SettingsLine(const char *section, const char *key, char *value)
{
    int i, w, h;
    if (SDL_strcasecmp(section, "Video"))
        return 0;
    while (*value == ' ' || *value == '\t')
        value++;
    if (!SDL_strcasecmp(key, "Mode")) {
        for (i = 0; i < PC_VIDEO_MODES; i++)
            if (!SDL_strncasecmp(value, s_modeNames[i], strlen(s_modeNames[i])))
                m_set.mode = i;
    } else if (!SDL_strcasecmp(key, "WindowSize") || !SDL_strcasecmp(key, "FullscreenResolution")) {
        if (sscanf(value, "%dx%d", &w, &h) == 2 && w >= 320 && h >= 240 && w <= 16384 &&
            h <= 16384) {
            if (!SDL_strcasecmp(key, "WindowSize")) {
                m_set.win_w = w;
                m_set.win_h = h;
            } else {
                m_set.full_w = w;
                m_set.full_h = h;
            }
        }
    } else if (!SDL_strcasecmp(key, "Widescreen")) {
        m_set.widescreen = !SDL_strncasecmp(value, "on", 2) || *value == '1' ||
                           !SDL_strncasecmp(value, "yes", 3);
    } else if (!SDL_strcasecmp(key, "DrawDistance")) {
        m_set.draw_distance = ParseDrawDistance(value);
    } else if (!SDL_strcasecmp(key, "ObjectDistance")) {
        m_set.object_distance = ClampDistance(atoi(value));
    } else if (!SDL_strcasecmp(key, "Fog")) {
        for (i = 0; i < PC_FOG_MODES; i++)
            if (!SDL_strncasecmp(value, s_fogNames[i], strlen(s_fogNames[i])))
                m_set.fog = i;
    } else {
        PC_Log("Settings.ini: unknown setting \"%s\" in [Video]", key);
    }
    if (m_set.mode != PC_VIDEO_WINDOWED)
        m_lastFull = m_set.mode;
    return 1;
}

void Video::WriteSettings(FILE *f)
{
    fputs("\n; Display. Mode = Windowed, Borderless (fullscreen at the desktop's resolution) or\n"
          "; Fullscreen (switches the display to FullscreenResolution; empty: the desktop's).\n"
          "; WindowSize is the window's drawing area. Either way the game is drawn at the\n"
          "; resolution it is shown at. Widescreen = On shows more of the world at the sides on\n"
          "; screens wider than 4:3 (the menus and the 2D overlays stay 4:3); Off keeps the\n"
          "; game 4:3 with black bars. Alt+Enter switches between the window and fullscreen.\n"
          "; DrawDistance is how far the level is drawn, in percent of the original (50-400;\n"
          "; the fog moves out with it), or Full for the entire level (the fog then reaches\n"
          "; across it); ObjectDistance how far models keep their detail and stay visible\n"
          "; (50-400). Fog = Normal, Light (only the far half) or Off.\n",
          f);
    fprintf(f, "\n[Video]\nMode = %s\nWindowSize = %dx%d\n", s_modeNames[m_set.mode], m_set.win_w,
            m_set.win_h);
    if (m_set.full_w > 0)
        fprintf(f, "FullscreenResolution = %dx%d\n", m_set.full_w, m_set.full_h);
    else
        fputs("FullscreenResolution =\n", f);
    fprintf(f, "Widescreen = %s\n", m_set.widescreen ? "On" : "Off");
    if (m_set.draw_distance == PC_DISTANCE_FULL)
        fputs("DrawDistance = Full\n", f);
    else
        fprintf(f, "DrawDistance = %d\n", m_set.draw_distance);
    fprintf(f, "ObjectDistance = %d\nFog = %s\n", m_set.object_distance, s_fogNames[m_set.fog]);
}

int Video::Save()
{
    return g_settings.Save();
}

/* ---- draw distance ---- */

/* THPS_DRAWDIST (percent or "full"), THPS_OBJDIST and THPS_FOG (0-2) override Settings.ini, for
 * tests */
void Video::DrawEnv()
{
    const char *e;
    if (m_envDone)
        return;
    m_envDone = true;
    if ((e = getenv("THPS_DRAWDIST")))
        m_set.draw_distance = ParseDrawDistance(e);
    if ((e = getenv("THPS_OBJDIST")))
        m_set.object_distance = ClampDistance(atoi(e));
    if ((e = getenv("THPS_FOG")) && atoi(e) >= 0 && atoi(e) < PC_FOG_MODES)
        m_set.fog = atoi(e);
}

bool Video::Full()
{
    DrawEnv();
    return m_set.draw_distance == PC_DISTANCE_FULL && R3d_Native();
}

/* how far the fog ends: DpqMax times the draw distance; for the entire level the far side of the
 * level (at least 4 times) */
double Video::FogEnd(int dpqmax)
{
    double end;
    if (m_set.draw_distance != PC_DISTANCE_FULL)
        return (double)dpqmax * m_set.draw_distance / 100;
    end = 4.0 * dpqmax;
    return m_levelSize > end ? m_levelSize : end;
}

int Video::ScaledFar(int dpqmax)
{
    double far = FogEnd(dpqmax);
    return far > 0x7fff ? 0x7fff : (int)far;
}

int Video::FarPlane(int dpqmax)
{
    DrawEnv();
    return m_set.draw_distance == 100 ? dpqmax : ScaledFar(dpqmax);
}

/* fog as the level sets it */
bool Video::FogRetail()
{
    DrawEnv();
    return m_set.draw_distance == 100 && m_set.fog == PC_FOG_NORMAL;
}

/* the fog's range with the settings; it ends at FogEnd */
double Video::FogRange(int dpqmin, int dpqmax)
{
    double range = (dpqmax - dpqmin) * FogEnd(dpqmax) / (dpqmax > 0 ? dpqmax : 1);
    if (m_set.fog == PC_FOG_LIGHT)
        range /= 2;
    return range < 1 ? 1 : range;
}

int Video::FogStart(int dpqmin, int dpqmax)
{
    if (FogRetail())
        return dpqmin;
    if (m_set.fog == PC_FOG_OFF)
        return INT_MAX;
    return (int)(FogEnd(dpqmax) - FogRange(dpqmin, dpqmax));
}

int Video::FogMin(int dpqmin, int dpqmax)
{
    int start = FogStart(dpqmin, dpqmax);
    return start > 0x7fff ? 0x7fff : start;   /* nothing nearer is fogged */
}

/* as M3dInit_SetFoggingParams: the shift that takes the range to 4096 */
int Video::FogShift(int dpqmin, int dpqmax, int dpqshift)
{
    double range;
    int shift = 0;
    if (FogRetail())
        return dpqshift;
    if (m_set.fog == PC_FOG_OFF)
        return 0;
    range = FogRange(dpqmin, dpqmax);
    for (; range < 4096; range *= 2)
        shift++;
    for (; range > 4096; range /= 2)
        shift--;
    return shift;
}

int Video::ObjectDistance(int d)
{
    DrawEnv();
    return m_set.object_distance == 100 ? d : (int)((int64_t)d * m_set.object_distance / 100);
}

unsigned Video::SuspendDistance(unsigned d)
{
    uint64_t far;
    DrawEnv();
    if (m_set.draw_distance <= 100)
        return d;
    if (m_set.draw_distance == PC_DISTANCE_FULL)
        return 0xffffffffu;
    far = (uint64_t)d * (unsigned)m_set.draw_distance / 100;
    return far > 0xffffffffu ? 0xffffffffu : (unsigned)far;
}

/* ---- window modes ---- */

int Video::DisplayIndex()
{
    int d = m_window ? SDL_GetWindowDisplayIndex(m_window) : 0;
    return d < 0 ? 0 : d;
}

void Video::ApplyWindow()
{
    SDL_Window *w = m_window;
    int d = DisplayIndex();
    if (!w || m_headless)
        return;
    int full = (SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN) != 0;
    switch (m_set.mode) {
    case PC_VIDEO_WINDOWED:
        if (full)
            SDL_SetWindowFullscreen(w, 0);
        SDL_RestoreWindow(w);
        SDL_SetWindowBordered(w, SDL_TRUE);
        SDL_SetWindowResizable(w, SDL_TRUE);
        SDL_SetWindowSize(w, m_set.win_w, m_set.win_h);
        SDL_SetWindowPosition(w, SDL_WINDOWPOS_CENTERED_DISPLAY(d), SDL_WINDOWPOS_CENTERED_DISPLAY(d));
        break;
    case PC_VIDEO_BORDERLESS:
        SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN_DESKTOP);
        break;
    case PC_VIDEO_FULLSCREEN: {
        SDL_DisplayMode m;
        if (m_set.full_w <= 0 || !FindMode(d, m_set.full_w, m_set.full_h, &m)) {
            if (m_set.full_w > 0)
                PC_Log("video: no %dx%d display mode, using the desktop's", m_set.full_w,
                       m_set.full_h);
            if (SDL_GetDesktopDisplayMode(d, &m))
                break;
        }
        if (full)
            SDL_SetWindowFullscreen(w, 0);   /* a new mode only takes when entering fullscreen */
        SDL_SetWindowDisplayMode(w, &m);
        if (SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN))
            PC_Log("video: fullscreen %dx%d failed: %s", m.w, m.h, SDL_GetError());
        break;
    }
    }
    SDL_ShowCursor(m_set.mode == PC_VIDEO_WINDOWED ? SDL_ENABLE : SDL_DISABLE);
}

void Video::Apply(const PcVideoSettings &s)
{
    m_set = s;
    if (m_set.mode < 0 || m_set.mode >= PC_VIDEO_MODES)
        m_set.mode = PC_VIDEO_WINDOWED;
    if (m_set.mode != PC_VIDEO_WINDOWED)
        m_lastFull = m_set.mode;
    g_glGpu.SetWidescreen(m_set.widescreen);
    ApplyWindow();
}

void Video::AddRes(int w, int h)
{
    for (int i = 0; i < m_nres; i++)
        if (m_res[i].w == w && m_res[i].h == h)
            return;
    if (m_nres < (int)(sizeof m_res / sizeof m_res[0])) {
        m_res[m_nres].w = w;
        m_res[m_nres].h = h;
        m_nres++;
    }
}

/* The resolutions a mode offers, smallest first: the display's modes (windowed: those that fit
 * on the desktop, plus the current window size), or just the desktop's for borderless. */
int Video::ResolutionCount(int mode)
{
    int d = DisplayIndex();
    SDL_DisplayMode m;
    SDL_Rect usable;
    m_nres = 0;
    m_resMode = mode;
    if (mode == PC_VIDEO_BORDERLESS) {
        if (!SDL_GetDesktopDisplayMode(d, &m))
            AddRes(m.w, m.h);
        return m_nres;
    }
    if (SDL_GetDisplayUsableBounds(d, &usable)) {
        usable.w = usable.h = 1 << 30;
    }
    for (int i = 0, n = SDL_GetNumDisplayModes(d); i < n; i++) {
        if (SDL_GetDisplayMode(d, i, &m) || m.w < 640 || m.h < 480)
            continue;
        /* a window also needs room for its title bar */
        if (mode == PC_VIDEO_WINDOWED && (m.w > usable.w || m.h + 40 > usable.h))
            continue;
        AddRes(m.w, m.h);
    }
    if (mode == PC_VIDEO_WINDOWED)
        AddRes(m_set.win_w, m_set.win_h);
    qsort(m_res, (size_t)m_nres, sizeof m_res[0], [](const void *a, const void *b) {
        const Res *x = (const Res *)a, *y = (const Res *)b;
        return x->w != y->w ? x->w - y->w : x->h - y->h;
    });
    return m_nres;
}

void Video::Resolution(int mode, int i, int *w, int *h)
{
    if (mode != m_resMode)
        ResolutionCount(mode);
    if (i < 0 || i >= m_nres) {
        *w = *h = 0;
        return;
    }
    *w = m_res[i].w;
    *h = m_res[i].h;
}

void Video::Event(const void *sdlEvent)
{
    const SDL_Event *e = (const SDL_Event *)sdlEvent;
    if (!m_window || m_headless)
        return;
    if (e->type == SDL_KEYDOWN && !e->key.repeat && e->key.keysym.sym == SDLK_RETURN &&
        (e->key.keysym.mod & KMOD_ALT)) {
        PcVideoSettings s = m_set;
        s.mode = s.mode == PC_VIDEO_WINDOWED ? m_lastFull : PC_VIDEO_WINDOWED;
        Apply(s);
        Save();
    } else if (e->type == SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED &&
               m_set.mode == PC_VIDEO_WINDOWED &&
               !(SDL_GetWindowFlags(m_window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED))) {
        /* the window was resized by hand: keep that size */
        SDL_GetWindowSize(m_window, &m_set.win_w, &m_set.win_h);
    }
}

/* ---- creation ---- */

/* The OpenGL window and renderer; false if there is no OpenGL 3.3. */
bool Video::InitGl(int scale)
{
    const char *e = getenv("THPS_GL_SCALE");
    int fixed = e && atoi(e) > 0 ? atoi(e) : 0;
    int w = m_headless ? 320 * scale : m_set.win_w, h = m_headless ? 240 * scale : m_set.win_h;
    const char *ws = getenv("THPS_WINDOW");
    if (ws && sscanf(ws, "%dx%d", &w, &h) == 2 && !m_headless) {
        m_set.mode = PC_VIDEO_WINDOWED;
        m_set.win_w = w;
        m_set.win_h = h;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    m_window = SDL_CreateWindow("Tony Hawk's Pro Skater", SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, w, h,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                    (m_headless ? SDL_WINDOW_HIDDEN : 0));
    if (m_window)
        m_glctx = SDL_GL_CreateContext(m_window);
    if (m_glctx) {
        int dw, dh;
        /* the port paces itself to the PS1's 60 Hz; waiting for the monitor as well would stall */
        SDL_GL_SetSwapInterval(0);
        ApplyWindow();
        SDL_GL_GetDrawableSize(m_window, &dw, &dh);
        if (getenv("THPS_WIDESCREEN"))
            m_set.widescreen = atoi(getenv("THPS_WIDESCREEN")) != 0;
        g_glGpu.SetWidescreen(m_set.widescreen);
        if (g_glGpu.Init(fixed, dw, dh) == 0)
            return true;
        g_glGpu.Shutdown();
        SDL_GL_DeleteContext(m_glctx);
        m_glctx = nullptr;
    }
    if (m_window)
        SDL_DestroyWindow(m_window);
    m_window = nullptr;
    return false;
}

void Video::Init(int scale)
{
    const char *h = getenv("THPS_HEADLESS");
    PC_DrawReset();
    PC_ScreenReset();
    m_headless = h && h[0] && h[0] != '0';
    if (m_window)
        return;
    if (scale < 1)
        scale = 2;
    g_settings.Load();

    /* sizes in pixels, not scaled by the desktop's DPI setting: 640x480 is 640x480 */
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
        PC_Fatal("Cannot open a window (SDL: %s).", SDL_GetError());
    m_videoInited = true;
    if (!InitGl(scale))
        PC_Fatal("Tony Hawk's Pro Skater needs a graphics card with OpenGL 3.3 (%s). Updating "
                 "the graphics driver may help.", SDL_GetError());
}

void Video::Shutdown()
{
    if (m_glctx) {
        g_glGpu.Shutdown();
        SDL_GL_DeleteContext(m_glctx);
        m_glctx = nullptr;
    }
    if (m_window)
        SDL_DestroyWindow(m_window);
    m_window = nullptr;
    if (m_videoInited) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        m_videoInited = false;
    }
}

void Video::Present()
{

    {
        if (m_gpuDumpFrom == -2) {
            const char *e = getenv("THPS_GPUDUMP");
            if (!e || sscanf(e, "%d-%d", &m_gpuDumpFrom, &m_gpuDumpTo) != 2)
                m_gpuDumpFrom = m_gpuDumpTo = -1;
        }
        if (m_gpuDumpFrom >= 0 && (int)pc_heartbeat >= m_gpuDumpFrom && (int)pc_heartbeat <= m_gpuDumpTo) {
            DisplayState dd;
            dd = g_screens.Display();
            fprintf(stderr, "present f%u disp_x=%d disp_y=%d\n", pc_heartbeat, dd.x, dd.y);
        }
    }

    DisplayState d;
    int iw = 0, ih = 0;
    const uint32_t *image = g_screens.Image(&iw, &ih);
    d = g_screens.Display();
    g_glGpu.Present(m_window, d.x, d.y, d.w, d.h, d.enabled, image, iw, ih);
}

/* ---- BMP dumps ---- */

int Video::DumpSpace(const char *path)
{
    uint16_t *v = (uint16_t *)malloc((size_t)kTexW * kTexH * 2);
    uint32_t *buf = (uint32_t *)malloc((size_t)kTexW * kTexH * sizeof(uint32_t));
    if (!buf || !v) {
        free(v);
        free(buf);
        return -1;
    }
    PC_ScreenRead(0, 0, kTexW, kTexH, v);
    /* the screens, and the textures where the texture store holds some (pc_texstore.h) */
    const uint16_t *tex = g_textureStore.Data();
    const uint8_t *cell = g_textureStore.Cells();
    for (int i = 0; i < kTexW * kTexH; i++) {
        int x = i % kTexW, y = i / kTexW;
        int used = cell[(y / kTexCell) * (kTexW / kTexCell) + x / kTexCell] & kTexUsed;
        buf[i] = C15ToArgb(used ? tex[i] : v[i]);
    }
    int rc = WriteBmp(path, buf, kTexW, kTexW, kTexH);
    free(buf);
    free(v);
    return rc;
}

int Video::DumpDisplay(const char *path)
{
    int iw, ih;
    const uint32_t *image = g_screens.Image(&iw, &ih);
    if (image)
        return WriteBmp(path, image, iw, iw, ih);
    DisplayState d;
    d = g_screens.Display();
    int w = d.w < kTexW ? d.w : kTexW, h = d.h < kTexH ? d.h : kTexH;
    uint16_t *px = (uint16_t *)malloc((size_t)w * h * 2);
    uint32_t *buf = (uint32_t *)malloc((size_t)w * h * sizeof(uint32_t));
    if (!buf || !px) {
        free(px);
        free(buf);
        return -1;
    }
    /* the display area at the PS1's resolution */
    PC_ScreenRead(d.x, d.y, w, h, px);
    for (int i = 0; i < w * h; i++)
        buf[i] = C15ToArgb(px[i]);
    int rc = WriteBmp(path, buf, w, w, h);
    free(buf);
    free(px);

    /* and what OpenGL drew, at its internal resolution: <path without .bmp>_gl.bmp */
    int hw = 0, hh = 0;
    uint32_t *hi = g_glGpu.ReadArea(d.x, d.y, w, h, &hw, &hh);
    if (hi && hw == w && hh == h) {
        free(hi);
        hi = nullptr;
    }
    if (hi) {
        char p2[512];
        const char *dot = strrchr(path, '.');
        int n = dot ? (int)(dot - path) : (int)strlen(path);
        snprintf(p2, sizeof p2, "%.*s_gl.bmp", n, path);
        WriteBmp(p2, hi, hw, hw, hh);
        free(hi);
    }
    return rc;
}

}   // namespace pc

/* ---- the game's interface (pc_video.h, pc_settings.h) ---- */

void PC_VideoGetSettings(PcVideoSettings *s) { *s = pc::g_video.Settings(); }
void PC_VideoApply(const PcVideoSettings *s) { pc::g_video.Apply(*s); }
int PC_VideoSave(void) { return pc::g_video.Save(); }
int PC_VideoResolutionCount(int mode) { return pc::g_video.ResolutionCount(mode); }
void PC_VideoResolution(int mode, int i, int *w, int *h) { pc::g_video.Resolution(mode, i, w, h); }


int PC_DrawFarPlane(int dpqmax) { return pc::g_video.FarPlane(dpqmax); }
int PC_DrawFogMin(int dpqmin, int dpqmax) { return pc::g_video.FogMin(dpqmin, dpqmax); }
int PC_DrawFogShift(int dpqmin, int dpqmax, int dpqshift) { return pc::g_video.FogShift(dpqmin, dpqmax, dpqshift); }
int PC_DrawFogStart(int dpqmin, int dpqmax) { return pc::g_video.FogStart(dpqmin, dpqmax); }
int PC_DrawObjectDistance(int d) { return pc::g_video.ObjectDistance(d); }
int PC_DrawFull(void) { return pc::g_video.Full(); }
void PC_DrawSetLevelSize(int size) { pc::g_video.SetLevelSize(size); }
unsigned PC_DrawSuspendDistance(unsigned d) { return pc::g_video.SuspendDistance(d); }

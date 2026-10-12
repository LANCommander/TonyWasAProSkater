/* The game window and presentation of the PS1 display area, 4:3 letterboxed, plus BMP dumps of
 * VRAM and the display for headless debugging. The window has an OpenGL 3.3 context and the GPU
 * draws through pc_gpu_gl.c, the screens at the window's resolution (or at THPS_GL_SCALE times the
 * PS1's). Windowed, borderless or fullscreen comes from Settings.ini (pc_video.h). With
 * THPS_RENDERER=sw, or without OpenGL, the software GPU draws and an SDL_Renderer streaming
 * texture shows its VRAM (always windowed). THPS_HEADLESS=1 skips the window and uses the
 * software GPU, unless THPS_RENDERER=gl (then OpenGL draws into a hidden window of THPS_SCALE
 * times 320x240, whatever Settings.ini says). THPS_FILTER=1 selects linear scaling. Event pumping
 * is the input layer's job; it hands window events to PC_VideoEvent. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "pc_gpu.h"
#include "pc_gpu_gl.h"
#include "pc_runtime.h"
#include "pc_settings.h"
#include "pc_video.h"

#define TEX_W 1024
#define TEX_H 512

static SDL_Window *s_window;
static SDL_Renderer *s_renderer;
static SDL_Texture *s_texture;
static SDL_GLContext s_glctx;
static uint32_t *s_pixels;
static int s_headless;
static int s_video_inited;

int PC_GpuIsHeadless(void)
{
    return s_headless;
}

/* ---- settings ---- */

static PcVideoSettings s_set;
static int s_last_full = PC_VIDEO_BORDERLESS;   /* where Alt+Enter goes from a window */
static const char *const s_mode_names[PC_VIDEO_MODES] = { "Windowed", "Borderless", "Fullscreen" };

void PC_VideoSettingsDefaults(void)
{
    memset(&s_set, 0, sizeof s_set);
    s_set.mode = PC_VIDEO_WINDOWED;
    s_set.win_w = 960;
    s_set.win_h = 720;
    s_set.widescreen = 1;
}

int PC_VideoSettingsLine(const char *section, const char *key, char *value)
{
    int i, w, h;
    if (SDL_strcasecmp(section, "Video"))
        return 0;
    while (*value == ' ' || *value == '\t')
        value++;
    if (!SDL_strcasecmp(key, "Mode")) {
        for (i = 0; i < PC_VIDEO_MODES; i++)
            if (!SDL_strncasecmp(value, s_mode_names[i], strlen(s_mode_names[i])))
                s_set.mode = i;
    } else if (!SDL_strcasecmp(key, "WindowSize") || !SDL_strcasecmp(key, "FullscreenResolution")) {
        if (sscanf(value, "%dx%d", &w, &h) == 2 && w >= 320 && h >= 240 && w <= 16384 &&
            h <= 16384) {
            if (!SDL_strcasecmp(key, "WindowSize")) {
                s_set.win_w = w;
                s_set.win_h = h;
            } else {
                s_set.full_w = w;
                s_set.full_h = h;
            }
        }
    } else if (!SDL_strcasecmp(key, "Widescreen")) {
        s_set.widescreen = !SDL_strncasecmp(value, "on", 2) || *value == '1' ||
                           !SDL_strncasecmp(value, "yes", 3);
    } else {
        PC_Log("Settings.ini: unknown setting \"%s\" in [Video]", key);
    }
    if (s_set.mode != PC_VIDEO_WINDOWED)
        s_last_full = s_set.mode;
    return 1;
}

void PC_VideoWriteSettings(FILE *f)
{
    fputs("\n; Display. Mode = Windowed, Borderless (fullscreen at the desktop's resolution) or\n"
          "; Fullscreen (switches the display to FullscreenResolution; empty: the desktop's).\n"
          "; WindowSize is the window's drawing area. Either way the game is drawn at the\n"
          "; resolution it is shown at. Widescreen = On shows more of the world at the sides on\n"
          "; screens wider than 4:3 (the menus and the 2D overlays stay 4:3); Off keeps the\n"
          "; game 4:3 with black bars. Alt+Enter switches between the window and fullscreen.\n",
          f);
    fprintf(f, "\n[Video]\nMode = %s\nWindowSize = %dx%d\n", s_mode_names[s_set.mode], s_set.win_w,
            s_set.win_h);
    if (s_set.full_w > 0)
        fprintf(f, "FullscreenResolution = %dx%d\n", s_set.full_w, s_set.full_h);
    else
        fputs("FullscreenResolution =\n", f);
    fprintf(f, "Widescreen = %s\n", s_set.widescreen ? "On" : "Off");
}

void PC_VideoGetSettings(PcVideoSettings *s)
{
    *s = s_set;
}

int PC_VideoSave(void)
{
    return PC_SettingsSave();
}

/* ---- window modes ---- */

static int display_index(void)
{
    int d = s_window ? SDL_GetWindowDisplayIndex(s_window) : 0;
    return d < 0 ? 0 : d;
}

/* The display mode w*h, preferring 60 Hz (the game's rate), else the fastest. */
static int find_mode(int display, int w, int h, SDL_DisplayMode *out)
{
    int n = SDL_GetNumDisplayModes(display), found = 0;
    for (int i = 0; i < n; i++) {
        SDL_DisplayMode m;
        if (SDL_GetDisplayMode(display, i, &m) || m.w != w || m.h != h)
            continue;
        if (!found || (m.refresh_rate == 60 && out->refresh_rate != 60) ||
            (out->refresh_rate != 60 && m.refresh_rate > out->refresh_rate))
            *out = m;
        found = 1;
    }
    return found;
}

static void apply_window(void)
{
    SDL_Window *w = s_window;
    int d = display_index();
    if (!w || s_headless)
        return;
    int full = (SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN) != 0;
    switch (s_set.mode) {
    case PC_VIDEO_WINDOWED:
        if (full)
            SDL_SetWindowFullscreen(w, 0);
        SDL_RestoreWindow(w);
        SDL_SetWindowBordered(w, SDL_TRUE);
        SDL_SetWindowResizable(w, SDL_TRUE);
        SDL_SetWindowSize(w, s_set.win_w, s_set.win_h);
        SDL_SetWindowPosition(w, SDL_WINDOWPOS_CENTERED_DISPLAY(d), SDL_WINDOWPOS_CENTERED_DISPLAY(d));
        break;
    case PC_VIDEO_BORDERLESS:
        SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN_DESKTOP);
        break;
    case PC_VIDEO_FULLSCREEN: {
        SDL_DisplayMode m;
        if (s_set.full_w <= 0 || !find_mode(d, s_set.full_w, s_set.full_h, &m)) {
            if (s_set.full_w > 0)
                PC_Log("video: no %dx%d display mode, using the desktop's", s_set.full_w,
                       s_set.full_h);
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
    SDL_ShowCursor(s_set.mode == PC_VIDEO_WINDOWED ? SDL_ENABLE : SDL_DISABLE);
}

void PC_VideoApply(const PcVideoSettings *s)
{
    s_set = *s;
    if (s_set.mode < 0 || s_set.mode >= PC_VIDEO_MODES)
        s_set.mode = PC_VIDEO_WINDOWED;
    if (s_set.mode != PC_VIDEO_WINDOWED)
        s_last_full = s_set.mode;
    PC_GlSetWidescreen(s_set.widescreen);
    apply_window();
}

typedef struct {
    int w, h;
} Res;

static Res s_res[256];
static int s_nres, s_res_mode = -1;

static void add_res(int w, int h)
{
    for (int i = 0; i < s_nres; i++)
        if (s_res[i].w == w && s_res[i].h == h)
            return;
    if (s_nres < (int)(sizeof s_res / sizeof s_res[0])) {
        s_res[s_nres].w = w;
        s_res[s_nres].h = h;
        s_nres++;
    }
}

static int cmp_res(const void *a, const void *b)
{
    const Res *x = (const Res *)a, *y = (const Res *)b;
    return x->w != y->w ? x->w - y->w : x->h - y->h;
}

int PC_VideoResolutionCount(int mode)
{
    int d = display_index();
    SDL_DisplayMode m;
    SDL_Rect usable;
    s_nres = 0;
    s_res_mode = mode;
    if (mode == PC_VIDEO_BORDERLESS) {
        if (!SDL_GetDesktopDisplayMode(d, &m))
            add_res(m.w, m.h);
        return s_nres;
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
        add_res(m.w, m.h);
    }
    if (mode == PC_VIDEO_WINDOWED)
        add_res(s_set.win_w, s_set.win_h);
    qsort(s_res, (size_t)s_nres, sizeof s_res[0], cmp_res);
    return s_nres;
}

void PC_VideoResolution(int mode, int i, int *w, int *h)
{
    if (mode != s_res_mode)
        PC_VideoResolutionCount(mode);
    if (i < 0 || i >= s_nres) {
        *w = *h = 0;
        return;
    }
    *w = s_res[i].w;
    *h = s_res[i].h;
}

void PC_VideoEvent(const void *sdl_event)
{
    const SDL_Event *e = (const SDL_Event *)sdl_event;
    if (!s_window || s_headless)
        return;
    if (e->type == SDL_KEYDOWN && !e->key.repeat && e->key.keysym.sym == SDLK_RETURN &&
        (e->key.keysym.mod & KMOD_ALT)) {
        PcVideoSettings s = s_set;
        s.mode = s.mode == PC_VIDEO_WINDOWED ? s_last_full : PC_VIDEO_WINDOWED;
        PC_VideoApply(&s);
        PC_VideoSave();
    } else if (e->type == SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED &&
               s_set.mode == PC_VIDEO_WINDOWED &&
               !(SDL_GetWindowFlags(s_window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED))) {
        /* the window was resized by hand: keep that size */
        SDL_GetWindowSize(s_window, &s_set.win_w, &s_set.win_h);
    }
}

/* ---- creation ---- */

/* The OpenGL window and renderer; 0 when the software path has to be used instead. */
static int init_gl(int scale)
{
    const char *e = getenv("THPS_GL_SCALE");
    int fixed = e && atoi(e) > 0 ? atoi(e) : 0;
    int w = s_headless ? 320 * scale : s_set.win_w, h = s_headless ? 240 * scale : s_set.win_h;
    const char *ws = getenv("THPS_WINDOW");
    if (ws && sscanf(ws, "%dx%d", &w, &h) == 2 && !s_headless) {
        s_set.mode = PC_VIDEO_WINDOWED;
        s_set.win_w = w;
        s_set.win_h = h;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    s_window = SDL_CreateWindow("Tony Hawk's Pro Skater", SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, w, h,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                    (s_headless ? SDL_WINDOW_HIDDEN : 0));
    if (s_window)
        s_glctx = SDL_GL_CreateContext(s_window);
    if (s_glctx) {
        int dw, dh;
        /* the port paces itself to the PS1's 60 Hz; waiting for the monitor as well would stall */
        SDL_GL_SetSwapInterval(0);
        apply_window();
        SDL_GL_GetDrawableSize(s_window, &dw, &dh);
        if (getenv("THPS_WIDESCREEN"))
            s_set.widescreen = atoi(getenv("THPS_WIDESCREEN")) != 0;
        PC_GlSetWidescreen(s_set.widescreen);
        if (PC_GlInit(fixed, dw, dh) == 0)
            return 1;
        PC_GlShutdown();
        SDL_GL_DeleteContext(s_glctx);
        s_glctx = NULL;
    }
    fprintf(stderr, "[video] no OpenGL 3.3 (%s): using the software renderer\n", SDL_GetError());
    if (s_window)
        SDL_DestroyWindow(s_window);
    s_window = NULL;
    return 0;
}

void PC_GpuInit(int scale)
{
    const char *h = getenv("THPS_HEADLESS"), *r = getenv("THPS_RENDERER");
    int gl = !(r && strcmp(r, "sw") == 0);
    PC_GpuReset();
    s_headless = h && h[0] && h[0] != '0';
    if (s_headless && !(r && strcmp(r, "gl") == 0))
        gl = 0;
    if (!s_pixels)
        s_pixels = (uint32_t *)calloc((size_t)TEX_W * TEX_H, sizeof(uint32_t));
    if ((s_headless && !gl) || s_window)
        return;
    if (scale < 1)
        scale = 2;
    PC_SettingsLoad();

    /* sizes in pixels, not scaled by the desktop's DPI setting: 640x480 is 640x480 */
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[video] SDL video init failed: %s (running headless)\n", SDL_GetError());
        s_headless = 1;
        return;
    }
    s_video_inited = 1;
    if (gl && init_gl(scale))
        return;
    if (s_headless)
        return;
    const char *f = getenv("THPS_FILTER");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, (f && f[0] == '1') ? "1" : "0");

    s_window = SDL_CreateWindow("Tony Hawk's Pro Skater", SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, 320 * scale, 240 * scale,
                                SDL_WINDOW_RESIZABLE);
    if (s_window)
        s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED);
    if (s_window && !s_renderer)
        s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_SOFTWARE);
    if (s_renderer)
        s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING, TEX_W, TEX_H);
    if (!s_texture) {
        fprintf(stderr, "[video] window setup failed: %s (running headless)\n", SDL_GetError());
        PC_GpuShutdown();
        s_headless = 1;
    }
}

void PC_GpuShutdown(void)
{
    if (s_glctx) {
        PC_GlShutdown();
        SDL_GL_DeleteContext(s_glctx);
        s_glctx = NULL;
    }
    if (s_texture)
        SDL_DestroyTexture(s_texture);
    if (s_renderer)
        SDL_DestroyRenderer(s_renderer);
    if (s_window)
        SDL_DestroyWindow(s_window);
    s_texture = NULL;
    s_renderer = NULL;
    s_window = NULL;
    if (s_video_inited) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        s_video_inited = 0;
    }
}

extern volatile uint32_t pc_heartbeat;

void PC_GpuPresent(void)
{
    PC_GpuVBlank();

    {
        static int from = -2, to = -2;
        if (from == -2) {
            const char *e = getenv("THPS_GPUDUMP");
            if (!e || sscanf(e, "%d-%d", &from, &to) != 2)
                from = to = -1;
        }
        if (from >= 0 && (int)pc_heartbeat >= from && (int)pc_heartbeat <= to) {
            PC_GpuDisplay dd;
            PC_GpuGetDisplay(&dd);
            fprintf(stderr, "present f%u disp_x=%d disp_y=%d\n", pc_heartbeat, dd.x, dd.y);
        }
    }

    PC_GpuDisplay d;
    PC_GpuGetDisplay(&d);
    if (PC_GlActive()) {
        PC_GlPresent(s_window, d.x, d.y, d.w, d.h, d.rgb24, d.enabled);
        return;
    }
    if (s_headless || !s_texture)
        return;

    int w, h;
    PC_GpuDisplayToARGB(s_pixels, TEX_W, TEX_W, TEX_H, &w, &h);
    if (!d.enabled)
        memset(s_pixels, 0, (size_t)TEX_W * TEX_H * sizeof(uint32_t));

    SDL_Rect src = { 0, 0, w, h };
    SDL_UpdateTexture(s_texture, &src, s_pixels, TEX_W * (int)sizeof(uint32_t));

    int ow, oh;
    SDL_GetRendererOutputSize(s_renderer, &ow, &oh);
    SDL_Rect dst;
    if (ow * 3 > oh * 4) {
        dst.h = oh;
        dst.w = oh * 4 / 3;
    } else {
        dst.w = ow;
        dst.h = ow * 3 / 4;
    }
    dst.x = (ow - dst.w) / 2;
    dst.y = (oh - dst.h) / 2;

    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    SDL_RenderCopy(s_renderer, s_texture, &src, &dst);
    SDL_RenderPresent(s_renderer);
}

/* ---- BMP dumps (24-bit, bottom-up) ---- */

static void put_le(uint8_t *p, uint32_t v, int n)
{
    for (int i = 0; i < n; i++)
        p[i] = (uint8_t)(v >> (8 * i));
}

static int write_bmp(const char *path, const uint32_t *argb, int pitch, int w, int h)
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
    put_le(hdr + 2, (uint32_t)(54 + stride * h), 4);
    put_le(hdr + 10, 54, 4);
    put_le(hdr + 14, 40, 4);
    put_le(hdr + 18, (uint32_t)w, 4);
    put_le(hdr + 22, (uint32_t)h, 4);
    put_le(hdr + 26, 1, 2);
    put_le(hdr + 28, 24, 2);
    put_le(hdr + 34, (uint32_t)(stride * h), 4);
    put_le(hdr + 38, 2835, 4);
    put_le(hdr + 42, 2835, 4);
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

int PC_GpuDumpVram(const char *path)
{
    const uint16_t *v = PC_GpuVram();
    PC_GlSyncRead(0, 0, TEX_W, TEX_H);
    uint32_t *buf = (uint32_t *)malloc((size_t)TEX_W * TEX_H * sizeof(uint32_t));
    if (!buf)
        return -1;
    for (int i = 0; i < TEX_W * TEX_H; i++) {
        uint32_t c = v[i], r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        r = (r << 3) | (r >> 2);
        g = (g << 3) | (g >> 2);
        b = (b << 3) | (b >> 2);
        buf[i] = 0xff000000u | (r << 16) | (g << 8) | b;
    }
    int rc = write_bmp(path, buf, TEX_W, TEX_W, TEX_H);
    free(buf);
    return rc;
}

int PC_GpuDumpDisplay(const char *path)
{
    uint32_t *buf = (uint32_t *)malloc((size_t)TEX_W * TEX_H * sizeof(uint32_t));
    int w, h;
    if (!buf)
        return -1;
    PC_GpuDisplayToARGB(buf, TEX_W, TEX_W, TEX_H, &w, &h);
    int rc = write_bmp(path, buf, TEX_W, w, h);
    free(buf);

    /* and what OpenGL drew, at its internal resolution: <path without .bmp>_gl.bmp */
    PC_GpuDisplay d;
    PC_GpuGetDisplay(&d);
    int hw = 0, hh = 0;
    uint32_t *hi = PC_GlActive() && !d.rgb24 ? PC_GlReadArea(d.x, d.y, w, h, &hw, &hh) : NULL;
    if (hi && hw == w && hh == h) {
        free(hi);
        hi = NULL;
    }
    if (hi) {
        char p2[512];
        const char *dot = strrchr(path, '.');
        int n = dot ? (int)(dot - path) : (int)strlen(path);
        snprintf(p2, sizeof p2, "%.*s_gl.bmp", n, path);
        write_bmp(p2, hi, hw, hw, hh);
        free(hi);
    }
    return rc;
}

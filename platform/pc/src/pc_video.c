/* Presentation of the PS1 display area, 4:3 letterboxed, plus BMP dumps of VRAM and the display
 * for headless debugging. The window has an OpenGL 3.3 context and the GPU draws through
 * pc_gpu_gl.c at THPS_GL_SCALE times the PS1 resolution (default: the window scale). With
 * THPS_RENDERER=sw, or without OpenGL, the software GPU draws and an SDL_Renderer streaming
 * texture shows its VRAM. THPS_HEADLESS=1 skips the window and uses the software GPU, unless
 * THPS_RENDERER=gl (then OpenGL draws into a hidden window). THPS_FILTER=1 selects linear
 * scaling. Event pumping is the input layer's job, not this file's. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "pc_gpu.h"
#include "pc_gpu_gl.h"

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

/* The OpenGL window and renderer; 0 when the software path has to be used instead. */
static int init_gl(int scale)
{
    const char *e = getenv("THPS_GL_SCALE");
    int internal = e && atoi(e) > 0 ? atoi(e) : scale;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    s_window = SDL_CreateWindow("Tony Hawk's Pro Skater", SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, 320 * scale, 240 * scale,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                    (s_headless ? SDL_WINDOW_HIDDEN : 0));
    if (s_window)
        s_glctx = SDL_GL_CreateContext(s_window);
    if (s_glctx) {
        /* the port paces itself to the PS1's 60 Hz; waiting for the monitor as well would stall */
        SDL_GL_SetSwapInterval(0);
        if (PC_GlInit(internal) == 0)
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
    uint32_t *hi = (PC_GlActive() && PC_GlScale() > 1 && !d.rgb24) ? PC_GlReadArea(d.x, d.y, w, h)
                                                                    : NULL;
    if (hi) {
        char p2[512];
        const char *dot = strrchr(path, '.');
        int n = dot ? (int)(dot - path) : (int)strlen(path);
        snprintf(p2, sizeof p2, "%.*s_gl.bmp", n, path);
        write_bmp(p2, hi, w * PC_GlScale(), w * PC_GlScale(), h * PC_GlScale());
        free(hi);
    }
    return rc;
}

/* PS1 GPU: 1024x512 16-bit VRAM driven by real GP0/GP1 command words, plus presentation of the
 * display area in an SDL2 window. Drawing goes to the OpenGL backend (pc_gpu_gl.h) when it is
 * active, else to the software rasteriser. The libgpu HLE (src/hle_libgpu.c) and anything else
 * that talks to the GPU goes through this interface. */
#ifndef PC_GPU_H
#define PC_GPU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create the VRAM and the window (scale = integer window scale of a 320x240 display). */
void PC_GpuInit(int scale);
void PC_GpuShutdown(void);

/* Feed command words, exactly as the CPU/DMA would write GPU0/GPU1 (0x1f801810/0x1f801814). */
void PC_GpuWriteGP0(uint32_t word);
void PC_GpuWriteGP1(uint32_t word);
/* GPUREAD (0x1f801810 read): VRAM->CPU transfer data after a C0 command, else the latch. */
uint32_t PC_GpuReadData(void);
/* GPUSTAT. */
uint32_t PC_GpuReadStat(void);

/* Walk an ordering table / primitive chain as DMA channel 2 in linked-list mode does: each packet
 * header holds the word count in bits 24-31 and the next packet's address in bits 0-23 (end marker
 * 0xffffff). Packet addresses are PS1 main-RAM addresses; in this process main RAM is mapped at
 * 0x80000000, so a link n is read at (0x80000000 | n). */
void PC_GpuDrawChain(const uint32_t *first);
/* The current drawing offset and drawing area (inclusive), for the native 3D renderer; widened
 * for widescreen when it covers a gameplay screen, then returns PC_GlWideClip's surface number. */
int PC_GpuDrawState(int *ofs_x, int *ofs_y, int clip[4]);

/* Direct VRAM access (1024x512 halfwords, row-major), for transfers and debugging. */
uint16_t *PC_GpuVram(void);

/* Present the current display area (GP1 display start/mode) to the window. Called once per VSync. */
void PC_GpuPresent(void);

/* Fire a DrawSyncCallback scheduled by DrawOTag since the last tick (deferred to approximate real,
 * asynchronous GPU completion timing). Called once per vblank, before the game's VSync callback. */
void PC_GpuRunDueCallback(void);

/* Write the whole VRAM (or the display area) to a .bmp, for debugging. */
int PC_GpuDumpVram(const char *path);
int PC_GpuDumpDisplay(const char *path);

/* ---- additions ---- */

/* Power-on state: GP1(00h) plus zeroed VRAM. PC_GpuInit calls it. */
void PC_GpuReset(void);

/* Current display configuration, decoded from GP1(05h-08h). */
typedef struct {
    int x, y;        /* display start in VRAM (halfword units) */
    int w, h;        /* visible size in pixels (h is 480 in interlaced 480-line mode) */
    int rgb24;       /* 24-bit colour mode */
    int interlaced;  /* 480-line interlaced */
    int pal;
    int enabled;     /* GP1(03h) display enable */
} PC_GpuDisplay;
void PC_GpuGetDisplay(PC_GpuDisplay *d);

/* Convert the display area to 0xAARRGGBB pixels (alpha 0xff), `pitch` pixels per row, at most
 * maxw x maxh. Returns the converted size through w and h. Ignores the display-enable bit. */
void PC_GpuDisplayToARGB(uint32_t *dst, int pitch, int maxw, int maxh, int *w, int *h);

/* Exact-rectangle fill of VRAM (no drawing area, offset or mask), clipped to VRAM; rgb24 is
 * 0x00BBGGRR as in GP0 commands. */
void PC_GpuFillRect(int x, int y, int w, int h, uint32_t rgb24);

/* Toggle the interlace field (GPUSTAT bit 31); PC_GpuPresent calls it once per frame. */
void PC_GpuVBlank(void);

/* Nonzero when THPS_HEADLESS=1 suppressed the window. */
int PC_GpuIsHeadless(void);

#ifdef __cplusplus
}
#endif

#endif

/* OpenGL 3.3 drawing backend for the PS1 GPU (src/pc_gpu_gl.c).
 *
 * pc_gpu.c stays the GPU: it parses every GP0/GP1 word, holds the drawing state and owns the CPU
 * copy of VRAM. With this backend active it hands polygons, rectangles, lines and fills here
 * instead of rasterising them, and does VRAM transfers and copies on its own VRAM as before.
 * The backend draws into a 1024x512 VRAM render target at an integer scale, sampling textures
 * from a 16-bit copy of the CPU VRAM. Both sides are kept coherent in 16x16 cells: drawing
 * marks cells as newer on the GPU, and pc_gpu.c calls PC_GlSyncRead before it reads VRAM (and
 * PC_GlBeforeWrite/PC_GlWritten around writes), which read drawn pixels back or re-upload. */
#ifndef PC_GPU_GL_H
#define PC_GPU_GL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Needs a current GL 3.3 core context. scale = internal resolution multiplier (1 = PS1). */
int  PC_GlInit(int scale);
void PC_GlShutdown(void);
int  PC_GlActive(void);
int  PC_GlScale(void);

typedef struct {
    int x, y;        /* VRAM position, drawing offset applied */
    uint8_t r, g, b;
    int u, v;        /* texel coordinates */
    float px, py, w; /* precise position (offset applied) and view depth; w = 0: unknown */
} PC_GlVertex;

typedef struct {
    int textured, raw, semi, abr;
    int depth, tp_x, tp_y, clut_x, clut_y;
    uint8_t tw_and_x, tw_and_y, tw_or_x, tw_or_y;
    int mask_set, mask_check;
    int clip_l, clip_t, clip_r, clip_b;   /* drawing area, inclusive */
} PC_GlPrim;

void PC_GlTriangle(const PC_GlPrim *p, const PC_GlVertex *a, const PC_GlVertex *b,
                   const PC_GlVertex *c);
/* Rectangle (TILE/SPRT) at x,y (offset applied); u0,v0 = top-left texel. */
void PC_GlRect(const PC_GlPrim *p, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b,
               int u0, int v0, int flip_x, int flip_y);
void PC_GlLine(const PC_GlPrim *p, const PC_GlVertex *a, const PC_GlVertex *b);
/* GP0(02h) / ClearImage: exact rectangle, no drawing area, offset or mask; rgb24 = 0x00BBGGRR. */
void PC_GlFill(int x, int y, int w, int h, uint32_t rgb24);

/* ---- Native 3D layers (pc_render3d.c) ----
 * A vertex as the shader takes it. Emulated primitives use x, y in VRAM coordinates and z as
 * the perspective divisor (1 = affine). With PC_GL_I_VIEW, x, y, z are a view-space position
 * that the layer's projection maps to the screen, with a depth buffer. info = texture page bits
 * 0-3 (x/64) and 4 (y/256), blend mode 5-6, colour depth 7-8, then the flags below; clut is the
 * GPU's CLUT halfword; tw = texture window and_x, and_y, or_x, or_y; uvr = the texels the
 * face's texture coordinates span (u min, v min, u max, v max), which its samples stay inside. */
typedef struct {
    float x, y, z;
    uint8_t r, g, b, a;
    float u, v;
    uint16_t clut, info;
    uint8_t tw[4];
    uint8_t uvr[4];
} PC_GlVert;

#define PC_GL_I_TEXTURED 0x0200
#define PC_GL_I_SEMI     0x0800
#define PC_GL_I_DEPTH    0x2000   /* emulated primitive with a view depth in z (set internally) */
#define PC_GL_I_VIEW     0x4000   /* view-space position, projected by the layer */
#define PC_GL_I_SKY      0x8000   /* behind everything: depth 1, never written */

typedef struct {
    int ofs_x, ofs_y;                     /* drawing offset */
    int clip_l, clip_t, clip_r, clip_b;   /* drawing area, inclusive */
    float h, ofx, ofy;                    /* projection: screen = of + h * xy / z (GTE H, OFX, OFY) */
    float hither;                         /* near clip plane (view z) */
} PC_GlLayerParams;

/* Start a layer: the vertices it will draw (copied), a depth clear of its drawing area. Until
 * PC_GlLayerEnd, primitives drawn the usual way are depth-tested against it when the GTE
 * projected them. cls of a draw: 0 opaque (writes depth), 1 semi-transparent, 2 subtractive. */
void PC_GlLayerBegin(const PC_GlLayerParams *lp, const PC_GlVert *v, int nv);
void PC_GlLayerDraw(int first, int count, int cls);
void PC_GlLayerEnd(void);
/* Make a texture page / CLUT current before a layer draws with it (tpage, clut: GPU halfwords). */
void PC_GlLayerSyncTexture(uint16_t tpage, uint16_t clut);

/* Coherence with the CPU VRAM (rectangles may wrap at the VRAM edges). */
void PC_GlSyncRead(int x, int y, int w, int h);      /* CPU VRAM must be current here */
void PC_GlBeforeWrite(int x, int y, int w, int h);   /* the CPU is about to overwrite this */
void PC_GlWritten(int x, int y, int w, int h);       /* the CPU has written this */

/* Draw the display area (VRAM x,y w*h, 24-bit or not) into the current window and swap.
 * enabled = 0 shows black. */
void PC_GlPresent(void *sdl_window, int x, int y, int w, int h, int rgb24, int enabled);

/* The VRAM area x,y w*h at internal resolution, as 0xAARRGGBB rows (w*scale x h*scale pixels,
 * top row first) into a malloc'd buffer; NULL on failure. For debugging dumps. */
uint32_t *PC_GlReadArea(int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif

#endif

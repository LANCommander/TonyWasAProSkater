/* The texture store (pc_texstore.h): the texels at the game's coordinates, and which 16x16 cells
 * changed since the renderer last uploaded them (pc_gpu_gl.c). */
#include <string.h>

#include "pc_gpu_gl.h"
#include "pc_texstore.h"

static uint16_t s_tex[PC_TEX_H][PC_TEX_W];
static uint8_t s_cell[PC_TEX_H / PC_TEX_CELL][PC_TEX_W / PC_TEX_CELL];

const uint16_t *PC_TexStoreData(void) {
    return &s_tex[0][0];
}

uint8_t *PC_TexStoreCells(void) {
    return &s_cell[0][0];
}

/* Mark the cells under x, y, w, h (in the store) changed and used. */
static void Mark(int x, int y, int w, int h) {
    int cx, cy;
    for (cy = y / PC_TEX_CELL; cy <= (y + h - 1) / PC_TEX_CELL; cy++)
        for (cx = x / PC_TEX_CELL; cx <= (x + w - 1) / PC_TEX_CELL; cx++)
            s_cell[cy][cx] |= PC_TEX_DIRTY | PC_TEX_USED;
}

/* Clip to the store; 0 if nothing is left. */
static int Clip(int *x, int *y, int *w, int *h) {
    if (*x < 0) {
        *w += *x;
        *x = 0;
    }
    if (*y < 0) {
        *h += *y;
        *y = 0;
    }
    if (*x + *w > PC_TEX_W)
        *w = PC_TEX_W - *x;
    if (*y + *h > PC_TEX_H)
        *h = PC_TEX_H - *y;
    return *w > 0 && *h > 0;
}

void PC_TexUpload(int x, int y, int w, int h, const void *src) {
    const uint16_t *s = (const uint16_t *)src;
    int sw = w, j;
    if (x < 0 || y < 0 || !Clip(&x, &y, &w, &h))
        return;
    PC_GlTexturesChanging(x, y, w, h);
    for (j = 0; j < h; j++)
        memcpy(&s_tex[y + j][x], s + j * sw, (size_t)w * 2);
    Mark(x, y, w, h);
}

void PC_TexRead(int x, int y, int w, int h, void *dst) {
    uint16_t *d = (uint16_t *)dst;
    int dw = w, j;
    if (x < 0 || y < 0 || !Clip(&x, &y, &w, &h))
        return;
    for (j = 0; j < h; j++)
        memcpy(d + j * dw, &s_tex[y + j][x], (size_t)w * 2);
}

void PC_TexFill(int x, int y, int w, int h, uint32_t rgb) {
    uint16_t c = (uint16_t)((rgb >> 3 & 31) | (rgb >> 11 & 31) << 5 | (rgb >> 19 & 31) << 10);
    int i, j;
    if (!Clip(&x, &y, &w, &h))
        return;
    PC_GlTexturesChanging(x, y, w, h);
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            s_tex[y + j][x + i] = c;
    Mark(x, y, w, h);
}

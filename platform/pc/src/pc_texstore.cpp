/* The texture store (pc_texstore.h): the texels at the game's coordinates, and which 16x16 cells
 * changed since the renderer last uploaded them (pc_gpu_gl.cpp). */
#include <string.h>

#include "pc_texstore.h"
#include "pc/gl_gpu.h"
#include "pc/texture_store.h"

namespace pc {

TextureStore g_textureStore;

/* Clip to the store; false if nothing is left. */
static bool Clip(int &x, int &y, int &w, int &h) {
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > kTexW)
        w = kTexW - x;
    if (y + h > kTexH)
        h = kTexH - y;
    return w > 0 && h > 0;
}

/* Mark the cells under x, y, w, h (in the store) changed and used. */
void TextureStore::Mark(int x, int y, int w, int h) {
    for (int cy = y / kTexCell; cy <= (y + h - 1) / kTexCell; cy++)
        for (int cx = x / kTexCell; cx <= (x + w - 1) / kTexCell; cx++)
            m_cell[cy][cx] |= kTexDirty | kTexUsed;
}

void TextureStore::Upload(int x, int y, int w, int h, const void *src) {
    const uint16_t *s = (const uint16_t *)src;
    int sw = w;
    if (x < 0 || y < 0 || !Clip(x, y, w, h))
        return;
    g_glGpu.TexturesChanging(x, y, w, h);
    for (int j = 0; j < h; j++)
        memcpy(&m_tex[y + j][x], s + j * sw, (size_t)w * 2);
    Mark(x, y, w, h);
}

void TextureStore::Read(int x, int y, int w, int h, void *dst) {
    uint16_t *d = (uint16_t *)dst;
    int dw = w;
    if (x < 0 || y < 0 || !Clip(x, y, w, h))
        return;
    for (int j = 0; j < h; j++)
        memcpy(d + j * dw, &m_tex[y + j][x], (size_t)w * 2);
}

void TextureStore::Fill(int x, int y, int w, int h, uint32_t rgb) {
    uint16_t c = (uint16_t)((rgb >> 3 & 31) | (rgb >> 11 & 31) << 5 | (rgb >> 19 & 31) << 10);
    if (!Clip(x, y, w, h))
        return;
    g_glGpu.TexturesChanging(x, y, w, h);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            m_tex[y + j][x + i] = c;
    Mark(x, y, w, h);
}

}   // namespace pc

/* ---- the game's interface (pc_texstore.h) ---- */

extern "C" {

void PC_TexUpload(int x, int y, int w, int h, const void *src) { pc::g_textureStore.Upload(x, y, w, h, src); }
void PC_TexRead(int x, int y, int w, int h, void *dst) { pc::g_textureStore.Read(x, y, w, h, dst); }
void PC_TexFill(int x, int y, int w, int h, uint32_t rgb) { pc::g_textureStore.Fill(x, y, w, h, rgb); }

}

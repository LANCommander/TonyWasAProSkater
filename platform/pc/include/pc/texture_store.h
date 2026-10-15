/* The texture store (src/pc_texstore.cpp; the game's interface is pc_texstore.h): the game's
 * textures and CLUTs at the PS1 VRAM coordinates it placed them at, 16-bit texels in the PS1's
 * texture formats, and which 16x16 cells changed since the renderer (pc/gl_gpu.h) last uploaded
 * them. */
#pragma once

#include <stdint.h>

#include "pc_texstore.h"

namespace pc {

/* The store's size (the PS1's VRAM), and its 16x16 cells' bits. */
constexpr int kTexW = 1024;
constexpr int kTexH = 512;
constexpr int kTexCell = 16;
constexpr uint8_t kTexDirty = 1; /* changed since the renderer's upload */
constexpr uint8_t kTexUsed = 2;  /* ever written */

class TextureStore {
  public:
    /* Texels (w x h halfwords, rows of w) to x, y. */
    void Upload(int x, int y, int w, int h, const void *src);
    /* Texels at x, y (w x h) into dst. */
    void Read(int x, int y, int w, int h, void *dst);
    /* Fill with one colour, rgb = 0x00BBGGRR (8 bits each, as ClearImage). */
    void Fill(int x, int y, int w, int h, uint32_t rgb);

    /* The texels (pc::kTexW x pc::kTexH), and a byte of PC_TEX_* bits per 16x16 cell, row by row. */
    const uint16_t *Data() const { return &m_tex[0][0]; }
    uint8_t *Cells() { return &m_cell[0][0]; }

  private:
    void Mark(int x, int y, int w, int h);

    uint16_t m_tex[kTexH][kTexW] = {};
    uint8_t m_cell[kTexH / kTexCell][kTexW / kTexCell] = {};
};

extern TextureStore g_textureStore;

}   // namespace pc

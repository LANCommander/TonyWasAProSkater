/* The PS1's STR video pictures (src/pc_vdec.cpp): the MDEC's replacement. pc_vdec.h has the C
 * interface and the format numbers (PC_VDEC_RGB24, PC_VDEC_15STP). */
#pragma once

#include <stdint.h>

namespace pc {

class VideoDecoder {
public:
    /* Decode a frame (runlevel: the game's VLC output) `cols` macroblock columns wide and h pixels
     * high into out, `pitch` bytes per row, in format fmt (PC_VDEC_*). */
    void Frame(const void *runlevel, int fmt, int cols, int h, void *out, int pitch);

    /* A song's video into the texture store (pc_texstore.h): the picture the game's columns
     * covered, from texture x to xend (halfwords: columns of 16 pixels, 24 halfwords each when
     * rgb24) at y, h high. */
    void ToTexture(const void *runlevel, bool rgb24, int x, int y, int xend, int h);

private:
    uint32_t Next();
    void Block(float out[64]);
    void Macroblock(uint8_t *out, int pitch, int h);

    const uint16_t *m_in = nullptr, *m_end = nullptr; /* run-level halfwords still to decode */
    bool m_rgb24 = false;
    uint16_t *m_texBuf = nullptr; /* ToTexture's picture, before it goes to the texture store */
    int m_texCap = 0;
};

extern VideoDecoder g_videoDecoder;

} // namespace pc

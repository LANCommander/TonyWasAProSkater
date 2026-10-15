/* The PS1's STR video pictures (pc_vdec.h).
 *
 * The decoding follows the MDEC (psx-spx "Macroblock Decoder"): each block is a DC halfword
 * (quantiser scale << 10 | 10-bit DC), then (run << 10 | 10-bit level) halfwords up to an end
 * code 0xfe00; coefficients come in zigzag order and are scaled by libpress's quantisation table
 * (MPEG-1's intra matrix); a macroblock is Cr, Cb and four Y blocks covering 16x16 pixels. The
 * inverse DCT is done in floating point and the colour conversion with the MDEC's coefficients,
 * so pictures can differ from the hardware's in the last bit, not visibly. */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pc/vdec.h"
#include "pc_texstore.h"
#include "pc_vdec.h"

namespace pc {

VideoDecoder g_videoDecoder;

namespace {

constexpr uint32_t kEob = 0xfe00u;

/* libpress's quantisation table (the same for luma and chroma), in natural order */
const uint8_t kQuant[64] = {
    2,  16, 19, 22, 26, 27, 29, 34, 16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38, 22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48, 26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69, 27, 29, 35, 38, 46, 56, 69, 83,
};

const uint8_t kZigzag[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

float s_cos[8][8]; /* s_cos[x][u] = C(u)/2 cos((2x+1)u pi/16), built on first use */

void InitTables() {
    int x, u;
    if (s_cos[0][0] != 0.0f)
        return;
    for (x = 0; x < 8; x++)
        for (u = 0; u < 8; u++)
            s_cos[x][u] = (float)((u ? 0.5 : 0.5 / sqrt(2.0)) * cos((2 * x + 1) * u * M_PI / 16));
}

int Signed10(uint32_t v) {
    return (int)((v & 0x3ff) ^ 0x200) - 0x200;
}

int Clamp8(float v) {
    int i = (int)lrintf(v);
    return i < -128 ? 0 : i > 127 ? 255 : i + 128;
}

} // namespace

uint32_t VideoDecoder::Next() {
    uint32_t h;
    if (m_in >= m_end)
        return kEob;
    h = *m_in++;
    return h;
}

/* One block: coefficients, then the inverse DCT into out (signed samples, row-major 8x8). */
void VideoDecoder::Block(float out[64]) {
    float coef[64], tmp[64];
    uint32_t h;
    int k, q, i, x, y, u;
    memset(coef, 0, sizeof coef);
    do
        h = Next();
    while (h == kEob && m_in < m_end); /* the MDEC skips end codes between blocks */
    q = (int)(h >> 10);
    coef[0] = (float)(Signed10(h) * kQuant[0]);
    for (k = 0;;) {
        int v;
        h = Next();
        if (h == kEob)
            break;
        k += (int)(h >> 10) + 1;
        if (k > 63)
            break;
        i = kZigzag[k];
        v = (Signed10(h) * kQuant[i] * q + 4) / 8;
        if (v < -1024)
            v = -1024;
        else if (v > 1023)
            v = 1023;
        coef[i] = (float)v;
    }
    /* rows then columns */
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) {
            float s = 0;
            for (u = 0; u < 8; u++)
                s += coef[y * 8 + u] * s_cos[x][u];
            tmp[y * 8 + x] = s;
        }
    for (x = 0; x < 8; x++)
        for (y = 0; y < 8; y++) {
            float s = 0;
            for (u = 0; u < 8; u++)
                s += tmp[u * 8 + x] * s_cos[y][u];
            out[y * 8 + x] = s;
        }
}

/* Decode the next macroblock into out (pitch bytes per row), at most h rows of it. */
void VideoDecoder::Macroblock(uint8_t *out, int pitch, int h) {
    float cr[64], cb[64], y[4][64];
    int i, px, py;
    Block(cr);
    Block(cb);
    for (i = 0; i < 4; i++)
        Block(y[i]);
    for (py = 0; py < 16 && py < h; py++)
        for (px = 0; px < 16; px++) {
            int c = (py >> 1) * 8 + (px >> 1);
            float l = y[(py >> 3) * 2 + (px >> 3)][(py & 7) * 8 + (px & 7)];
            int r = Clamp8(l + 1.402f * cr[c]);
            int g = Clamp8(l - 0.3437f * cb[c] - 0.7143f * cr[c]);
            int b = Clamp8(l + 1.772f * cb[c]);
            if (m_rgb24) {
                uint8_t *o = out + py * pitch + px * 3;
                o[0] = (uint8_t)r;
                o[1] = (uint8_t)g;
                o[2] = (uint8_t)b;
            } else {
                uint16_t p = (uint16_t)((r >> 3) | (g >> 3) << 5 | (b >> 3) << 10 | 0x8000);
                memcpy(out + py * pitch + px * 2, &p, 2);
            }
        }
}

void VideoDecoder::Frame(const void *runlevel, int fmt, int cols, int h, void *out, int pitch) {
    const uint32_t *p = (const uint32_t *)runlevel;
    int bpp = fmt == PC_VDEC_RGB24 ? 3 : 2, c, y;
    InitTables();
    m_in = (const uint16_t *)(p + 1);
    m_end = (const uint16_t *)(p + 1 + (p[0] & 0xffff));
    m_rgb24 = fmt == PC_VDEC_RGB24;
    for (c = 0; c < cols; c++)
        for (y = 0; y < h; y += 16)
            Macroblock((uint8_t *)out + y * pitch + c * 16 * bpp, pitch, h - y);
}

void VideoDecoder::ToTexture(const void *runlevel, bool rgb24, int x, int y, int xend, int h) {
    int colw = rgb24 ? 24 : 16, cols = (xend - x + colw - 1) / colw, w = cols * colw;
    if (cols <= 0 || h <= 0)
        return;
    if (m_texCap < w * h) {
        m_texCap = w * h;
        m_texBuf = (uint16_t *)realloc(m_texBuf, (size_t)m_texCap * 2);
    }
    Frame(runlevel, rgb24 ? PC_VDEC_RGB24 : PC_VDEC_15STP, cols, h, m_texBuf, w * 2);
    PC_TexUpload(x, y, w, h, m_texBuf);
}

} // namespace pc

/* ---- the game's interface (pc_vdec.h) ---- */

void PC_VdecToTexture(const void *runlevel, int rgb24, int x, int y, int xend, int h) {
    pc::g_videoDecoder.ToTexture(runlevel, rgb24 != 0, x, y, xend, h);
}

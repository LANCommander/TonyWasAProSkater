/* The MDEC (libpress): decodes the PS1's STR video (full-screen movies, the songs' 96x64 video).
 *
 * The game does the variable-length stage itself (DecDCTvlc3, decomp/src/shatter/decdctvlc3.c):
 * it turns each frame's bitstream into the run-level halfwords the MDEC takes and hands them over
 * with DecDCTin. DecDCTout then asks for so many words of pixels, a column of macroblocks 16
 * pixels wide, and when they are there the DecDCToutCallback (the game's strCallback) uploads
 * them to VRAM with LoadImage and asks for the next column.
 *
 * Here DecDCTout decodes at once. The callback runs after DecDCTout returns, not inside it: the
 * game's callback asks for the next column into the other of its two buffers before it uploads
 * the one just decoded, which works on the PS1 because the next column takes a while; done
 * immediately and recursively, the third column would overwrite the first before its upload.
 *
 * The decoding follows the MDEC (psx-spx "Macroblock Decoder"): each block is a DC halfword
 * (quantiser scale << 10 | 10-bit DC), then (run << 10 | 10-bit level) halfwords up to an end
 * code 0xfe00; coefficients come in zigzag order and are scaled by libpress's quantisation table
 * (MPEG-1's intra matrix); a macroblock is Cr, Cb and four Y blocks covering 16x16 pixels. The
 * inverse DCT is done in floating point and the colour conversion with the MDEC's coefficients,
 * so pictures can differ from the hardware's in the last bit, not visibly. */
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "pc_runtime.h"

#define EOB 0xfe00u
#define MB_PIXELS 256

/* libpress's quantisation table (the same for luma and chroma), in natural order */
static const uint8_t kQuant[64] = {
    2,  16, 19, 22, 26, 27, 29, 34, 16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38, 22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48, 26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69, 27, 29, 35, 38, 46, 56, 69, 83,
};

static const uint8_t kZigzag[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

static struct {
    uint32_t in, end;       /* run-level halfwords still to decode (PS1 addresses) */
    int rgb24, stp;         /* output: 24-bit, or 15-bit with the mask bit set */
    uint32_t callback;      /* DecDCToutCallback */
    uint8_t mb[MB_PIXELS * 3]; /* a decoded macroblock not fully taken yet */
    int mbBytes, mbUsed;
    int pending, dispatching;
} s_mdec;

static float s_cos[8][8]; /* s_cos[x][u] = C(u)/2 cos((2x+1)u pi/16) */

static void InitTables(void) {
    int x, u;
    if (s_cos[0][0] != 0.0f)
        return;
    for (x = 0; x < 8; x++)
        for (u = 0; u < 8; u++)
            s_cos[x][u] = (float)((u ? 0.5 : 0.5 / sqrt(2.0)) * cos((2 * x + 1) * u * M_PI / 16));
}

static int Signed10(uint32_t v) {
    return (int)((v & 0x3ff) ^ 0x200) - 0x200;
}

static uint32_t Next(void) {
    uint32_t h;
    if (s_mdec.in >= s_mdec.end)
        return EOB;
    h = *(uint16_t *)PSX_PTR(s_mdec.in);
    s_mdec.in += 2;
    return h;
}

/* One block: coefficients, then the inverse DCT into out (signed samples, row-major 8x8). */
static void Block(float out[64]) {
    float coef[64], tmp[64];
    uint32_t h;
    int k, q, i, x, y, u;
    memset(coef, 0, sizeof coef);
    do
        h = Next();
    while (h == EOB && s_mdec.in < s_mdec.end); /* the MDEC skips end codes between blocks */
    q = (int)(h >> 10);
    coef[0] = (float)(Signed10(h) * kQuant[0]);
    for (k = 0;;) {
        int v;
        h = Next();
        if (h == EOB)
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

static int Clamp8(float v) {
    int i = (int)lrintf(v);
    return i < -128 ? 0 : i > 127 ? 255 : i + 128;
}

/* Decode the next macroblock into s_mdec.mb, as 16 rows of 16 pixels. */
static void Macroblock(void) {
    float cr[64], cb[64], y[4][64];
    int i, px, py;
    Block(cr);
    Block(cb);
    for (i = 0; i < 4; i++)
        Block(y[i]);
    for (py = 0; py < 16; py++)
        for (px = 0; px < 16; px++) {
            int c = (py >> 1) * 8 + (px >> 1);
            float l = y[(py >> 3) * 2 + (px >> 3)][(py & 7) * 8 + (px & 7)];
            int r = Clamp8(l + 1.402f * cr[c]);
            int g = Clamp8(l - 0.3437f * cb[c] - 0.7143f * cr[c]);
            int b = Clamp8(l + 1.772f * cb[c]);
            if (s_mdec.rgb24) {
                uint8_t *o = s_mdec.mb + (py * 16 + px) * 3;
                o[0] = (uint8_t)r;
                o[1] = (uint8_t)g;
                o[2] = (uint8_t)b;
            } else {
                uint16_t p = (uint16_t)((r >> 3) | (g >> 3) << 5 | (b >> 3) << 10 |
                                        (s_mdec.stp ? 0x8000 : 0));
                memcpy(s_mdec.mb + (py * 16 + px) * 2, &p, 2);
            }
        }
    s_mdec.mbBytes = s_mdec.rgb24 ? MB_PIXELS * 3 : MB_PIXELS * 2;
    s_mdec.mbUsed = 0;
}

void DecDCTReset(int mode) {
    (void)mode;
    InitTables();
    s_mdec.in = s_mdec.end = 0;
    s_mdec.mbBytes = s_mdec.mbUsed = 0;
}

/* DecDCTin(buf, mode): buf = the MDEC header word (its low 16 bits: the run-level data's size in
 * words), then the halfwords. mode bit 0: 24-bit output, else 15-bit; bit 1: set the mask bit. */
void DecDCTin(uint32_t buf, int mode) {
    InitTables();
    s_mdec.in = buf + 4;
    s_mdec.end = buf + 4 + 4 * (*(uint32_t *)PSX_PTR(buf) & 0xffff);
    s_mdec.rgb24 = mode & 1;
    s_mdec.stp = (mode & 2) != 0;
    s_mdec.mbBytes = s_mdec.mbUsed = 0;
}

int DecDCTinSync(int mode) {
    (void)mode;
    return 0;
}

int DecDCToutSync(int mode) {
    (void)mode;
    return 0;
}

uint32_t DecDCToutCallback(uint32_t func) {
    uint32_t old = s_mdec.callback;
    s_mdec.callback = func;
    return old;
}

/* DecDCTout(buf, words) at 0x8009033c: the next `words` 32-bit words of pixels into buf. */
void hle_8009033c(uint32_t buf, int words) {
    uint8_t *o = PSX_PTR(buf);
    int left = words * 4;
    while (left > 0) {
        int n;
        if (s_mdec.mbUsed >= s_mdec.mbBytes)
            Macroblock();
        n = s_mdec.mbBytes - s_mdec.mbUsed;
        if (n > left)
            n = left;
        memcpy(o, s_mdec.mb + s_mdec.mbUsed, (size_t)n);
        s_mdec.mbUsed += n;
        o += n;
        left -= n;
    }
    if (!s_mdec.callback)
        return;
    s_mdec.pending++;
    if (s_mdec.dispatching)
        return;
    s_mdec.dispatching = 1;
    while (s_mdec.pending) {
        s_mdec.pending--;
        ((void (*)(void))PSX_PTR(s_mdec.callback))();
    }
    s_mdec.dispatching = 0;
}

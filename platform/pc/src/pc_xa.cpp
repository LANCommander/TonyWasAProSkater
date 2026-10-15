/* CD-XA ADPCM decoder (psx-spx "CDROM XA Audio ADPCM Compression").
 *
 * An audio sector is mode 2 form 2 with submode bit 2 set; its coding byte gives mono/stereo
 * (bits 0-1), 37800/18900 Hz (bits 2-3) and 4/8-bit samples (bits 4-5). The 2304 data bytes after
 * the subheader are 18 sound groups of 128 bytes: 16 bytes of per-unit parameters (shift in bits
 * 0-3, filter in bits 4-5, unit u at byte 4+u) and 112 bytes holding 28 samples for each of 8
 * (4-bit) or 4 (8-bit) sound units, interleaved word by word. In stereo, even units are the left
 * channel and odd units the right; each channel keeps its own filter history across sectors. */
#include <string.h>

#include "pc/xa.h"

namespace pc {

constinit XaDecoder g_xaDecoder;

namespace {

constexpr int k_k0[4] = {0, 60, 115, 98};
constexpr int k_k1[4] = {0, 0, -52, -55};

int Clamp16(int x) { return x < -32768 ? -32768 : x > 32767 ? 32767 : x; }

} // namespace

void XaDecoder::Reset() { memset(m_hist, 0, sizeof m_hist); }

int XaDecoder::DecodeSector(const uint8_t *raw, int16_t *stereo, int &rate) {
    const uint8_t *sub = raw + 16;
    if (raw[15] != 2 || !(sub[2] & 0x04))
        return 0;
    int coding = sub[3];
    int isStereo = (coding & 3) == 1;
    int bits8 = (coding & 0x30) == 0x10;
    int units = bits8 ? 4 : 8;
    /* frames each group yields: units x 28 samples, halved when they are split into L/R */
    int perGroup = units * 28 / (isStereo ? 2 : 1);
    rate = (coding & 0x0c) ? 18900 : 37800;

    for (int g = 0; g < 18; g++) {
        const uint8_t *grp = raw + 24 + g * 128;
        int base = g * perGroup;
        for (int u = 0; u < units; u++) {
            int param = grp[4 + u];
            int shift = param & 0x0f;
            int filt = (param >> 4) & 3;
            int ch = isStereo ? (u & 1) : 0;
            int *h = m_hist[ch];
            /* first output frame of this unit's 28 samples */
            int first = base + (isStereo ? (u >> 1) : u) * 28;
            if (shift > 12)
                shift = 9;
            for (int j = 0; j < 28; j++) {
                int s;
                if (bits8)
                    s = (int16_t)(grp[16 + j * 4 + u] << 8);
                else
                    s = (int16_t)(((grp[16 + j * 4 + (u >> 1)] >> ((u & 1) * 4)) & 0x0f) << 12);
                s >>= shift;
                s = Clamp16(s + ((h[0] * k_k0[filt] + h[1] * k_k1[filt] + 32) >> 6));
                h[1] = h[0];
                h[0] = s;
                if (isStereo) {
                    stereo[(first + j) * 2 + ch] = (int16_t)s;
                } else {
                    stereo[(first + j) * 2] = (int16_t)s;
                    stereo[(first + j) * 2 + 1] = (int16_t)s;
                }
            }
        }
    }
    return 18 * perGroup;
}

} // namespace pc

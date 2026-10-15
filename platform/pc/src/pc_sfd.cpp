/* Sofdec films (see pc_sfd.h): MPEG-1 program-stream demultiplexer and CRI ADX decoder.
 *
 * Program stream: pack headers (00 00 01 BA: 12 bytes in MPEG-1, 14 + stuffing in MPEG-2), then
 * packets of 00 00 01 <id> <u16 length> <payload>: the system header (BB), padding (BE), private
 * stream 2 (BF), video (E0) and audio (C0). An MPEG-1 packet's payload starts with stuffing (FF),
 * an optional STD buffer size (01xx xxxx, 2 bytes) and a PTS (0010, 5 bytes), PTS + DTS (0011,
 * 10 bytes) or 0F.
 *
 * ADX (big endian): u16 0x8000, u16 data offset - 4 ("(c)CRI" just before the data), u8 encoding
 * (3 = standard), u8 frame size (18), u8 bits per sample (4), u8 channels, u32 rate, u32 samples,
 * u16 high-pass cutoff (the prediction coefficients come from it). The data is frames of 18
 * bytes per channel, channels interleaved: u16 scale, then 32 4-bit samples, high nibble first:
 * s = nibble * (scale + 1) + (c1 * s[-1] >> 12) + (c2 * s[-2] >> 12). A frame whose scale has
 * bit 15 set ends the data. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/sfd.h"
#include "pc/platform.h"

namespace pc {

namespace {

constexpr int kReadChunk = 0x10000;
constexpr int kBufSize = kReadChunk * 2 + 0x10010; /* a whole packet (6 + 65535 bytes) always fits */

int Be16(const uint8_t *p) { return p[0] << 8 | p[1]; }
uint32_t Be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

int Clamp16(int x) { return x < -32768 ? -32768 : x > 32767 ? 32767 : x; }

} // namespace

/* Make n bytes from m_bufPos available: false at the end of the file. */
bool SfdFilm::Need(int n) {
    while (m_bufLen - m_bufPos < n) {
        size_t got;
        if (m_bufPos) {
            memmove(m_buf.get(), m_buf.get() + m_bufPos, (size_t)(m_bufLen - m_bufPos));
            m_bufBase += m_bufPos;
            m_bufLen -= m_bufPos;
            m_bufPos = 0;
        }
        got = fread(m_buf.get() + m_bufLen, 1, kReadChunk, m_f);
        if (!got)
            return false;
        m_bufLen += (int)got;
    }
    return true;
}

void SfdFilm::ParseAdxHeader() {
    const uint8_t *h = m_hdr;
    int start;
    if (m_adxReady || m_hdrLen < 20)
        return;
    start = Be16(h + 2) + 4;
    if (Be16(h) != 0x8000 || h[4] != 3 || h[6] != 4 || h[7] < 1 || h[7] > kAdxMaxCh ||
        h[5] < 3) {
        m_adxEnded = true;          /* not standard ADX: no audio */
        m_adxReady = true;
        return;
    }
    m_channels = h[7];
    m_chFrame = h[5];
    m_frameBytes = m_chFrame * m_channels;
    m_rate = (int)Be32(h + 8);
    m_total = Be32(h + 12);
    m_dataStart = start;
    {
        double z = cos(2.0 * 3.14159265358979323846 * Be16(h + 16) / (m_rate ? m_rate : 1));
        double a = sqrt(2.0) - z, b = sqrt(2.0) - 1.0;
        double c = (a - sqrt((a + b) * (a - b))) / b;
        m_c1 = (int)(c * 8192);
        m_c2 = (int)(c * c * -4096);
    }
    if (m_keepFrom < start)
        m_keepFrom = start;
    m_adxReady = true;
}

void SfdFilm::AudioBytes(const uint8_t *p, int n) {
    long long at = m_audioBytes;
    m_audioBytes += n;
    if (m_hdrLen < kHdrMax) {
        int k = n < kHdrMax - m_hdrLen ? n : kHdrMax - m_hdrLen;
        memcpy(m_hdr + m_hdrLen, p, (size_t)k);
        m_hdrLen += k;
        ParseAdxHeader();
    }
    if (!m_audioOn || m_skipTo || !m_adxReady || m_adxEnded)
        return;
    if (at < m_keepFrom) {
        long long d = m_keepFrom - at;
        if (d >= n)
            return;
        p += d;
        n -= (int)d;
    }
    if (m_qLen + n > m_qCap) {
        m_qCap = (m_qLen + n) * 2;
        m_q = (uint8_t *)realloc(m_q, (size_t)m_qCap);
    }
    memcpy(m_q + m_qLen, p, (size_t)n);
    m_qLen += n;
}

/* An audio or video packet's payload, after the MPEG-1 packet header. */
void SfdFilm::Payload(int id, const uint8_t *b, int len) {
    int k;
    for (k = 0; k < len && b[k] == 0xff; k++)
        ;
    if (k < len && (b[k] >> 6) == 1)
        k += 2;
    if (k < len && (b[k] >> 4) == 2)
        k += 5;
    else if (k < len && (b[k] >> 4) == 3)
        k += 10;
    else
        k++;
    if (k > len)
        return;
    if (id == 0xc0)
        AudioBytes(b + k, len - k);
    else if (m_video && !m_skipTo)
        m_video(m_videoCtx, b + k, len - k);
}

bool SfdFilm::Demux() {
    for (;;) {
        const uint8_t *b;
        int id, len;
        if (m_ended || !Need(4)) {
            m_ended = true;
            return false;
        }
        b = m_buf.get() + m_bufPos;
        if (b[0] || b[1] || b[2] != 1) {    /* resynchronise on the next start code */
            m_bufPos++;
            continue;
        }
        id = b[3];
        if (id == 0xb9) {                   /* end of the program stream */
            m_ended = true;
            return false;
        }
        if (id == 0xba) {
            if (!Need(14))
                continue;
            b = m_buf.get() + m_bufPos;
            m_bufPos += (b[4] >> 6) == 1 ? 14 + (b[13] & 7) : 12;
            continue;
        }
        if (id < 0xbb) {                    /* not a packet: a video start code in raw data */
            m_bufPos += 4;
            continue;
        }
        if (!Need(6))
            continue;
        len = Be16(m_buf.get() + m_bufPos + 4);
        if (!Need(6 + len)) {
            m_ended = true;
            return false;
        }
        b = m_buf.get() + m_bufPos + 6;
        m_bufPos += 6 + len;
        if (id == 0xe0 || id == 0xc0)
            Payload(id, b, len);
        if (m_skipTo && m_bufBase + m_bufPos >= m_skipTo)
            m_skipTo = 0;                   /* reached the seek target: queue what follows */
        return true;
    }
}

int SfdFilm::Audio(int16_t *stereo, int max, int *rate) {
    int out = 0, qPos = 0;
    while (out + 32 <= max && !m_adxEnded) {
        int ch;
        if (m_adxReady && m_qLen - qPos < m_frameBytes && m_ended) {
            m_adxEnded = true;
            break;
        }
        if (!m_adxReady || m_qLen - qPos < m_frameBytes) {
            if (qPos) {
                memmove(m_q, m_q + qPos, (size_t)(m_qLen - qPos));
                m_qLen -= qPos;
                qPos = 0;
            }
            if (!Demux() && !m_adxReady)
                m_adxEnded = true;
            continue;
        }
        for (ch = 0; ch < m_channels; ch++) {
            const uint8_t *fr = m_q + qPos + ch * m_chFrame;
            int scale = Be16(fr), i, *h = m_hist[ch];
            if (scale & 0x8000) {
                m_adxEnded = true;
                break;
            }
            scale++;
            for (i = 0; i < 32; i++) {
                int nib = (fr[2 + i / 2] >> (i & 1 ? 0 : 4)) & 15;
                int smp;
                if (nib >= 8)
                    nib -= 16;
                smp = Clamp16(nib * scale + ((m_c1 * h[0]) >> 12) + ((m_c2 * h[1]) >> 12));
                h[1] = h[0];
                h[0] = smp;
                stereo[(out + i) * 2 + ch] = (int16_t)smp;
                if (m_channels == 1)
                    stereo[(out + i) * 2 + 1] = (int16_t)smp;
            }
        }
        if (m_adxEnded)
            break;
        qPos += m_frameBytes;
        out += 32;
        m_done += 32;
        if (m_total && m_done >= m_total) {
            out -= (int)(m_done - m_total);
            m_adxEnded = true;
        }
    }
    if (qPos) {
        memmove(m_q, m_q + qPos, (size_t)(m_qLen - qPos));
        m_qLen -= qPos;
    }
    *rate = m_rate ? m_rate : 44100;
    return out;
}

void SfdFilm::Restart() {
    fseek(m_f, 0, SEEK_SET);
    m_bufLen = m_bufPos = 0;
    m_bufBase = 0;
    m_ended = false;
    m_hdrLen = 0;
    m_audioBytes = 0;
    m_adxReady = m_adxEnded = false;
    m_done = 0;
    memset(m_hist, 0, sizeof m_hist);
    m_keepFrom = 0;
    m_skipTo = 0;
    m_qLen = 0;
}

void SfdFilm::Seek(long long pos) {
    Restart();
    if (pos <= 0)
        return;
    /* the audio offset a file position stands for is only known by reading up to it */
    m_skipTo = pos;
    while (m_skipTo && Demux())
        ;
    if (m_adxReady && !m_adxEnded) {
        long long frames = (m_audioBytes - m_dataStart + m_frameBytes - 1) / m_frameBytes;
        if (frames < 0)
            frames = 0;
        m_keepFrom = m_dataStart + frames * m_frameBytes;
        m_done = (uint32_t)(frames * 32);
    }
}

std::unique_ptr<SfdFilm> SfdFilm::Open(const char *path) {
    uint8_t magic[4];
    FILE *f = fopen(path, "rb");
    if (!f)
        return nullptr;
    if (fread(magic, 1, 4, f) != 4 || magic[0] || magic[1] || magic[2] != 1 || magic[3] != 0xba) {
        fclose(f);
        return nullptr;
    }
    std::unique_ptr<SfdFilm> s(new SfdFilm);
    s->m_f = f;
    PC_FSEEK(f, 0, SEEK_END);
    s->m_size = PC_FTELL(f);
    s->m_buf.reset(new uint8_t[kBufSize]);
    s->m_audioOn = true;
    s->Restart();
    return s;
}

SfdFilm::~SfdFilm() {
    fclose(m_f);
    free(m_q);
}

int SfdFilm::AudioSeconds(const char *path) {
    std::unique_ptr<SfdFilm> s = Open(path);
    int secs = 0, i;
    if (!s)
        return 0;
    s->m_audioOn = false;
    for (i = 0; i < 64 && !s->m_adxReady && s->Demux(); i++)
        ;
    if (s->m_adxReady && !s->m_adxEnded && s->m_rate)
        secs = (int)(s->m_total / (uint32_t)s->m_rate);
    return secs;
}

} // namespace pc

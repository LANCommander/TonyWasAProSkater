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

#include "pc_platform.h"
#include "pc_sfd.h"

#define READ_CHUNK 0x10000
#define BUF_SIZE (READ_CHUNK * 2 + 0x10010) /* a whole packet (6 + 65535 bytes) always fits */
#define HDR_MAX 1024
#define ADX_MAX_CH 2

struct PcSfd {
    FILE *f;
    long long size;
    uint8_t *buf;
    int bufLen, bufPos;
    long long bufBase;          /* file offset of buf[0] */
    int ended;
    PC_SfdVideoFn video;
    void *videoCtx;
    int audioOn;

    /* audio stream */
    uint8_t hdr[HDR_MAX];       /* its first bytes: the ADX header */
    int hdrLen;
    long long audioBytes;       /* audio-stream bytes seen */
    int adxReady, adxEnded;
    int dataStart, frameBytes, chFrame, channels, rate;
    uint32_t total, done;       /* samples per channel: in the stream, decoded */
    int c1, c2, hist[ADX_MAX_CH][2];
    long long keepFrom;         /* audio offset from which bytes are queued */
    long long skipTo;           /* seeking: demultiplex to this file offset without queueing */
    uint8_t *q;
    int qLen, qCap;
};

static int Be16(const uint8_t *p) { return p[0] << 8 | p[1]; }
static uint32_t Be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

/* Make n bytes from bufPos available: 0 at the end of the file. */
static int Need(PcSfd *s, int n) {
    while (s->bufLen - s->bufPos < n) {
        size_t got;
        if (s->bufPos) {
            memmove(s->buf, s->buf + s->bufPos, (size_t)(s->bufLen - s->bufPos));
            s->bufBase += s->bufPos;
            s->bufLen -= s->bufPos;
            s->bufPos = 0;
        }
        got = fread(s->buf + s->bufLen, 1, READ_CHUNK, s->f);
        if (!got)
            return 0;
        s->bufLen += (int)got;
    }
    return 1;
}

static void ParseAdxHeader(PcSfd *s) {
    const uint8_t *h = s->hdr;
    int start;
    if (s->adxReady || s->hdrLen < 20)
        return;
    start = Be16(h + 2) + 4;
    if (Be16(h) != 0x8000 || h[4] != 3 || h[6] != 4 || h[7] < 1 || h[7] > ADX_MAX_CH ||
        h[5] < 3) {
        s->adxEnded = 1;            /* not standard ADX: no audio */
        s->adxReady = 1;
        return;
    }
    s->channels = h[7];
    s->chFrame = h[5];
    s->frameBytes = s->chFrame * s->channels;
    s->rate = (int)Be32(h + 8);
    s->total = Be32(h + 12);
    s->dataStart = start;
    {
        double z = cos(2.0 * 3.14159265358979323846 * Be16(h + 16) / (s->rate ? s->rate : 1));
        double a = sqrt(2.0) - z, b = sqrt(2.0) - 1.0;
        double c = (a - sqrt((a + b) * (a - b))) / b;
        s->c1 = (int)(c * 8192);
        s->c2 = (int)(c * c * -4096);
    }
    if (s->keepFrom < start)
        s->keepFrom = start;
    s->adxReady = 1;
}

static void AudioBytes(PcSfd *s, const uint8_t *p, int n) {
    long long at = s->audioBytes;
    s->audioBytes += n;
    if (s->hdrLen < HDR_MAX) {
        int k = n < HDR_MAX - s->hdrLen ? n : HDR_MAX - s->hdrLen;
        memcpy(s->hdr + s->hdrLen, p, (size_t)k);
        s->hdrLen += k;
        ParseAdxHeader(s);
    }
    if (!s->audioOn || s->skipTo || !s->adxReady || s->adxEnded)
        return;
    if (at < s->keepFrom) {
        long long d = s->keepFrom - at;
        if (d >= n)
            return;
        p += d;
        n -= (int)d;
    }
    if (s->qLen + n > s->qCap) {
        s->qCap = (s->qLen + n) * 2;
        s->q = realloc(s->q, (size_t)s->qCap);
    }
    memcpy(s->q + s->qLen, p, (size_t)n);
    s->qLen += n;
}

/* An audio or video packet's payload, after the MPEG-1 packet header. */
static void Payload(PcSfd *s, int id, const uint8_t *b, int len) {
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
        AudioBytes(s, b + k, len - k);
    else if (s->video && !s->skipTo)
        s->video(s->videoCtx, b + k, len - k);
}

int PC_SfdDemux(PcSfd *s) {
    for (;;) {
        const uint8_t *b;
        int id, len;
        if (s->ended || !Need(s, 4)) {
            s->ended = 1;
            return 0;
        }
        b = s->buf + s->bufPos;
        if (b[0] || b[1] || b[2] != 1) {    /* resynchronise on the next start code */
            s->bufPos++;
            continue;
        }
        id = b[3];
        if (id == 0xb9) {                   /* end of the program stream */
            s->ended = 1;
            return 0;
        }
        if (id == 0xba) {
            if (!Need(s, 14))
                continue;
            b = s->buf + s->bufPos;
            s->bufPos += (b[4] >> 6) == 1 ? 14 + (b[13] & 7) : 12;
            continue;
        }
        if (id < 0xbb) {                    /* not a packet: a video start code in raw data */
            s->bufPos += 4;
            continue;
        }
        if (!Need(s, 6))
            continue;
        len = Be16(s->buf + s->bufPos + 4);
        if (!Need(s, 6 + len)) {
            s->ended = 1;
            return 0;
        }
        b = s->buf + s->bufPos + 6;
        s->bufPos += 6 + len;
        if (id == 0xe0 || id == 0xc0)
            Payload(s, id, b, len);
        if (s->skipTo && s->bufBase + s->bufPos >= s->skipTo)
            s->skipTo = 0;                  /* reached the seek target: queue what follows */
        return 1;
    }
}

static int Clamp16(int x) { return x < -32768 ? -32768 : x > 32767 ? 32767 : x; }

int PC_SfdAudio(PcSfd *s, int16_t *stereo, int max, int *rate) {
    int out = 0, qPos = 0;
    while (out + 32 <= max && !s->adxEnded) {
        int ch;
        if (s->adxReady && s->qLen - qPos < s->frameBytes && s->ended) {
            s->adxEnded = 1;
            break;
        }
        if (!s->adxReady || s->qLen - qPos < s->frameBytes) {
            if (qPos) {
                memmove(s->q, s->q + qPos, (size_t)(s->qLen - qPos));
                s->qLen -= qPos;
                qPos = 0;
            }
            if (!PC_SfdDemux(s) && !s->adxReady)
                s->adxEnded = 1;
            continue;
        }
        for (ch = 0; ch < s->channels; ch++) {
            const uint8_t *fr = s->q + qPos + ch * s->chFrame;
            int scale = Be16(fr), i, *h = s->hist[ch];
            if (scale & 0x8000) {
                s->adxEnded = 1;
                break;
            }
            scale++;
            for (i = 0; i < 32; i++) {
                int nib = (fr[2 + i / 2] >> (i & 1 ? 0 : 4)) & 15;
                int smp;
                if (nib >= 8)
                    nib -= 16;
                smp = Clamp16(nib * scale + ((s->c1 * h[0]) >> 12) + ((s->c2 * h[1]) >> 12));
                h[1] = h[0];
                h[0] = smp;
                stereo[(out + i) * 2 + ch] = (int16_t)smp;
                if (s->channels == 1)
                    stereo[(out + i) * 2 + 1] = (int16_t)smp;
            }
        }
        if (s->adxEnded)
            break;
        qPos += s->frameBytes;
        out += 32;
        s->done += 32;
        if (s->total && s->done >= s->total) {
            out -= (int)(s->done - s->total);
            s->adxEnded = 1;
        }
    }
    if (qPos) {
        memmove(s->q, s->q + qPos, (size_t)(s->qLen - qPos));
        s->qLen -= qPos;
    }
    *rate = s->rate ? s->rate : 44100;
    return out;
}

static void Restart(PcSfd *s) {
    fseek(s->f, 0, SEEK_SET);
    s->bufLen = s->bufPos = 0;
    s->bufBase = 0;
    s->ended = 0;
    s->hdrLen = 0;
    s->audioBytes = 0;
    s->adxReady = s->adxEnded = 0;
    s->done = 0;
    memset(s->hist, 0, sizeof s->hist);
    s->keepFrom = 0;
    s->skipTo = 0;
    s->qLen = 0;
}

void PC_SfdSeek(PcSfd *s, long long pos) {
    Restart(s);
    if (pos <= 0)
        return;
    /* the audio offset a file position stands for is only known by reading up to it */
    s->skipTo = pos;
    while (s->skipTo && PC_SfdDemux(s))
        ;
    if (s->adxReady && !s->adxEnded) {
        long long frames = (s->audioBytes - s->dataStart + s->frameBytes - 1) / s->frameBytes;
        if (frames < 0)
            frames = 0;
        s->keepFrom = s->dataStart + frames * s->frameBytes;
        s->done = (uint32_t)(frames * 32);
    }
}

long long PC_SfdTell(const PcSfd *s) { return s->bufBase + s->bufPos; }
long long PC_SfdSize(const PcSfd *s) { return s->size; }

void PC_SfdSetVideo(PcSfd *s, PC_SfdVideoFn fn, void *ctx) {
    s->video = fn;
    s->videoCtx = ctx;
}

void PC_SfdSetAudio(PcSfd *s, int on) { s->audioOn = on; }

PcSfd *PC_SfdOpen(const char *path) {
    PcSfd *s;
    uint8_t magic[4];
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    if (fread(magic, 1, 4, f) != 4 || magic[0] || magic[1] || magic[2] != 1 || magic[3] != 0xba) {
        fclose(f);
        return NULL;
    }
    s = calloc(1, sizeof *s);
    s->f = f;
    PC_FSEEK(f, 0, SEEK_END);
    s->size = PC_FTELL(f);
    s->buf = malloc(BUF_SIZE);
    s->audioOn = 1;
    Restart(s);
    return s;
}

void PC_SfdClose(PcSfd *s) {
    if (!s)
        return;
    fclose(s->f);
    free(s->buf);
    free(s->q);
    free(s);
}

int PC_SfdAudioSeconds(const char *path) {
    PcSfd *s = PC_SfdOpen(path);
    int secs = 0, i;
    if (!s)
        return 0;
    s->audioOn = 0;
    for (i = 0; i < 64 && !s->adxReady && PC_SfdDemux(s); i++)
        ;
    if (s->adxReady && !s->adxEnded && s->rate)
        secs = (int)(s->total / (uint32_t)s->rate);
    PC_SfdClose(s);
    return secs;
}

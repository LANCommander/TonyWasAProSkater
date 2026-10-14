/* Sound (see pc_sound.h): banks of PCM samples, 24 channels, a reverb and the music input, mixed
 * to 44.1 kHz stereo on an SDL audio device.
 *
 * Banks. A VAB's sample data is the PS1's ADPCM: 16-byte blocks of 28 samples, with flags for
 * the end of a sample, whether it repeats, and where its loop starts. Loading a bank decodes every
 * sample to PCM once; a sample that loops also gets its loop decoded on its own, as the decoder
 * continues into it from the sample's end. The Dreamcast's banks (pc_dcsound.c) are PCM already:
 * their data holds stand-ins that name a registered sample.
 *
 * Channels play a sample at a pitch (0x1000: one sample per output frame, the PS1's 44.1 kHz;
 * linear interpolation), through a volume and an envelope. The envelope follows the PS1's ADSR
 * as the tones' words describe it: attack (linear, or exponential with the slower end above
 * 0x6000), exponential decay to the sustain level, sustain (either direction, linear or
 * exponential) until the key is released, then release to 0. Each phase's rate is a shift and a
 * step: the level changes by the step every 2^(shift-11) frames (several steps a frame for shifts
 * under 11). The game frees a channel once its level is 0.
 *
 * The reverb is a native one in the PS1's modes' spirit (rooms and halls as Freeverb-style comb
 * and allpass filters of those sizes, echo and delay as feedback delays), not the SPU's own
 * algorithm. Channels the game sends to it are mixed into it after their volume; its output joins
 * the effects before their level, as on the PS1. The music doesn't go through it.
 *
 * The mixer runs on SDL's audio thread (or a thread of its own with no device: the game reads the
 * envelopes, so they must keep moving). Everything that touches its state takes the lock SDL holds
 * while the mixer runs. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "pc_runtime.h"
#include "pc_sound.h"

#define RATE 44100
#define BUF_FRAMES 512
#define PITCH_ONE 0x1000u
#define FULL 0x4000         /* volumes and levels: 1.0 */
#define ENV_MAX 0x7fff

/* ---- PCM samples (the Dreamcast's banks) ------------------------------------------------------ */

/* The Dreamcast's samples are the PS1's sounds at a higher rate, and the PS1's banks tune them
 * (VAB centre note and fine tune) to play one semitone below the rate the Dreamcast files give:
 * comparing the two versions' lengths of the same sound, across banks, rates and centre notes,
 * gives 0.94-0.95 for nearly all. So the rate is scaled by that. */
#define PCM_TUNE 0.943874 /* 2^(-1/12) */
#define PCM_MAX 1024
#define PCM_MAGIC "DCPC"

typedef struct {
    int16_t *data;
    uint32_t frames;
    unsigned mul;           /* step per output frame at pitch 0x1000, in 1/0x1000 */
    int loop;
    uint32_t loopStart, loopEnd;
} Pcm;

static Pcm s_pcm[PCM_MAX];
static int s_npcm;

/* ---- banks -------------------------------------------------------------------------------------- */

typedef struct {
    uint32_t offset;        /* in the bank's data */
    int16_t *data;          /* the sample, then (if it loops) its loop again */
    uint32_t frames;        /* the sample's frames */
    uint32_t loopFrames;    /* frames of the loop after them (0: plays once) */
    const Pcm *pcm;         /* or a Dreamcast stand-in's PCM */
} Sample;

#define MAX_BANKS 8
#define BANK_SHIFT 24       /* a sample's id: (bank + 1) << BANK_SHIFT | offset */

typedef struct {
    uint8_t *data;
    uint32_t size;
    Sample **samples;       /* sorted by offset; each its own allocation (channels point at them) */
    int n, cap;
} Bank;

static Bank s_banks[MAX_BANKS];

/* ---- channels ----------------------------------------------------------------------------------- */

enum { ENV_OFF, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

typedef struct {
    int rate, decreasing, exponential;
    int step;               /* per counter overflow, scaled for shifts under 11 */
    unsigned inc;           /* counter increment per frame (0x8000 = a step every frame) */
    unsigned counter;
} Ramp;

typedef struct {
    const Sample *next;     /* what the next key on plays */
    uint16_t adsr1, adsr2;
    int volL, volR;         /* FULL = 1.0 */
    unsigned pitch;
    int reverb;
    /* playing */
    const Sample *s;
    int phase;              /* ENV_* */
    int level;              /* 0..ENV_MAX */
    int sustainLevel;
    Ramp ramp;
    uint32_t pos;           /* frame of s->data (the loop's frames follow the sample's) */
    unsigned frac;          /* 0..PITCH_ONE between prev and cur */
    int prev, cur;
} Channel;

static Channel s_ch[PC_SOUND_CHANNELS];
static int s_sfx[2] = {FULL, FULL};

/* ---- reverb --------------------------------------------------------------------------------------- */

#define COMBS 8
#define ALLPASSES 4
#define STEREO_SPREAD 23
static const int k_comb[COMBS] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
static const int k_allpass[ALLPASSES] = {556, 441, 341, 225};

typedef struct {
    float *buf;
    int len, idx;
    float store;
} Filter;

typedef struct {
    float room, damp;       /* Freeverb room size and damping */
    float echo, feedback;   /* or a delay in seconds, and its feedback */
} ReverbMode;

/* modes 1-9 (0 is off) */
static const ReverbMode k_modes[10] = {
    {0, 0, 0, 0},
    {0.55f, 0.50f, 0, 0},   /* room */
    {0.45f, 0.45f, 0, 0},   /* studio, small */
    {0.60f, 0.45f, 0, 0},   /* studio, medium */
    {0.72f, 0.40f, 0, 0},   /* studio, large */
    {0.84f, 0.30f, 0, 0},   /* hall */
    {0.92f, 0.15f, 0, 0},   /* space echo */
    {0, 0, 0.25f, 0.55f},   /* echo */
    {0, 0, 0.25f, 0.0f},    /* delay */
    {0.30f, 0.05f, 0, 0},   /* pipe */
};

#define ECHO_MAX (RATE / 2)

static struct {
    int mode;
    float depth[2];
    uint32_t channels;
    Filter comb[2][COMBS], allpass[2][ALLPASSES];
    float echo[2][ECHO_MAX];
    int echoIdx, echoLen;
} s_rev;

/* ---- the music input -------------------------------------------------------------------------- */

/* A FIFO of stereo frames at the stream's rate, resampled to RATE. Delivery comes in bursts (one
 * XA sector every 8 or 16 sectors read), so playback waits for a prebuffer after starting or
 * running dry. */
#define MUSIC_FIFO 131072u
#define MUSIC_PREBUFFER 8192u
static int16_t s_mBuf[MUSIC_FIFO * 2];
static unsigned s_mRead, s_mCount;
static uint64_t s_mConsumed;
static unsigned s_mRate = 37800;
static unsigned s_mPhase;       /* 16.16 between prev and cur */
static int s_mPrimed;
static int s_mPrev[2], s_mCur[2];
static int s_mMix[4] = {0x80, 0x00, 0x80, 0x00}; /* L->L, L->R, R->R, R->L */
static int s_mLevel[2] = {FULL, FULL};
static int s_mMuted;

/* ---- device ------------------------------------------------------------------------------------ */

static SDL_AudioDeviceID s_dev;
static SDL_mutex *s_mx;
static int s_running;
static FILE *s_dump;
static int s_trace;

static void Lock(void) {
    if (s_mx)
        SDL_LockMutex(s_mx);
}

static void Unlock(void) {
    if (s_mx)
        SDL_UnlockMutex(s_mx);
}

static int Clamp16(int x) { return x < -32768 ? -32768 : x > 32767 ? 32767 : x; }

/* ---- ADPCM ------------------------------------------------------------------------------------- */

/* PS-ADPCM filter coefficients, in 1/64. */
static const int k_f0[5] = {0, 60, 115, 98, 122};
static const int k_f1[5] = {0, 0, -52, -55, -60};

static void DecodeBlock(const uint8_t *b, int16_t *out, int *h1, int *h2) {
    int shift = b[0] & 0xf, filt = (b[0] >> 4) & 0xf;
    if (shift > 12)
        shift = 9;
    if (filt > 4)
        filt = 4;
    for (int i = 0; i < 28; i++) {
        int nib = (b[2 + i / 2] >> (i & 1 ? 4 : 0)) & 0xf;
        int s = ((nib >= 8 ? nib - 16 : nib) * 4096) >> shift;
        s = Clamp16(s + ((*h1 * k_f0[filt] + *h2 * k_f1[filt] + 32) >> 6));
        out[i] = (int16_t)s;
        *h2 = *h1;
        *h1 = s;
    }
}

/* The sample starting at `offset` of a bank's data: its blocks up to the one flagged as the end
 * (bit 0; bit 1: it repeats from the last block flagged as the loop start, bit 2). */
static int DecodeSample(const Bank *bk, uint32_t offset, Sample *out) {
    uint32_t end = offset, loop = offset, blocks, loopBlocks = 0, i;
    int repeat = 0, h1 = 0, h2 = 0;
    memset(out, 0, sizeof *out);
    out->offset = offset;
    if (offset + 16 > bk->size)
        return 0;
    if (!memcmp(bk->data + offset + 2, PCM_MAGIC, 4)) {
        int id;
        memcpy(&id, bk->data + offset + 6, 4);
        if (id >= 0 && id < s_npcm)
            out->pcm = &s_pcm[id];
        return 1;
    }
    for (;;) {
        uint8_t f = bk->data[end + 1];
        if (f & 4)
            loop = end;
        end += 16;
        if ((f & 1) || end + 16 > bk->size) {
            repeat = (f & 3) == 3;
            break;
        }
    }
    blocks = (end - offset) / 16;
    if (repeat)
        loopBlocks = (end - loop) / 16;
    out->frames = blocks * 28;
    out->loopFrames = loopBlocks * 28;
    out->data = malloc(((size_t)out->frames + out->loopFrames) * sizeof *out->data);
    if (!out->data)
        return 0;
    for (i = 0; i < blocks; i++)
        DecodeBlock(bk->data + offset + i * 16, out->data + i * 28, &h1, &h2);
    /* the loop as it sounds the second time round: the decoder carries on from the end */
    for (i = 0; i < loopBlocks; i++)
        DecodeBlock(bk->data + loop + i * 16, out->data + out->frames + i * 28, &h1, &h2);
    return 1;
}

static Sample *AddSample(Bank *bk, uint32_t offset) {
    Sample *s = malloc(sizeof *s);
    int i;
    if (!s)
        return NULL;
    if (!DecodeSample(bk, offset, s)) {
        free(s);
        return NULL;
    }
    if (bk->n == bk->cap) {
        Sample **n = realloc(bk->samples, (size_t)(bk->cap ? bk->cap * 2 : 64) * sizeof *n);
        if (!n) {
            free(s->data);
            free(s);
            return NULL;
        }
        bk->samples = n;
        bk->cap = bk->cap ? bk->cap * 2 : 64;
    }
    for (i = bk->n; i > 0 && bk->samples[i - 1]->offset > offset; i--)
        bk->samples[i] = bk->samples[i - 1];
    bk->samples[i] = s;
    bk->n++;
    return s;
}

static void FreeBank(Bank *bk) {
    for (int i = 0; i < bk->n; i++) {
        free(bk->samples[i]->data);
        free(bk->samples[i]);
    }
    free(bk->samples);
    free(bk->data);
    memset(bk, 0, sizeof *bk);
}

void PC_SoundBanksReset(void) {
    Lock();
    for (int c = 0; c < PC_SOUND_CHANNELS; c++) {
        s_ch[c].s = s_ch[c].next = NULL;
        s_ch[c].phase = ENV_OFF;
        s_ch[c].level = 0;
    }
    for (int b = 0; b < MAX_BANKS; b++)
        FreeBank(&s_banks[b]);
    Unlock();
}

uint32_t PC_SoundBankLoad(const void *data, uint32_t size) {
    Bank *bk = NULL;
    uint32_t off = 0;
    int b;
    for (b = 0; b < MAX_BANKS && s_banks[b].data; b++)
        ;
    if (b == MAX_BANKS || !size) {
        PC_Log("sound: no room for another bank");
        return 0;
    }
    bk = &s_banks[b];
    bk->data = malloc(size);
    if (!bk->data)
        return 0;
    memcpy(bk->data, data, size);
    bk->size = size;
    /* the samples one after another: each ends at a block flagged as the end */
    while (off + 16 <= size) {
        Sample *s = AddSample(bk, off);
        if (!s)
            break;
        off += s->pcm ? PC_SOUND_PCM_STUB : (s->frames / 28) * 16;
    }
    PC_Trace("sound: bank %d, %u bytes, %d samples", b, size, bk->n);
    return (uint32_t)(b + 1) << BANK_SHIFT;
}

/* The sample at id (a bank's base + an offset): decoded now if it isn't one the bank's scan found. */
static const Sample *FindSample(uint32_t id) {
    int b = (int)(id >> BANK_SHIFT) - 1;
    uint32_t off = id & ((1u << BANK_SHIFT) - 1);
    Bank *bk;
    int lo, hi;
    if (b < 0 || b >= MAX_BANKS || !(bk = &s_banks[b])->data || off >= bk->size)
        return NULL;
    for (lo = 0, hi = bk->n - 1; lo <= hi;) {
        int m = (lo + hi) / 2;
        if (bk->samples[m]->offset == off)
            return bk->samples[m];
        if (bk->samples[m]->offset < off)
            lo = m + 1;
        else
            hi = m - 1;
    }
    return AddSample(bk, off);
}

/* ---- envelope ---------------------------------------------------------------------------------- */

static void RampSet(Ramp *r, int rate, int decreasing, int exponential) {
    int shift = rate >> 2;
    r->rate = rate;
    r->decreasing = decreasing;
    r->exponential = exponential;
    r->step = decreasing ? -8 + (rate & 3) : 7 - (rate & 3);
    if (shift < 11)
        r->step *= 1 << (11 - shift);
    r->inc = shift > 11 ? (shift - 11 >= 16 ? 0 : 0x8000u >> (shift - 11)) : 0x8000u;
    r->counter = 0;
}

/* One frame of the ramp; the level it reaches. */
static int RampTick(Ramp *r, int level) {
    int step = r->step;
    unsigned inc = r->inc;
    if (r->exponential) {
        if (r->decreasing) {
            step = (step * level) >> 15;
        } else if (level >= 0x6000) {
            if (r->rate < 40) {
                step >>= 2;
            } else if (r->rate >= 44) {
                inc >>= 2;
            } else {
                step >>= 1;
                inc >>= 1;
            }
        }
    }
    r->counter += inc;
    if (!(r->counter & 0x8000))
        return level;
    r->counter = 0;
    level += step;
    return level < 0 ? 0 : level > ENV_MAX ? ENV_MAX : level;
}

static void EnterPhase(Channel *c, int phase) {
    c->phase = phase;
    switch (phase) {
    case ENV_ATTACK:
        RampSet(&c->ramp, (c->adsr1 >> 8) & 0x7f, 0, c->adsr1 >> 15);
        break;
    case ENV_DECAY:
        RampSet(&c->ramp, ((c->adsr1 >> 4) & 0xf) << 2, 1, 1);
        break;
    case ENV_SUSTAIN:
        RampSet(&c->ramp, (c->adsr2 >> 6) & 0x7f, (c->adsr2 >> 14) & 1, c->adsr2 >> 15);
        break;
    case ENV_RELEASE:
        RampSet(&c->ramp, (c->adsr2 & 0x1f) << 2, 1, (c->adsr2 >> 5) & 1);
        break;
    }
}

static void EnvTick(Channel *c) {
    c->level = RampTick(&c->ramp, c->level);
    switch (c->phase) {
    case ENV_ATTACK:
        if (c->level >= ENV_MAX)
            EnterPhase(c, ENV_DECAY);
        break;
    case ENV_DECAY:
        if (c->level <= c->sustainLevel)
            EnterPhase(c, ENV_SUSTAIN);
        break;
    case ENV_RELEASE:
        if (c->level <= 0) {
            c->phase = ENV_OFF;
            c->s = NULL;
        }
        break;
    }
}

/* ---- channels ---------------------------------------------------------------------------------- */

int PC_SoundNotePitch(int note, int fine, int centre, int centreFine) {
    double d = (double)(note - centre) * 128.0 + (double)(fine - centreFine);
    double p = 4096.0 * pow(2.0, d / (12.0 * 128.0));
    return p < 0.0 ? 0 : p > 0x3fff ? 0x3fff : (int)(p + 0.5);
}

static int Ok(int ch) { return ch >= 0 && ch < PC_SOUND_CHANNELS; }

void PC_SoundChannelSet(int ch, uint32_t sample, int pitch, int volL, int volR, uint16_t adsr1,
                        uint16_t adsr2) {
    if (!Ok(ch))
        return;
    Lock();
    s_ch[ch].next = FindSample(sample);
    s_ch[ch].pitch = (unsigned)pitch & 0x3fff;
    s_ch[ch].volL = volL;
    s_ch[ch].volR = volR;
    s_ch[ch].adsr1 = adsr1;
    s_ch[ch].adsr2 = adsr2;
    Unlock();
    if (!s_ch[ch].next)
        PC_Trace("sound: channel %d: no sample at %08x", ch, sample);
}

void PC_SoundChannelVolume(int ch, int volL, int volR) {
    if (!Ok(ch))
        return;
    Lock();
    s_ch[ch].volL = volL;
    s_ch[ch].volR = volR;
    Unlock();
}

void PC_SoundChannelPitch(int ch, int pitch) {
    if (!Ok(ch))
        return;
    Lock();
    s_ch[ch].pitch = (unsigned)pitch & 0x3fff;
    Unlock();
}

int PC_SoundChannelGetPitch(int ch) { return Ok(ch) ? (int)s_ch[ch].pitch : 0; }

int PC_SoundChannelLevel(int ch) {
    int l;
    if (!Ok(ch))
        return 0;
    Lock();
    l = s_ch[ch].level;
    Unlock();
    return l;
}

void PC_SoundKeyOn(uint32_t channels) {
    Lock();
    for (int n = 0; n < PC_SOUND_CHANNELS; n++) {
        Channel *c = &s_ch[n];
        if (!(channels >> n & 1))
            continue;
        c->s = c->next;
        c->pos = 0;
        c->frac = 0;
        c->prev = c->cur = 0;
        c->level = 0;
        c->sustainLevel = ((c->adsr1 & 0xf) + 1) * 0x800;
        EnterPhase(c, c->s ? ENV_ATTACK : ENV_OFF);
        if (s_trace)
            PC_Log("sound: key on %d: sample %u pitch %04x vol %d/%d adsr %04x %04x", n,
                   c->s ? c->s->offset : 0, c->pitch, c->volL, c->volR, c->adsr1, c->adsr2);
    }
    Unlock();
}

void PC_SoundKeyOff(uint32_t channels) {
    Lock();
    for (int n = 0; n < PC_SOUND_CHANNELS; n++)
        if ((channels >> n & 1) && s_ch[n].phase != ENV_OFF && s_ch[n].phase != ENV_RELEASE) {
            EnterPhase(&s_ch[n], ENV_RELEASE);
            if (s_trace)
                PC_Log("sound: key off %d (level %d)", n, s_ch[n].level);
        }
    Unlock();
}

/* The channel's next source frame, or 0 at the end of a sample that doesn't repeat. */
static int NextFrame(Channel *c, int *out) {
    const Sample *s = c->s;
    if (s->pcm) {
        const Pcm *p = s->pcm;
        uint32_t end = p->loop && p->loopEnd ? p->loopEnd : p->frames;
        if (c->pos >= end) {
            if (!p->loop || p->loopStart >= end)
                return 0;
            c->pos = p->loopStart;
        }
        *out = p->data[c->pos++];
        return 1;
    }
    if (c->pos >= s->frames + s->loopFrames) {
        if (!s->loopFrames)
            return 0;
        c->pos = s->frames;
    }
    *out = s->data[c->pos++];
    return 1;
}

/* ---- the mix ------------------------------------------------------------------------------------- */

void PC_SoundLevels(int sfxL, int sfxR, int musicL, int musicR) {
    Lock();
    s_sfx[0] = sfxL;
    s_sfx[1] = sfxR;
    s_mLevel[0] = musicL;
    s_mLevel[1] = musicR;
    Unlock();
}

static void FilterInit(Filter *f, int len) {
    free(f->buf);
    f->buf = calloc((size_t)len, sizeof *f->buf);
    f->len = len;
    f->idx = 0;
    f->store = 0;
}

int PC_SoundReverbMode(int mode) {
    if (mode < 0 || mode > 9)
        return 0;
    Lock();
    if (mode != s_rev.mode) {
        for (int ch = 0; ch < 2; ch++) {
            for (int i = 0; i < COMBS; i++)
                FilterInit(&s_rev.comb[ch][i], k_comb[i] + ch * STEREO_SPREAD);
            for (int i = 0; i < ALLPASSES; i++)
                FilterInit(&s_rev.allpass[ch][i], k_allpass[i] + ch * STEREO_SPREAD);
        }
        memset(s_rev.echo, 0, sizeof s_rev.echo);
        s_rev.echoIdx = 0;
        s_rev.echoLen = (int)(k_modes[mode].echo * RATE);
        if (s_rev.echoLen > ECHO_MAX)
            s_rev.echoLen = ECHO_MAX;
        PC_Trace("sound: reverb mode %d", mode);
    }
    s_rev.mode = mode;
    s_rev.depth[0] = s_rev.depth[1] = 0; /* as the PS1 does: the depth is set again after */
    Unlock();
    return 1;
}

void PC_SoundReverbDepth(int left, int right) {
    Lock();
    s_rev.depth[0] = (float)left / ENV_MAX;
    s_rev.depth[1] = (float)right / ENV_MAX;
    Unlock();
}

void PC_SoundReverbChannels(int on, uint32_t channels) {
    Lock();
    s_rev.channels = on ? s_rev.channels | channels : s_rev.channels & ~channels;
    Unlock();
}

/* One frame of the reverb's wet output for its input (both in 16-bit sample units). */
static void ReverbFrame(float inL, float inR, float *outL, float *outR) {
    const ReverbMode *m = &k_modes[s_rev.mode];
    float in[2] = {inL, inR};
    for (int ch = 0; ch < 2; ch++) {
        float out = 0;
        if (s_rev.echoLen) {
            float *e = &s_rev.echo[ch][s_rev.echoIdx];
            out = *e;
            *e = in[ch] + out * m->feedback;
        } else {
            float x = in[ch] * 0.015f;
            for (int i = 0; i < COMBS; i++) {
                Filter *f = &s_rev.comb[ch][i];
                float y = f->buf[f->idx];
                f->store = y * (1 - m->damp) + f->store * m->damp;
                f->buf[f->idx] = x + f->store * (0.7f + 0.28f * m->room);
                f->idx = (f->idx + 1) % f->len;
                out += y;
            }
            for (int i = 0; i < ALLPASSES; i++) {
                Filter *f = &s_rev.allpass[ch][i];
                float y = f->buf[f->idx];
                f->buf[f->idx] = out + y * 0.5f;
                f->idx = (f->idx + 1) % f->len;
                out = y - out;
            }
            out *= 3.0f;
        }
        *(ch ? outR : outL) = out * s_rev.depth[ch];
    }
    if (s_rev.echoLen)
        s_rev.echoIdx = (s_rev.echoIdx + 1) % s_rev.echoLen;
}

/* The next music frame at RATE, through its mix and level. */
static void MusicFrame(int *outL, int *outR) {
    *outL = *outR = 0;
    if (!s_mPrimed) {
        if (s_mCount < MUSIC_PREBUFFER)
            return;
        s_mPrimed = 1;
    }
    s_mPhase += (s_mRate << 16) / RATE;
    while (s_mPhase >= 0x10000) {
        s_mPhase -= 0x10000;
        s_mPrev[0] = s_mCur[0];
        s_mPrev[1] = s_mCur[1];
        if (!s_mCount) { /* ran dry: silence until the prebuffer refills */
            s_mPrimed = 0;
            s_mCur[0] = s_mCur[1] = 0;
            return;
        }
        s_mCur[0] = s_mBuf[s_mRead * 2];
        s_mCur[1] = s_mBuf[s_mRead * 2 + 1];
        s_mRead = (s_mRead + 1) % MUSIC_FIFO;
        s_mCount--;
        s_mConsumed++;
    }
    if (s_mMuted)
        return;
    int f = (int)(s_mPhase >> 4); /* a 12-bit fraction keeps the products in range */
    int cl = s_mPrev[0] + (((s_mCur[0] - s_mPrev[0]) * f) >> 12);
    int cr = s_mPrev[1] + (((s_mCur[1] - s_mPrev[1]) * f) >> 12);
    int l = Clamp16((cl * s_mMix[0] + cr * s_mMix[3]) >> 7);
    int r = Clamp16((cl * s_mMix[1] + cr * s_mMix[2]) >> 7);
    *outL = (l * s_mLevel[0]) >> 14;
    *outR = (r * s_mLevel[1]) >> 14;
}

static void Mix(Uint8 *stream, int len) {
    int16_t *out = (int16_t *)stream;
    int frames = len / 4;
    for (int f = 0; f < frames; f++) {
        int L = 0, R = 0, revL = 0, revR = 0;
        for (int n = 0; n < PC_SOUND_CHANNELS; n++) {
            Channel *c = &s_ch[n];
            if (!c->s)
                continue;
            EnvTick(c);
            if (!c->s)
                continue;
            c->frac += c->s->pcm ? (c->pitch * c->s->pcm->mul) >> 12 : c->pitch;
            while (c->frac >= PITCH_ONE) {
                int nx;
                c->frac -= PITCH_ONE;
                c->prev = c->cur;
                if (!NextFrame(c, &nx)) {
                    /* the end of a sample that doesn't repeat: silent, level 0 */
                    c->s = NULL;
                    c->phase = ENV_OFF;
                    c->level = 0;
                    break;
                }
                c->cur = nx;
            }
            if (!c->s)
                continue;
            int s = c->prev + (((c->cur - c->prev) * (int)c->frac) >> 12);
            int x = (s * c->level) >> 15;
            int l = (x * c->volL) >> 14, r = (x * c->volR) >> 14;
            L += l;
            R += r;
            if (s_rev.channels >> n & 1) {
                revL += l;
                revR += r;
            }
        }
        if (s_rev.mode && (s_rev.depth[0] > 0 || s_rev.depth[1] > 0)) {
            float wl, wr;
            ReverbFrame((float)revL, (float)revR, &wl, &wr);
            L += (int)wl;
            R += (int)wr;
        }
        int ml, mr;
        MusicFrame(&ml, &mr);
        out[2 * f] = (int16_t)Clamp16((int)(((int64_t)L * s_sfx[0]) >> 14) + ml);
        out[2 * f + 1] = (int16_t)Clamp16((int)(((int64_t)R * s_sfx[1]) >> 14) + mr);
    }
    if (s_dump)
        fwrite(stream, 1, (size_t)frames * 4, s_dump);
}

static void DeviceCallback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    Lock();
    Mix(stream, len);
    Unlock();
}

/* No device (or THPS_AUDIO=0): mix in real time on a thread anyway, paced on the performance
 * counter, so the envelopes the game reads keep moving; the dump, if any, records it. */
static int Pacer(void *userdata) {
    Uint8 buf[BUF_FRAMES * 4];
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 next = SDL_GetPerformanceCounter();
    (void)userdata;
    while (s_running) {
        Lock();
        Mix(buf, sizeof buf);
        Unlock();
        next += freq * BUF_FRAMES / RATE;
        for (Uint64 now = SDL_GetPerformanceCounter(); now < next && s_running;
             now = SDL_GetPerformanceCounter())
            SDL_Delay(1);
    }
    return 0;
}

static void StartPacer(void) {
    s_running = 1;
    if (!SDL_CreateThread(Pacer, "sound-pacer", NULL))
        PC_Log("audio: cannot start the mixer thread: %s", SDL_GetError());
}

void PC_SoundInit(void) {
    const char *audio = getenv("THPS_AUDIO");
    const char *dump = getenv("THPS_AUDIO_DUMP");
    SDL_AudioSpec want, have;

    s_trace = getenv("THPS_TRACE_SOUND") && atoi(getenv("THPS_TRACE_SOUND"));
    s_mx = SDL_CreateMutex();
    PC_SoundReverbMode(0);
    if (dump && !(s_dump = fopen(dump, "wb")))
        PC_Log("audio: cannot create %s", dump);
    if (audio && audio[0] == '0') {
        PC_Log("audio: off (THPS_AUDIO=0)");
        StartPacer();
        return;
    }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        PC_Log("audio: SDL audio init failed: %s", SDL_GetError());
        StartPacer();
        return;
    }
    SDL_zero(want);
    want.freq = RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = BUF_FRAMES;
    want.callback = DeviceCallback;
    s_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!s_dev) {
        PC_Log("audio: cannot open an audio device: %s", SDL_GetError());
        StartPacer();
        return;
    }
    SDL_PauseAudioDevice(s_dev, 0);
    PC_Log("audio: %d Hz, %d channels, %d-frame buffers", have.freq, have.channels, have.samples);
}

/* ---- the music --------------------------------------------------------------------------------- */

void PC_SoundMusicQueue(const int16_t *stereo, int frames, int rate) {
    Lock();
    s_mRate = (unsigned)rate;
    for (int i = 0; i < frames; i++) {
        unsigned w;
        if (s_mCount == MUSIC_FIFO) { /* overrun (the game running fast): drop the oldest */
            s_mRead = (s_mRead + 1) % MUSIC_FIFO;
            s_mCount--;
            s_mConsumed++;
        }
        w = (s_mRead + s_mCount) % MUSIC_FIFO;
        s_mBuf[w * 2] = stereo[i * 2];
        s_mBuf[w * 2 + 1] = stereo[i * 2 + 1];
        s_mCount++;
    }
    Unlock();
}

unsigned PC_SoundMusicQueued(void) {
    unsigned n;
    Lock();
    n = s_mCount;
    Unlock();
    return n;
}

uint64_t PC_SoundMusicConsumed(void) {
    uint64_t n;
    Lock();
    n = s_mConsumed;
    Unlock();
    return n;
}

void PC_SoundMusicFlush(void) {
    Lock();
    s_mConsumed += s_mCount; /* dropped frames count as gone by */
    s_mRead = s_mCount = 0;
    s_mPhase = 0;
    s_mPrimed = 0;
    s_mPrev[0] = s_mPrev[1] = s_mCur[0] = s_mCur[1] = 0;
    Unlock();
}

void PC_SoundMusicMix(const uint8_t levels[4]) {
    Lock();
    for (int i = 0; i < 4; i++)
        s_mMix[i] = levels[i];
    Unlock();
}

void PC_SoundMusicMute(int mute) {
    Lock();
    s_mMuted = mute;
    Unlock();
}

/* ---- PCM samples ----------------------------------------------------------------------------- */

int PC_SoundPcmAdd(const int16_t *pcm, uint32_t frames, int rate, int loop, uint32_t loopStart,
                   uint32_t loopEnd) {
    Pcm *p;
    int id;
    Lock();
    if (s_npcm == PCM_MAX) {
        Unlock();
        return -1;
    }
    id = s_npcm;
    p = &s_pcm[id];
    p->data = malloc((frames ? frames : 1) * sizeof *p->data);
    memcpy(p->data, pcm, frames * sizeof *p->data);
    p->frames = frames;
    p->mul = (unsigned)(rate * PCM_TUNE / RATE * 4096.0 + 0.5);
    p->loop = loop;
    p->loopStart = loopStart;
    p->loopEnd = loopEnd > frames ? frames : loopEnd;
    s_npcm++;
    Unlock();
    return id;
}

int PC_SoundPcmLooped(int id) { return id >= 0 && id < s_npcm && s_pcm[id].loop; }

/* Block 0 carries the marker and the id, block 1 ends the stand-in (it also reads as near-silent
 * ADPCM). */
void PC_SoundPcmStub(int id, uint8_t out[PC_SOUND_PCM_STUB]) {
    memset(out, 0, PC_SOUND_PCM_STUB);
    out[0] = 12;
    memcpy(out + 2, PCM_MAGIC, 4);
    memcpy(out + 6, &id, 4);
    out[16 + 1] = 1;
}

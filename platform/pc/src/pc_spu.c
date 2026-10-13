/* SPU: sound RAM, 24 ADPCM voices, master volume, and the mixer that turns them into 44.1 kHz
 * stereo on an SDL audio device. The libspu/libsnd entry points the game calls are implemented
 * here on top of that model.
 *
 * What is modelled: PS-ADPCM blocks (28 samples per 16 bytes, loop/end flags), pitch stepping with
 * linear interpolation, per-voice L/R volume, the ADSR envelope as linear ramps, key on/off, and
 * the master volume, and the CD audio input (the music: decoded XA or ADX from pc_stream.c, at the
 * CD volume and the stream's mix levels, added after the master volume). Voices can also play PCM samples
 * (the Dreamcast's sound banks): see PC_SpuPcmAdd. Not modelled yet: reverb, sweep
 * volume, exponential ADSR curves, noise. Rates and timings are approximate, not bit-exact.
 *
 * The mixer runs on SDL's audio thread. Every game-side call that touches voice state takes the
 * device lock, which is also what SDL holds while the callback runs. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "pc_runtime.h"
#include "pc_spu.h"

#define SPU_RATE 44100
#define SPU_BUF_FRAMES 512
#define SPU_PITCH_ONE 0x1000u     /* pitch register value that steps one sample per output frame */
#define SPU_ALLOC_BASE 0x1010u    /* SpuMalloc hands out memory above the reserved low SPU RAM */
#define SPU_ENV_MAX 0x7fff

/* libspu ABI: the structs the game passes in (same layout as Psy-Q's). */
typedef struct {
    short left;
    short right;
} PcSpuVolume;

typedef struct {
    unsigned long voice;     /* voice bit mask (Set) / voice bit (Get) */
    unsigned long mask;      /* which fields below are valid (SPU_VOICE_*) */
    PcSpuVolume volume;
    PcSpuVolume volmode;
    PcSpuVolume volumex;     /* current volume (Get only) */
    unsigned short pitch;
    unsigned short note;
    unsigned short sample_note;
    short envx;              /* current envelope level (Get only) */
    unsigned long addr;      /* sample start address in sound RAM */
    unsigned long loop_addr;
    long a_mode, s_mode, r_mode;
    unsigned short ar, dr, sr, rr, sl;
    unsigned short adsr1, adsr2;
} PcSpuVoiceAttr;

typedef struct {
    PcSpuVolume volume;
    long reverb;
    long mix;
} PcSpuExtAttr;

typedef struct {
    unsigned long mask;
    PcSpuVolume mvol;        /* master volume */
    PcSpuVolume mvolmode;
    PcSpuVolume mvolx;
    PcSpuExtAttr cd;
    PcSpuExtAttr ext;
} PcSpuCommonAttr;

/* SpuVoiceAttr.mask bits and SpuCommonAttr.mask bits (the values the game uses). */
#define VA_VOLL 0x001
#define VA_VOLR 0x002
#define VA_PITCH 0x010
#define VA_NOTE 0x020
#define VA_SAMPLE_NOTE 0x040
#define VA_WDSA 0x080
#define VA_ADSR_AR 0x800
#define VA_ADSR_DR 0x1000
#define VA_ADSR_SR 0x2000
#define VA_ADSR_RR 0x4000
#define VA_ADSR_SL 0x8000
#define VA_LSAX 0x10000
#define VA_ADSR1 0x20000
#define VA_ADSR2 0x40000
#define CA_MVOLL 0x001
#define CA_MVOLR 0x002
#define CA_CDVOLL 0x040
#define CA_CDVOLR 0x080
#define CA_CDMIX 0x200

enum { ENV_OFF, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

/* PCM samples. The Dreamcast's samples are the PS1's sounds at a higher rate, and the PS1's
 * banks tune them (VAB centre note and fine tune) to play one semitone below the rate the
 * Dreamcast files give: comparing the two versions' lengths of the same sound, across banks,
 * rates and centre notes, gives 0.94-0.95 for nearly all. So the rate is scaled by that. */
#define PC_SPU_PCM_TUNE 0.943874     /* 2^(-1/12) */
#define PCM_MAX 1024
#define PCM_MAGIC "DCPC"

typedef struct {
    int16_t *data;
    uint32_t frames;
    unsigned mul;           /* SPU_PITCH_ONE-based step per output frame at pitch 0x1000 */
    int loop;
    uint32_t loopStart, loopEnd;
} PcmSample;

static PcmSample s_pcm[PCM_MAX];
static int s_npcm;

typedef struct {
    int playing;            /* keyed (or releasing) and not finished */
    int env_phase, env_ctr;
    int env;                /* 0..SPU_ENV_MAX */
    int volL, volR;
    unsigned pitch;         /* SPU_PITCH_ONE = one sample per output frame */
    unsigned note, sample_note;
    unsigned start, loop;   /* byte addresses in sound RAM */
    /* ADSR, unpacked from the register bits */
    int a_shift, a_step, d_shift, s_level, s_shift, s_dec, r_shift;
    /* ADPCM decoder */
    unsigned blk;           /* 16-byte block being decoded */
    int started, idx, flags;
    short dec[28];
    int h1, h2;
    /* playback position: phase is the fraction (0..SPU_PITCH_ONE) between prev and cur */
    unsigned phase;
    int prev, cur;
    /* or a PCM sample, at frame pcmPos */
    const PcmSample *pcm;
    uint32_t pcmPos;
} Voice;

static uint8_t s_ram[PC_SPU_RAM_SIZE];
static Voice s_v[PC_SPU_VOICES];
static unsigned s_xfer;
static unsigned s_alloc = SPU_ALLOC_BASE;
static int s_mvol[2] = {SPU_ENV_MAX, SPU_ENV_MAX};
static SDL_AudioDeviceID s_dev;
static SDL_mutex *s_mx;          /* guards the voices against the mixer (device or pacer thread) */
static int s_running;            /* the pacer thread runs the mixer when there is no device */
static FILE *s_dump;
static int s_trace;
static int s_traceFrames;

/* CD audio input: a FIFO of stereo frames at the stream's rate, resampled to SPU_RATE. Delivery
 * comes in bursts (one XA sector every 8 or 16 sectors read), so playback waits for a prebuffer
 * after starting or running dry. */
#define CD_FIFO_FRAMES 131072u
#define CD_PREBUFFER 8192u
static int16_t s_cdBuf[CD_FIFO_FRAMES * 2];
static unsigned s_cdRead, s_cdCount;      /* oldest frame, frames queued */
static uint64_t s_cdConsumed;             /* frames taken from the queue, ever */
static unsigned s_cdRate = 37800;
static unsigned s_cdPhase;                /* 16.16 position between cdPrev and cdCur */
static int s_cdPrimed;
static int s_cdPrev[2], s_cdCur[2];
static int s_cdAtv[4] = {0x80, 0x00, 0x80, 0x00}; /* L->L, L->R, R->R, R->L */
static int s_cdVol[2] = {SPU_ENV_MAX, SPU_ENV_MAX};
static int s_cdMix = 1;
static int s_cdMuted;

/* PS-ADPCM filter coefficients, in 1/64 (the SPU's K0/K1 pairs). */
static const int k_f0[5] = {0, 60, 115, 98, 122};
static const int k_f1[5] = {0, 0, -52, -55, -60};

static void Lock(void) {
    if (s_mx)
        SDL_LockMutex(s_mx);
}

static void Unlock(void) {
    if (s_mx)
        SDL_UnlockMutex(s_mx);
}

static int Clamp16(int x) { return x < -32768 ? -32768 : x > 32767 ? 32767 : x; }

static int ClampEnv(int x) { return x < 0 ? 0 : x > SPU_ENV_MAX ? SPU_ENV_MAX : x; }

/* Pitch register value for `note` relative to `cn` (both in 1/12-octave steps, 60 = middle C; fine
 * and cf in 1/128 of a step). 0x1000 is the sample's own rate. */
static unsigned NoteToPitch(int note, int fine, int cn, int cf) {
    double d = (double)(note - cn) * 128.0 + (double)(fine - cf);
    double p = 4096.0 * pow(2.0, d / (12.0 * 128.0));
    if (p < 0.0)
        p = 0.0;
    if (p > 0x3fff)
        p = 0x3fff;
    return (unsigned)(p + 0.5);
}

/* Decode the next 16-byte ADPCM block into v->dec. Returns 0 when a one-shot sample has ended. */
static int DecodeBlock(Voice *v) {
    if (v->started) {
        if (v->flags & 1) {                  /* end of sample: repeat from loop, or stop */
            if (!(v->flags & 2))
                return 0;
            v->blk = v->loop;
        } else {
            v->blk += 16;
        }
    }
    v->started = 1;

    const uint8_t *b = &s_ram[v->blk & (PC_SPU_RAM_SIZE - 1) & ~15u];
    v->flags = b[1];
    if (b[1] & 4)                            /* loop-start marker */
        v->loop = v->blk;

    int shift = b[0] & 0xf;
    if (shift > 12)
        shift = 9;
    int filt = (b[0] >> 4) & 0xf;
    if (filt > 4)
        filt = 4;

    for (int i = 0; i < 14; i++) {
        for (int half = 0; half < 2; half++) {
            int nib = half ? (b[2 + i] >> 4) : (b[2 + i] & 0xf);
            if (nib >= 8)
                nib -= 16;
            int s = (nib * 4096) >> shift;
            s += (v->h1 * k_f0[filt] + v->h2 * k_f1[filt] + 32) >> 6;
            s = Clamp16(s);
            v->dec[2 * i + half] = (short)s;
            v->h2 = v->h1;
            v->h1 = s;
        }
    }
    v->idx = 0;
    return 1;
}

static int NextSample(Voice *v, int *out) {
    if (v->pcm) {
        const PcmSample *p = v->pcm;
        uint32_t end = p->loop && p->loopEnd ? p->loopEnd : p->frames;
        if (v->pcmPos >= end) {
            if (!p->loop || p->loopStart >= end)
                return 0;
            v->pcmPos = p->loopStart;
        }
        *out = p->data[v->pcmPos++];
        return 1;
    }
    if (v->idx >= 28 && !DecodeBlock(v))
        return 0;
    *out = v->dec[v->idx++];
    return 1;
}

static void KeyOn(Voice *v, int n) {
    v->playing = 1;
    v->env_phase = ENV_ATTACK;
    v->env = 0;
    v->env_ctr = 0;
    v->blk = v->start;
    v->started = 0;
    v->idx = 28;                             /* forces a decode on the first sample */
    v->h1 = v->h2 = 0;
    v->phase = 0;
    v->prev = v->cur = 0;
    v->pcm = NULL;
    v->pcmPos = 0;
    {
        /* a PCM sample's stand-in */
        const uint8_t *b = &s_ram[v->start & (PC_SPU_RAM_SIZE - 1) & ~15u];
        int id;
        memcpy(&id, b + 6, 4);
        if (!memcmp(b + 2, PCM_MAGIC, 4) && id >= 0 && id < s_npcm)
            v->pcm = &s_pcm[id];
    }
    if (s_trace)
        PC_Log("spu: key on voice %d: addr %05x loop %05x pitch %04x vol %d/%d pcm %d", n,
               v->start, v->loop, v->pitch, v->volL, v->volR,
               v->pcm ? (int)(v->pcm - s_pcm) : -1);
}

static void KeyOff(Voice *v, int n) {
    if (!v->playing || v->env_phase == ENV_RELEASE)
        return;
    v->env_phase = ENV_RELEASE;
    v->env_ctr = 0;
    if (s_trace)
        PC_Log("spu: key off voice %d (env %d)", n, v->env);
}

/* One envelope step of a linear ramp. The step of `step` is applied every 2^(shift-11) output
 * frames (shift <= 11: several times per frame, scaled by 2^(11-shift)). */
static void Ramp(Voice *v, int shift, int step) {
    int period = 1, inc = step;
    if (shift > 26)
        shift = 26;
    if (shift <= 11)
        inc = step * (1 << (11 - shift));
    else
        period = 1 << (shift - 11);
    if (++v->env_ctr < period)
        return;
    v->env_ctr = 0;
    v->env = ClampEnv(v->env + inc);
}

static void EnvStep(Voice *v) {
    switch (v->env_phase) {
    case ENV_ATTACK:
        Ramp(v, v->a_shift, 7 - v->a_step);
        if (v->env >= SPU_ENV_MAX) {
            v->env_phase = ENV_DECAY;
            v->env_ctr = 0;
        }
        break;
    case ENV_DECAY:
        Ramp(v, v->d_shift, -8);
        if (v->env <= v->s_level) {
            v->env_phase = ENV_SUSTAIN;
            v->env_ctr = 0;
        }
        break;
    case ENV_SUSTAIN:
        Ramp(v, v->s_shift, v->s_dec ? -8 : 7);
        break;
    case ENV_RELEASE:
        Ramp(v, v->r_shift, -8);
        if (v->env <= 0) {
            v->playing = 0;
            v->env_phase = ENV_OFF;
        }
        break;
    }
}

/* ADSR1/ADSR2 register bits (psx-spx layout). */
static void UnpackAdsr(Voice *v, unsigned adsr1, unsigned adsr2) {
    v->a_shift = (adsr1 >> 10) & 0x1f;
    v->a_step = (adsr1 >> 8) & 3;
    v->d_shift = (adsr1 >> 4) & 0xf;
    v->s_level = ((adsr1 & 0xf) + 1) * 0x800;
    v->s_dec = (adsr2 >> 14) & 1;
    v->s_shift = (adsr2 >> 6) & 0x7f;
    v->r_shift = adsr2 & 0x1f;
}

static void ApplyVoiceAttr(Voice *v, const PcSpuVoiceAttr *a) {
    unsigned m = (unsigned)a->mask;
    if (m & VA_VOLL)
        v->volL = a->volume.left;
    if (m & VA_VOLR)
        v->volR = a->volume.right;
    if (m & VA_PITCH)
        v->pitch = a->pitch & 0x3fff;
    if (m & VA_SAMPLE_NOTE)
        v->sample_note = a->sample_note;
    if (m & VA_NOTE) {
        v->note = a->note;
        v->pitch = NoteToPitch(v->note, 0, v->sample_note, 0);
    }
    if (m & VA_WDSA)
        v->start = a->addr & (PC_SPU_RAM_SIZE - 1);
    if (m & VA_LSAX)
        v->loop = a->loop_addr & (PC_SPU_RAM_SIZE - 1);
    if (m & VA_ADSR1)
        UnpackAdsr(v, a->adsr1, (m & VA_ADSR2) ? a->adsr2 : 0);
    if (m & VA_ADSR_AR)
        v->a_shift = a->ar & 0x1f;
    if (m & VA_ADSR_DR)
        v->d_shift = a->dr & 0xf;
    if (m & VA_ADSR_SR)
        v->s_shift = a->sr & 0x7f;
    if (m & VA_ADSR_RR)
        v->r_shift = a->rr & 0x1f;
    if (m & VA_ADSR_SL)
        v->s_level = ((a->sl & 0xf) + 1) * 0x800;
}

/* THPS_TRACE_SPU: once a second, the state of every voice that is still playing. */
static void TraceVoices(void) {
    for (int n = 0; n < PC_SPU_VOICES; n++) {
        const Voice *v = &s_v[n];
        if (!v->playing)
            continue;
        PC_Log("spu: voice %2d env %5d phase %d pitch %04x blk %05x idx %2d flags %02x "
               "loop %05x start %05x cur %6d prev %6d L%d R%d", n, v->env, v->env_phase,
               v->pitch, v->blk, v->idx, v->flags, v->loop, v->start, v->cur, v->prev, v->volL,
               v->volR);
    }
}

/* Next CD input frame at SPU_RATE (linear interpolation), through the CD mixer and volume. */
static void CdSample(int *outL, int *outR) {
    int l, r;
    *outL = *outR = 0;
    if (!s_cdPrimed) {
        if (s_cdCount < CD_PREBUFFER)
            return;
        s_cdPrimed = 1;
    }
    s_cdPhase += (s_cdRate << 16) / SPU_RATE;
    while (s_cdPhase >= 0x10000) {
        s_cdPhase -= 0x10000;
        s_cdPrev[0] = s_cdCur[0];
        s_cdPrev[1] = s_cdCur[1];
        if (!s_cdCount) {                   /* ran dry: silence until the prebuffer refills */
            s_cdPrimed = 0;
            s_cdCur[0] = s_cdCur[1] = 0;
            return;
        }
        s_cdCur[0] = s_cdBuf[s_cdRead * 2];
        s_cdCur[1] = s_cdBuf[s_cdRead * 2 + 1];
        s_cdRead = (s_cdRead + 1) % CD_FIFO_FRAMES;
        s_cdCount--;
        s_cdConsumed++;
    }
    if (!s_cdMix || s_cdMuted)
        return;
    int f = (int)(s_cdPhase >> 4);          /* 12-bit fraction keeps the products in range */
    int cl = s_cdPrev[0] + (((s_cdCur[0] - s_cdPrev[0]) * f) >> 12);
    int cr = s_cdPrev[1] + (((s_cdCur[1] - s_cdPrev[1]) * f) >> 12);
    l = Clamp16((cl * s_cdAtv[0] + cr * s_cdAtv[3]) >> 7);
    r = Clamp16((cl * s_cdAtv[1] + cr * s_cdAtv[2]) >> 7);
    *outL = (l * s_cdVol[0]) >> 15;
    *outR = (r * s_cdVol[1]) >> 15;
}

/* Mixer: runs on the audio thread. */
static void Mix(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    int16_t *out = (int16_t *)stream;
    int frames = len / 4;
    for (int f = 0; f < frames; f++) {
        int L = 0, R = 0;
        if (s_trace && ++s_traceFrames >= SPU_RATE) {
            s_traceFrames = 0;
            TraceVoices();
        }
        for (int n = 0; n < PC_SPU_VOICES; n++) {
            Voice *v = &s_v[n];
            if (!v->playing)
                continue;
            EnvStep(v);
            v->phase += v->pcm ? (v->pitch * v->pcm->mul) >> 12 : v->pitch;
            while (v->phase >= SPU_PITCH_ONE && v->playing) {
                int nx;
                v->phase -= SPU_PITCH_ONE;
                v->prev = v->cur;
                if (NextSample(v, &nx)) {
                    v->cur = nx;
                } else {
                    /* End block without repeat: the SPU silences the voice and zeroes its
                     * envelope. The game frees a voice only once it reads envelope 0. */
                    v->playing = 0;
                    v->env_phase = ENV_OFF;
                    v->env = 0;
                }
            }
            if (!v->playing)
                continue;
            int s = v->prev + (((v->cur - v->prev) * (int)v->phase) >> 12);
            int x = (s * v->env) >> 15;
            L += (x * v->volL) >> 15;
            R += (x * v->volR) >> 15;
        }
        int cdL, cdR;
        CdSample(&cdL, &cdR);
        out[2 * f] = (int16_t)Clamp16((int)(((int64_t)L * s_mvol[0]) >> 15) + cdL);
        out[2 * f + 1] = (int16_t)Clamp16((int)(((int64_t)R * s_mvol[1]) >> 15) + cdR);
    }
    if (s_dump)
        fwrite(stream, 1, (size_t)frames * 4, s_dump);
}

/* Device callback: the mixer, under the same lock the game-side calls take. */
static void DeviceCallback(void *userdata, Uint8 *stream, int len) {
    Lock();
    Mix(userdata, stream, len);
    Unlock();
}

/* No device (or THPS_AUDIO=0): run the mixer in real time on a thread anyway. The game reads
 * voice envelopes to decide which voices are free, so they must keep advancing even when nothing
 * is heard; the dump, if any, records the output. Blocks are paced on the performance counter. */
static int Pacer(void *userdata) {
    Uint8 buf[SPU_BUF_FRAMES * 4];
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 next = SDL_GetPerformanceCounter();
    (void)userdata;
    while (s_running) {
        Lock();
        Mix(NULL, buf, sizeof buf);
        Unlock();
        next += freq * SPU_BUF_FRAMES / SPU_RATE;
        for (Uint64 now = SDL_GetPerformanceCounter(); now < next && s_running;
             now = SDL_GetPerformanceCounter())
            SDL_Delay(1);
    }
    return 0;
}

static void StartPacer(void) {
    s_running = 1;
    if (!SDL_CreateThread(Pacer, "spu-pacer", NULL))
        PC_Log("audio: cannot start the mixer thread: %s", SDL_GetError());
}

void PC_SpuInit(void) {
    const char *audio = getenv("THPS_AUDIO");
    const char *dump = getenv("THPS_AUDIO_DUMP");
    SDL_AudioSpec want, have;

    s_trace = getenv("THPS_TRACE_SPU") != NULL;
    s_mx = SDL_CreateMutex();
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
    want.freq = SPU_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = SPU_BUF_FRAMES;
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

void PC_SpuReset(void) {
    Lock();
    memset(s_ram, 0, sizeof s_ram);
    memset(s_v, 0, sizeof s_v);
    s_xfer = 0;
    s_alloc = SPU_ALLOC_BASE;
    s_mvol[0] = s_mvol[1] = SPU_ENV_MAX;
    Unlock();
    PC_SpuCdFlush();
}

/* ---- libspu / libsnd entry points ------------------------------------------------------------- */

/* _SpuInit(mode): the game's own call at boot. */
void hle__SpuInit(int mode) {
    (void)mode;
    PC_SpuReset();
}

void SpuStart(void) {}

/* SpuInitMalloc(num, top): `top` is the malloc table in main RAM; the sound RAM area it manages
 * always starts at SPU_ALLOC_BASE. */
int SpuInitMalloc(int num, uint32_t top) {
    (void)top;
    Lock();
    s_alloc = SPU_ALLOC_BASE;
    Unlock();
    return num;
}

/* SpuMalloc(size): bump allocator over sound RAM, 16-byte aligned so ADPCM blocks line up. */
int SpuMalloc(int size) {
    unsigned need = ((unsigned)size + 15) & ~15u;
    int a = -1;
    Lock();
    if (s_alloc + need <= PC_SPU_RAM_SIZE) {
        a = (int)s_alloc;
        s_alloc += need;
    }
    Unlock();
    return a;
}

uint32_t SpuSetTransferStartAddr(uint32_t addr) {
    Lock();
    s_xfer = addr & (PC_SPU_RAM_SIZE - 1);
    Unlock();
    return addr;
}

/* The game's sample upload (VAB bodies, decompiled as FUN_80093df4 in
 * decomp/src/unnamed/8004b188.c). The signature table names this address SpuRead, but it is
 * SpuWrite: main RAM (src) into sound RAM at the transfer address. */
void hle_80093df4(const void *src, uint32_t size) {
    Lock();
    unsigned a = s_xfer;
    const uint8_t *p = src;
    while (size) {
        unsigned n = PC_SPU_RAM_SIZE - a;
        if (n > size)
            n = size;
        memcpy(&s_ram[a], p, n);
        p += n;
        size -= n;
        a = (a + n) & (PC_SPU_RAM_SIZE - 1);
    }
    s_xfer = a;
    Unlock();
}

int SpuIsTransferCompleted(int flag) {
    (void)flag;
    return 1;
}

void SpuNSetVoiceAttr(int voice, PcSpuVoiceAttr *attr) {
    if (voice < 0 || voice >= PC_SPU_VOICES || !attr)
        return;
    Lock();
    ApplyVoiceAttr(&s_v[voice], attr);
    Unlock();
}

void SpuGetVoiceAttr(PcSpuVoiceAttr *attr) {
    int n = 0;
    if (!attr)
        return;
    while (n < PC_SPU_VOICES && !(attr->voice & (1ul << n)))
        n++;
    if (n >= PC_SPU_VOICES)
        return;
    Lock();
    Voice *v = &s_v[n];
    attr->volumex.left = (short)v->volL;
    attr->volumex.right = (short)v->volR;
    attr->envx = (short)v->env;
    attr->pitch = (unsigned short)v->pitch;
    attr->addr = v->start;
    attr->loop_addr = v->loop;
    Unlock();
}

void SpuGetVoicePitch(int voice, uint16_t *pitch) {
    if (voice < 0 || voice >= PC_SPU_VOICES || !pitch)
        return;
    Lock();
    *pitch = (uint16_t)s_v[voice].pitch;
    Unlock();
}

void SpuGetVoiceEnvelope(int voice, int16_t *env) {
    if (voice < 0 || voice >= PC_SPU_VOICES || !env)
        return;
    Lock();
    *env = (int16_t)s_v[voice].env;
    Unlock();
}

void SpuSetCommonAttr(PcSpuCommonAttr *attr) {
    if (!attr)
        return;
    Lock();
    if (attr->mask & CA_MVOLL)
        s_mvol[0] = attr->mvol.left;
    if (attr->mask & CA_MVOLR)
        s_mvol[1] = attr->mvol.right;
    if (attr->mask & CA_CDVOLL)
        s_cdVol[0] = attr->cd.volume.left;
    if (attr->mask & CA_CDVOLR)
        s_cdVol[1] = attr->cd.volume.right;
    if (attr->mask & CA_CDMIX)
        s_cdMix = attr->cd.mix != 0;
    Unlock();
}

/* ---- CD audio input ---------------------------------------------------------------------------- */

void PC_SpuCdQueue(const int16_t *stereo, int frames, int rate) {
    Lock();
    s_cdRate = (unsigned)rate;
    for (int i = 0; i < frames; i++) {
        unsigned w;
        if (s_cdCount == CD_FIFO_FRAMES) {  /* overrun (game running fast): drop the oldest */
            s_cdRead = (s_cdRead + 1) % CD_FIFO_FRAMES;
            s_cdCount--;
            s_cdConsumed++;
        }
        w = (s_cdRead + s_cdCount) % CD_FIFO_FRAMES;
        s_cdBuf[w * 2] = stereo[i * 2];
        s_cdBuf[w * 2 + 1] = stereo[i * 2 + 1];
        s_cdCount++;
    }
    Unlock();
}

unsigned PC_SpuCdQueued(void) {
    unsigned n;
    Lock();
    n = s_cdCount;
    Unlock();
    return n;
}

uint64_t PC_SpuCdConsumed(void) {
    uint64_t n;
    Lock();
    n = s_cdConsumed;
    Unlock();
    return n;
}

void PC_SpuCdFlush(void) {
    Lock();
    s_cdConsumed += s_cdCount;   /* dropped frames count as gone by */
    s_cdRead = s_cdCount = 0;
    s_cdPhase = 0;
    s_cdPrimed = 0;
    s_cdPrev[0] = s_cdPrev[1] = s_cdCur[0] = s_cdCur[1] = 0;
    Unlock();
}

void PC_SpuCdSetMix(const uint8_t atv[4]) {
    Lock();
    for (int i = 0; i < 4; i++)
        s_cdAtv[i] = atv[i];
    Unlock();
}

void PC_SpuCdMute(int mute) {
    Lock();
    s_cdMuted = mute;
    Unlock();
}

/* SpuSetKey(on, voices): on = SPU_ON (1) starts the voices in the mask, SPU_OFF (0) releases
 * them. */
void SpuSetKey(int on, uint32_t voices) {
    Lock();
    for (int n = 0; n < PC_SPU_VOICES; n++) {
        if (!(voices & (1u << n)))
            continue;
        if (on & 1)
            KeyOn(&s_v[n], n);
        else
            KeyOff(&s_v[n], n);
    }
    Unlock();
}

int PC_SpuPcmAdd(const int16_t *pcm, uint32_t frames, int rate, int loop, uint32_t loopStart,
                 uint32_t loopEnd) {
    PcmSample *p;
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
    p->mul = (unsigned)(rate * PC_SPU_PCM_TUNE / SPU_RATE * 4096.0 + 0.5);
    p->loop = loop;
    p->loopStart = loopStart;
    p->loopEnd = loopEnd > frames ? frames : loopEnd;
    s_npcm++;
    Unlock();
    return id;
}

int PC_SpuPcmLooped(int id) { return id >= 0 && id < s_npcm && s_pcm[id].loop; }

/* Block 0 carries the marker and the id (as ADPCM, shift 12: next to silence), block 1 ends the
 * sample. */
void PC_SpuPcmStub(int id, uint8_t out[PC_SPU_PCM_STUB]) {
    memset(out, 0, PC_SPU_PCM_STUB);
    out[0] = 12;
    memcpy(out + 2, PCM_MAGIC, 4);
    memcpy(out + 6, &id, 4);
    out[16 + 1] = 1;
}

uint16_t SsPitchFromNote(int note, int fine, int cn, int cf) {
    return (uint16_t)NoteToPitch(note, fine, cn, cf);
}

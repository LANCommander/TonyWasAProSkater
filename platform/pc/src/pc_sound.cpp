/* Sound (see pc_sound.h): banks of PCM samples, 24 channels, a reverb and the music input, mixed
 * to 44.1 kHz stereo on an SDL audio device.
 *
 * Banks. A VAB's sample data is the PS1's ADPCM: 16-byte blocks of 28 samples, with flags for
 * the end of a sample, whether it repeats, and where its loop starts. Loading a bank decodes every
 * sample to PCM once; a sample that loops also gets its loop decoded on its own, as the decoder
 * continues into it from the sample's end. The Dreamcast's banks (pc_dcsound.cpp) are PCM already:
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

#include "pc/sound.h"
#include "pc_runtime.h"

namespace pc {

constinit Sound g_sound;

namespace {

constexpr int kBufFrames = 512;
constexpr unsigned kPitchOne = 0x1000u;

constexpr char kPcmMagic[] = "DCPC";

constexpr int kBankShift = 24;          /* a sample's id: (bank + 1) << kBankShift | offset */

constexpr int kStereoSpread = 23;
constexpr int k_comb[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
constexpr int k_allpass[4] = {556, 441, 341, 225};

struct ReverbModeDesc {
    float room, damp;       /* Freeverb room size and damping */
    float echo, feedback;   /* or a delay in seconds, and its feedback */
};

/* modes 1-9 (0 is off) */
constexpr ReverbModeDesc k_modes[10] = {
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

/* Delivery comes in bursts (one XA sector every 8 or 16 sectors read), so music playback waits for
 * a prebuffer after starting or running dry. */
constexpr unsigned kMusicPrebuffer = 8192u;

int Clamp16(int x) { return x < -32768 ? -32768 : x > 32767 ? 32767 : x; }

/* ---- ADPCM ------------------------------------------------------------------------------------- */

/* PS-ADPCM filter coefficients, in 1/64. */
constexpr int k_f0[5] = {0, 60, 115, 98, 122};
constexpr int k_f1[5] = {0, 0, -52, -55, -60};

void DecodeBlock(const uint8_t *b, int16_t *out, int *h1, int *h2) {
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

} // namespace

Sound::Pcm Sound::PcmTable::s_pcm[kPcmMax];
float Sound::Reverb::s_echo[2][kEchoMax];
int16_t Sound::Music::s_buf[kMusicFifo * 2];

/* ---- device lock ------------------------------------------------------------------------------- */

void Sound::Lock() {
    if (m_mx)
        SDL_LockMutex(m_mx);
}

void Sound::Unlock() {
    if (m_mx)
        SDL_UnlockMutex(m_mx);
}

/* ---- banks -------------------------------------------------------------------------------------- */

/* The sample starting at `offset` of a bank's data: its blocks up to the one flagged as the end
 * (bit 0; bit 1: it repeats from the last block flagged as the loop start, bit 2). */
int Sound::Bank::DecodeSample(uint32_t offset, Sample *out, const PcmTable &pcm) const {
    uint32_t end = offset, loop = offset, blocks, loopBlocks = 0, i;
    int repeat = 0, h1 = 0, h2 = 0;
    *out = Sample{};
    out->offset = offset;
    if (offset + 16 > m_size)
        return 0;
    if (!memcmp(m_data + offset + 2, kPcmMagic, 4)) {
        int id;
        memcpy(&id, m_data + offset + 6, 4);
        out->pcm = pcm.Get(id);
        out->pcmLoop = m_data[offset + 10];
        return 1;
    }
    for (;;) {
        uint8_t f = m_data[end + 1];
        if (f & 4)
            loop = end;
        end += 16;
        if ((f & 1) || end + 16 > m_size) {
            repeat = (f & 3) == 3;
            break;
        }
    }
    blocks = (end - offset) / 16;
    if (repeat)
        loopBlocks = (end - loop) / 16;
    out->frames = blocks * 28;
    out->loopFrames = loopBlocks * 28;
    out->data = (int16_t *)malloc(((size_t)out->frames + out->loopFrames) * sizeof *out->data);
    if (!out->data)
        return 0;
    for (i = 0; i < blocks; i++)
        DecodeBlock(m_data + offset + i * 16, out->data + i * 28, &h1, &h2);
    /* the loop as it sounds the second time round: the decoder carries on from the end */
    for (i = 0; i < loopBlocks; i++)
        DecodeBlock(m_data + loop + i * 16, out->data + out->frames + i * 28, &h1, &h2);
    return 1;
}

Sound::Sample *Sound::Bank::AddSample(uint32_t offset, const PcmTable &pcm) {
    Sample *s = (Sample *)malloc(sizeof *s);
    int i;
    if (!s)
        return nullptr;
    if (!DecodeSample(offset, s, pcm)) {
        free(s);
        return nullptr;
    }
    if (m_n == m_cap) {
        Sample **n = (Sample **)realloc(m_samples, (size_t)(m_cap ? m_cap * 2 : 64) * sizeof *n);
        if (!n) {
            free(s->data);
            free(s);
            return nullptr;
        }
        m_samples = n;
        m_cap = m_cap ? m_cap * 2 : 64;
    }
    for (i = m_n; i > 0 && m_samples[i - 1]->offset > offset; i--)
        m_samples[i] = m_samples[i - 1];
    m_samples[i] = s;
    m_n++;
    return s;
}

void Sound::Bank::Free() {
    for (int i = 0; i < m_n; i++) {
        free(m_samples[i]->data);
        free(m_samples[i]);
    }
    free(m_samples);
    free(m_data);
    *this = Bank{};
}

bool Sound::Bank::Load(const void *data, uint32_t size, const PcmTable &pcm) {
    uint32_t off = 0;
    m_data = (uint8_t *)malloc(size);
    if (!m_data)
        return false;
    memcpy(m_data, data, size);
    m_size = size;
    /* the samples one after another: each ends at a block flagged as the end */
    while (off + 16 <= size) {
        Sample *s = AddSample(off, pcm);
        if (!s)
            break;
        off += s->pcm ? kPcmStub : (s->frames / 28) * 16;
    }
    return true;
}

const Sound::Sample *Sound::Bank::Find(uint32_t off, const PcmTable &pcm) {
    int lo, hi;
    if (!m_data || off >= m_size)
        return nullptr;
    for (lo = 0, hi = m_n - 1; lo <= hi;) {
        int m = (lo + hi) / 2;
        if (m_samples[m]->offset == off)
            return m_samples[m];
        if (m_samples[m]->offset < off)
            lo = m + 1;
        else
            hi = m - 1;
    }
    return AddSample(off, pcm);
}

void Sound::BanksReset() {
    Lock();
    for (int c = 0; c < kChannels; c++)
        m_ch[c].Stop();
    for (int b = 0; b < kMaxBanks; b++)
        m_banks[b].Free();
    Unlock();
}

uint32_t Sound::BankLoad(const void *data, uint32_t size) {
    int b;
    for (b = 0; b < kMaxBanks && m_banks[b].Loaded(); b++)
        ;
    if (b == kMaxBanks || !size) {
        PC_Log("sound: no room for another bank");
        return 0;
    }
    if (!m_banks[b].Load(data, size, m_pcm))
        return 0;
    PC_Trace("sound: bank %d, %u bytes, %d samples", b, size, m_banks[b].Count());
    return (uint32_t)(b + 1) << kBankShift;
}

/* The sample at id (a bank's base + an offset): decoded now if it isn't one the bank's scan found. */
const Sound::Sample *Sound::FindSample(uint32_t id) {
    int b = (int)(id >> kBankShift) - 1;
    uint32_t off = id & ((1u << kBankShift) - 1);
    if (b < 0 || b >= kMaxBanks)
        return nullptr;
    return m_banks[b].Find(off, m_pcm);
}

/* ---- envelope ---------------------------------------------------------------------------------- */

void Sound::Ramp::Set(int rate, int decreasing, int exponential) {
    int shift = rate >> 2;
    m_rate = rate;
    m_decreasing = decreasing;
    m_exponential = exponential;
    m_step = decreasing ? -8 + (rate & 3) : 7 - (rate & 3);
    if (shift < 11)
        m_step *= 1 << (11 - shift);
    m_inc = shift > 11 ? (shift - 11 >= 16 ? 0 : 0x8000u >> (shift - 11)) : 0x8000u;
    m_counter = 0;
}

/* One frame of the ramp; the level it reaches. */
int Sound::Ramp::Tick(int level) {
    int step = m_step;
    unsigned inc = m_inc;
    if (m_exponential) {
        if (m_decreasing) {
            step = (step * level) >> 15;
        } else if (level >= 0x6000) {
            if (m_rate < 40) {
                step >>= 2;
            } else if (m_rate >= 44) {
                inc >>= 2;
            } else {
                step >>= 1;
                inc >>= 1;
            }
        }
    }
    m_counter += inc;
    if (!(m_counter & 0x8000))
        return level;
    m_counter = 0;
    level += step;
    return level < 0 ? 0 : level > kEnvMax ? kEnvMax : level;
}

void Sound::Voice::EnterPhase(Phase phase) {
    m_phase = phase;
    switch (phase) {
    case Phase::Attack:
        m_ramp.Set((m_adsr1 >> 8) & 0x7f, 0, m_adsr1 >> 15);
        break;
    case Phase::Decay:
        m_ramp.Set(((m_adsr1 >> 4) & 0xf) << 2, 1, 1);
        break;
    case Phase::Sustain:
        m_ramp.Set((m_adsr2 >> 6) & 0x7f, (m_adsr2 >> 14) & 1, m_adsr2 >> 15);
        break;
    case Phase::Release:
        m_ramp.Set((m_adsr2 & 0x1f) << 2, 1, (m_adsr2 >> 5) & 1);
        break;
    default:
        break;
    }
}

void Sound::Voice::EnvTick() {
    m_level = m_ramp.Tick(m_level);
    switch (m_phase) {
    case Phase::Attack:
        if (m_level >= kEnvMax)
            EnterPhase(Phase::Decay);
        break;
    case Phase::Decay:
        if (m_level <= m_sustainLevel)
            EnterPhase(Phase::Sustain);
        break;
    case Phase::Release:
        if (m_level <= 0) {
            m_phase = Phase::Off;
            m_s = nullptr;
        }
        break;
    default:
        break;
    }
}

/* ---- channels ---------------------------------------------------------------------------------- */

/* libsnd's SsPitchFromNote, integer for integer: a tone's fine tune (the VAB's `shift`) is ADDED
 * to the note's, then the semitone and the fine step come from tables of 0x8000 * 2^(i/12) and
 * 0x8000 * 2^(i/1536), rounded down. Two octaves or more above the centre is 0x3fff. */
int Sound::NotePitch(int note, int fine, int centre, int centreFine) {
    static uint16_t s_semitone[12], s_fine[128];
    if (!s_semitone[0]) {
        for (int i = 0; i < 128; i++)
            s_fine[i] = (uint16_t)floor(32768.0 * pow(2.0, i / 1536.0));
        for (int i = 0; i < 12; i++)
            s_semitone[i] = (uint16_t)floor(32768.0 * pow(2.0, i / 12.0));
    }
    int f = (int16_t)((centreFine & 0xff) + fine);
    int n = note + f / 128 - (centre & 0xff);
    f -= f / 128 * 128;
    if (f < 0) {
        f += 128;
        n--;
    }
    n = (int16_t)n;
    int octave = n / 12 - 2, semi = n % 12;
    if (semi < 0) {
        semi += 12;
        octave--;
    }
    if (octave >= 0)
        return 0x3fff;
    int p = (s_semitone[semi] * s_fine[f]) >> 16;
    return ((p + (1 << (-octave - 1))) >> -octave) & 0xffff;
}

void Sound::Voice::Set(const Sample *next, int pitch, int volL, int volR, uint16_t adsr1,
                       uint16_t adsr2) {
    m_next = next;
    m_pitch = (unsigned)pitch & 0x3fff;
    m_volL = volL;
    m_volR = volR;
    m_adsr1 = adsr1;
    m_adsr2 = adsr2;
}

void Sound::Voice::SetVolume(int volL, int volR) {
    m_volL = volL;
    m_volR = volR;
}

void Sound::Voice::SetPitch(int pitch) { m_pitch = (unsigned)pitch & 0x3fff; }

void Sound::Voice::Stop() {
    m_s = m_next = nullptr;
    m_phase = Phase::Off;
    m_level = 0;
}

/* Start from the beginning of the sample set for the next key on; n is the channel's number. */
void Sound::Voice::KeyOn(int n, bool trace) {
    m_s = m_next;
    m_pos = 0;
    m_frac = 0;
    m_prev = m_cur = 0;
    m_level = 0;
    m_sustainLevel = ((m_adsr1 & 0xf) + 1) * 0x800;
    EnterPhase(m_s ? Phase::Attack : Phase::Off);
    if (trace)
        PC_Log("sound: key on %d: sample %u pitch %04x vol %d/%d adsr %04x %04x", n,
               m_s ? m_s->offset : 0, m_pitch, m_volL, m_volR, m_adsr1, m_adsr2);
}

/* Release (the envelope fades), unless already silent or releasing. */
void Sound::Voice::KeyOff(int n, bool trace) {
    if (m_phase != Phase::Off && m_phase != Phase::Release) {
        EnterPhase(Phase::Release);
        if (trace)
            PC_Log("sound: key off %d (level %d)", n, m_level);
    }
}

void Sound::ChannelSet(int ch, uint32_t sample, int pitch, int volL, int volR, uint16_t adsr1,
                       uint16_t adsr2) {
    if (!Ok(ch))
        return;
    Lock();
    m_ch[ch].Set(FindSample(sample), pitch, volL, volR, adsr1, adsr2);
    Unlock();
    if (!m_ch[ch].HasNext())
        PC_Trace("sound: channel %d: no sample at %08x", ch, sample);
}

void Sound::ChannelVolume(int ch, int volL, int volR) {
    if (!Ok(ch))
        return;
    Lock();
    m_ch[ch].SetVolume(volL, volR);
    Unlock();
}

void Sound::ChannelPitch(int ch, int pitch) {
    if (!Ok(ch))
        return;
    Lock();
    m_ch[ch].SetPitch(pitch);
    Unlock();
}

int Sound::ChannelGetPitch(int ch) const { return Ok(ch) ? m_ch[ch].Pitch() : 0; }

int Sound::ChannelLevel(int ch) {
    int l;
    if (!Ok(ch))
        return 0;
    Lock();
    l = m_ch[ch].Level();
    Unlock();
    return l;
}

void Sound::KeyOn(uint32_t channels) {
    Lock();
    for (int n = 0; n < kChannels; n++) {
        if (!(channels >> n & 1))
            continue;
        m_ch[n].KeyOn(n, m_trace);
    }
    Unlock();
}

void Sound::KeyOff(uint32_t channels) {
    Lock();
    for (int n = 0; n < kChannels; n++)
        if (channels >> n & 1)
            m_ch[n].KeyOff(n, m_trace);
    Unlock();
}

/* The channel's next source frame, or 0 at the end of a sample that doesn't repeat. */
int Sound::Voice::NextFrame(int *out) {
    const Sample *s = m_s;
    if (s->pcm) {
        const Pcm *p = s->pcm;
        if (m_pos >= p->frames) {
            if (!s->pcmLoop || !p->frames)
                return 0;
            m_pos = 0;
        }
        *out = p->data[m_pos++];
        return 1;
    }
    if (m_pos >= s->frames + s->loopFrames) {
        if (!s->loopFrames)
            return 0;
        m_pos = s->frames;
    }
    *out = s->data[m_pos++];
    return 1;
}

bool Sound::Voice::Render(int &l, int &r) {
    if (!m_s)
        return false;
    EnvTick();
    if (!m_s)
        return false;
    m_frac += m_s->pcm ? (m_pitch * m_s->pcm->mul) >> 12 : m_pitch;
    while (m_frac >= kPitchOne) {
        int nx;
        m_frac -= kPitchOne;
        m_prev = m_cur;
        if (!NextFrame(&nx)) {
            /* the end of a sample that doesn't repeat: silent, level 0 */
            m_s = nullptr;
            m_phase = Phase::Off;
            m_level = 0;
            break;
        }
        m_cur = nx;
    }
    if (!m_s)
        return false;
    int s = m_prev + (((m_cur - m_prev) * (int)m_frac) >> 12);
    int x = (s * m_level) >> 15;
    if (m_s->pcm) {
        /* the Dreamcast's volume law: its driver turns a volume v (0..16383) into the AICA's
           attenuation as -20 log10((v / 16383)^2) dB (1ST_READ.BIN 0x8c0983c2 and the other
           volume setters), so the gain is the square; its table's volumes are set for that */
        l = (int)(((int64_t)x * m_volL * (m_volL < 0 ? -m_volL : m_volL)) >> 28);
        r = (int)(((int64_t)x * m_volR * (m_volR < 0 ? -m_volR : m_volR)) >> 28);
        return true;
    }
    l = (x * m_volL) >> 14;
    r = (x * m_volR) >> 14;
    return true;
}

/* ---- the mix ------------------------------------------------------------------------------------- */

void Sound::Levels(int sfxL, int sfxR, int musicL, int musicR) {
    Lock();
    m_sfx[0] = sfxL;
    m_sfx[1] = sfxR;
    m_music.SetLevels(musicL, musicR);
    Unlock();
}

void Sound::Filter::Init(int length) {
    free(buf);
    buf = (float *)calloc((size_t)length, sizeof *buf);
    len = length;
    idx = 0;
    store = 0;
}

void Sound::Reverb::SetMode(int mode) {
    if (mode != m_mode) {
        for (int ch = 0; ch < 2; ch++) {
            for (int i = 0; i < kCombs; i++)
                m_comb[ch][i].Init(k_comb[i] + ch * kStereoSpread);
            for (int i = 0; i < kAllpasses; i++)
                m_allpass[ch][i].Init(k_allpass[i] + ch * kStereoSpread);
        }
        memset(s_echo, 0, sizeof s_echo);
        m_echoIdx = 0;
        m_echoLen = (int)(k_modes[mode].echo * kRate);
        if (m_echoLen > kEchoMax)
            m_echoLen = kEchoMax;
        PC_Trace("sound: reverb mode %d", mode);
    }
    m_mode = mode;
    m_depth[0] = m_depth[1] = 0; /* as the PS1 does: the depth is set again after */
}

void Sound::Reverb::SetDepth(int left, int right) {
    m_depth[0] = (float)left / kEnvMax;
    m_depth[1] = (float)right / kEnvMax;
}

void Sound::Reverb::SetChannels(int on, uint32_t channels) {
    m_channels = on ? m_channels | channels : m_channels & ~channels;
}

int Sound::ReverbMode(int mode) {
    if (mode < 0 || mode > 9)
        return 0;
    Lock();
    m_rev.SetMode(mode);
    Unlock();
    return 1;
}

void Sound::ReverbDepth(int left, int right) {
    Lock();
    m_rev.SetDepth(left, right);
    Unlock();
}

void Sound::ReverbChannels(int on, uint32_t channels) {
    Lock();
    m_rev.SetChannels(on, channels);
    Unlock();
}

/* One frame of the reverb's wet output for its input (both in 16-bit sample units). */
void Sound::Reverb::Frame(float inL, float inR, float *outL, float *outR) {
    const ReverbModeDesc *m = &k_modes[m_mode];
    float in[2] = {inL, inR};
    for (int ch = 0; ch < 2; ch++) {
        float out = 0;
        if (m_echoLen) {
            float *e = &s_echo[ch][m_echoIdx];
            out = *e;
            *e = in[ch] + out * m->feedback;
        } else {
            float x = in[ch] * 0.015f;
            for (int i = 0; i < kCombs; i++) {
                Filter *f = &m_comb[ch][i];
                float y = f->buf[f->idx];
                f->store = y * (1 - m->damp) + f->store * m->damp;
                f->buf[f->idx] = x + f->store * (0.7f + 0.28f * m->room);
                f->idx = (f->idx + 1) % f->len;
                out += y;
            }
            for (int i = 0; i < kAllpasses; i++) {
                Filter *f = &m_allpass[ch][i];
                float y = f->buf[f->idx];
                f->buf[f->idx] = out + y * 0.5f;
                f->idx = (f->idx + 1) % f->len;
                out = y - out;
            }
            out *= 3.0f;
        }
        *(ch ? outR : outL) = out * m_depth[ch];
    }
    if (m_echoLen)
        m_echoIdx = (m_echoIdx + 1) % m_echoLen;
}

/* The next music frame at kRate, through its mix and level. */
void Sound::Music::Frame(int *outL, int *outR) {
    *outL = *outR = 0;
    if (!m_primed) {
        if (m_count < kMusicPrebuffer)
            return;
        m_primed = 1;
    }
    m_phase += (m_rate << 16) / kRate;
    while (m_phase >= 0x10000) {
        m_phase -= 0x10000;
        m_prev[0] = m_cur[0];
        m_prev[1] = m_cur[1];
        if (!m_count) { /* ran dry: silence until the prebuffer refills */
            m_primed = 0;
            m_cur[0] = m_cur[1] = 0;
            return;
        }
        m_cur[0] = s_buf[m_read * 2];
        m_cur[1] = s_buf[m_read * 2 + 1];
        m_read = (m_read + 1) % kMusicFifo;
        m_count--;
        m_consumed++;
    }
    if (m_muted)
        return;
    int f = (int)(m_phase >> 4); /* a 12-bit fraction keeps the products in range */
    int cl = m_prev[0] + (((m_cur[0] - m_prev[0]) * f) >> 12);
    int cr = m_prev[1] + (((m_cur[1] - m_prev[1]) * f) >> 12);
    int l = Clamp16((cl * m_mix[0] + cr * m_mix[3]) >> 7);
    int r = Clamp16((cl * m_mix[1] + cr * m_mix[2]) >> 7);
    *outL = (l * m_level[0]) >> 14;
    *outR = (r * m_level[1]) >> 14;
}

void Sound::Mix(uint8_t *stream, int len) {
    int16_t *out = (int16_t *)stream;
    int frames = len / 4;
    for (int f = 0; f < frames; f++) {
        int L = 0, R = 0, revL = 0, revR = 0;
        for (int n = 0; n < kChannels; n++) {
            int l, r;
            if (!m_ch[n].Render(l, r))
                continue;
            L += l;
            R += r;
            if (m_rev.Sends(n)) {
                revL += l;
                revR += r;
            }
        }
        if (m_rev.Active()) {
            float wl, wr;
            m_rev.Frame((float)revL, (float)revR, &wl, &wr);
            L += (int)wl;
            R += (int)wr;
        }
        int ml, mr;
        m_music.Frame(&ml, &mr);
        out[2 * f] = (int16_t)Clamp16((int)(((int64_t)L * m_sfx[0]) >> 14) + ml);
        out[2 * f + 1] = (int16_t)Clamp16((int)(((int64_t)R * m_sfx[1]) >> 14) + mr);
    }
    if (m_dump)
        fwrite(stream, 1, (size_t)frames * 4, m_dump);
}

void Sound::DeviceCallback(void *userdata, uint8_t *stream, int len) {
    Sound *self = (Sound *)userdata;
    self->Lock();
    self->Mix(stream, len);
    self->Unlock();
}

/* No device (or THPS_AUDIO=0): mix in real time on a thread anyway, paced on the performance
 * counter, so the envelopes the game reads keep moving; the dump, if any, records it. */
int Sound::Pacer(void *userdata) {
    Sound *self = (Sound *)userdata;
    uint8_t buf[kBufFrames * 4];
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 next = SDL_GetPerformanceCounter();
    while (self->m_running) {
        self->Lock();
        self->Mix(buf, sizeof buf);
        self->Unlock();
        next += freq * kBufFrames / kRate;
        for (Uint64 now = SDL_GetPerformanceCounter(); now < next && self->m_running;
             now = SDL_GetPerformanceCounter())
            SDL_Delay(1);
    }
    return 0;
}

void Sound::StartPacer() {
    m_running = 1;
    if (!SDL_CreateThread(Pacer, "sound-pacer", this))
        PC_Log("audio: cannot start the mixer thread: %s", SDL_GetError());
}

void Sound::Init() {
    const char *audio = getenv("THPS_AUDIO");
    const char *dump = getenv("THPS_AUDIO_DUMP");
    SDL_AudioSpec want, have;

    m_trace = getenv("THPS_TRACE_SOUND") && atoi(getenv("THPS_TRACE_SOUND"));
    m_mx = SDL_CreateMutex();
    ReverbMode(0);
    if (dump && !(m_dump = fopen(dump, "wb")))
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
    want.freq = kRate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = kBufFrames;
    want.callback = DeviceCallback;
    want.userdata = this;
    m_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!m_dev) {
        PC_Log("audio: cannot open an audio device: %s", SDL_GetError());
        StartPacer();
        return;
    }
    SDL_PauseAudioDevice(m_dev, 0);
    PC_Log("audio: %d Hz, %d channels, %d-frame buffers", have.freq, have.channels, have.samples);
}

/* ---- the music --------------------------------------------------------------------------------- */

void Sound::Music::Queue(const int16_t *stereo, int frames, int rate) {
    m_rate = (unsigned)rate;
    for (int i = 0; i < frames; i++) {
        unsigned w;
        if (m_count == kMusicFifo) { /* overrun (the game running fast): drop the oldest */
            m_read = (m_read + 1) % kMusicFifo;
            m_count--;
            m_consumed++;
        }
        w = (m_read + m_count) % kMusicFifo;
        s_buf[w * 2] = stereo[i * 2];
        s_buf[w * 2 + 1] = stereo[i * 2 + 1];
        m_count++;
    }
}

void Sound::Music::Flush() {
    m_consumed += m_count; /* dropped frames count as gone by */
    m_read = m_count = 0;
    m_phase = 0;
    m_primed = 0;
    m_prev[0] = m_prev[1] = m_cur[0] = m_cur[1] = 0;
}

void Sound::Music::SetMix(const uint8_t levels[4]) {
    for (int i = 0; i < 4; i++)
        m_mix[i] = levels[i];
}

void Sound::Music::SetLevels(int left, int right) {
    m_level[0] = left;
    m_level[1] = right;
}

void Sound::MusicQueue(const int16_t *stereo, int frames, int rate) {
    Lock();
    m_music.Queue(stereo, frames, rate);
    Unlock();
}

unsigned Sound::MusicQueued() {
    unsigned n;
    Lock();
    n = m_music.Queued();
    Unlock();
    return n;
}

uint64_t Sound::MusicConsumed() {
    uint64_t n;
    Lock();
    n = m_music.Consumed();
    Unlock();
    return n;
}

void Sound::MusicFlush() {
    Lock();
    m_music.Flush();
    Unlock();
}

void Sound::MusicMix(const uint8_t levels[4]) {
    Lock();
    m_music.SetMix(levels);
    Unlock();
}

void Sound::MusicMute(int mute) {
    Lock();
    m_music.SetMute(mute);
    Unlock();
}

/* ---- PCM samples ----------------------------------------------------------------------------- */

int Sound::PcmTable::Add(const int16_t *pcm, uint32_t frames, int rate) {
    if (m_n == kPcmMax)
        return -1;
    int id = m_n;
    Pcm *p = &s_pcm[id];
    p->data = (int16_t *)malloc((frames ? frames : 1) * sizeof *p->data);
    memcpy(p->data, pcm, frames * sizeof *p->data);
    p->frames = frames;
    p->mul = (unsigned)((double)rate / kRate * kPitchOne / kPcmUnity * kPitchOne + 0.5);
    m_n++;
    return id;
}

int Sound::PcmAdd(const int16_t *pcm, uint32_t frames, int rate) {
    Lock();
    int id = m_pcm.Add(pcm, frames, rate);
    Unlock();
    return id;
}

/* Block 0 carries the marker, the id and the loop flag, block 1 ends the stand-in (it also reads
 * as near-silent ADPCM). */
void Sound::PcmStub(int id, int loop, uint8_t out[kPcmStub]) {
    memset(out, 0, kPcmStub);
    out[0] = 12;
    memcpy(out + 2, kPcmMagic, 4);
    memcpy(out + 6, &id, 4);
    out[10] = loop != 0;
    out[16 + 1] = 1;
}

} // namespace pc

/* ---- the game's interface (pc_sound.h) ---- */

extern "C" {

void PC_SoundBanksReset(void) { pc::g_sound.BanksReset(); }
uint32_t PC_SoundBankLoad(const void *data, uint32_t size) { return pc::g_sound.BankLoad(data, size); }
void PC_SoundChannelSet(int ch, uint32_t sample, int pitch, int volL, int volR, uint16_t adsr1,
                        uint16_t adsr2) {
    pc::g_sound.ChannelSet(ch, sample, pitch, volL, volR, adsr1, adsr2);
}
void PC_SoundChannelVolume(int ch, int volL, int volR) { pc::g_sound.ChannelVolume(ch, volL, volR); }
void PC_SoundChannelPitch(int ch, int pitch) { pc::g_sound.ChannelPitch(ch, pitch); }
int PC_SoundChannelGetPitch(int ch) { return pc::g_sound.ChannelGetPitch(ch); }
int PC_SoundChannelLevel(int ch) { return pc::g_sound.ChannelLevel(ch); }
void PC_SoundKeyOn(uint32_t channels) { pc::g_sound.KeyOn(channels); }
void PC_SoundKeyOff(uint32_t channels) { pc::g_sound.KeyOff(channels); }
int PC_SoundNotePitch(int note, int fine, int centre, int centreFine) {
    return pc::Sound::NotePitch(note, fine, centre, centreFine);
}
void PC_SoundLevels(int sfxL, int sfxR, int musicL, int musicR) {
    pc::g_sound.Levels(sfxL, sfxR, musicL, musicR);
}
int PC_SoundReverbMode(int mode) { return pc::g_sound.ReverbMode(mode); }
void PC_SoundReverbDepth(int left, int right) { pc::g_sound.ReverbDepth(left, right); }
void PC_SoundReverbChannels(int on, uint32_t channels) { pc::g_sound.ReverbChannels(on, channels); }

} // extern "C"

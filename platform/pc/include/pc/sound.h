/* The SPU's replacement as a class (pc_sound.cpp; the game's C interface is pc_sound.h): sound
 * banks decoded to PCM, 24 voices with the PS1's ADSR, a native reverb and the music input,
 * mixed to 44.1 kHz stereo on SDL's audio thread. Every method that touches the mixer's state
 * takes the lock the mixer runs under, as noted in pc_sound.cpp. */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include "pc_sound.h"

struct SDL_mutex;

namespace pc {

class Sound {
public:
    static constexpr int kChannels = 24;
    static constexpr int kRate = 44100;
    static constexpr int kFull = 0x4000;      /* volumes and levels: 1.0 */
    static constexpr int kEnvMax = 0x7fff;
    /* The stand-in a registered PCM sample (PcmAdd) leaves in a bank's data. */
    static constexpr int kPcmStub = 32;

    /* Open the audio device and start the mixer. THPS_AUDIO=0 mixes but plays nothing;
     * THPS_AUDIO_DUMP=path also writes the mixed output as raw 44.1 kHz s16le stereo. */
    void Init();

    /* ---- banks ---- */

    /* Drop every bank (the game is about to load its banks again); channels stop. */
    void BanksReset();
    /* A VAB's sample data (the PS1's ADPCM, decoded here). Returns the bank's base: a sample is
     * base + its byte offset in the data. 0 if it can't be loaded. */
    uint32_t BankLoad(const void *data, uint32_t size);

    /* ---- channels ---- */

    /* What channel ch plays at its next key on: the sample at `sample` (a bank base + offset). */
    void ChannelSet(int ch, uint32_t sample, int pitch, int volL, int volR, uint16_t adsr1,
                    uint16_t adsr2);
    void ChannelVolume(int ch, int volL, int volR);
    void ChannelPitch(int ch, int pitch);
    int ChannelGetPitch(int ch) const;
    /* The envelope's level, 0..0x7fff: 0 once the sound has ended or been released to silence. */
    int ChannelLevel(int ch);
    /* Start the channels in the mask from the beginning, or release them (their envelopes fade). */
    void KeyOn(uint32_t channels);
    void KeyOff(uint32_t channels);
    /* The pitch for a note (60 = middle C, fine in 1/128) of a sample centred on centre/centreFine. */
    static int NotePitch(int note, int fine, int centre, int centreFine);

    /* ---- mix ---- */

    /* The sound effects' and the music's levels (0x4000 = full). */
    void Levels(int sfxL, int sfxR, int musicL, int musicR);
    /* Reverb: the PS1's modes (0 off, 1 room, 2-4 studio small/medium/large, 5 hall, 6 space
     * echo, 7 echo, 8 delay, 9 pipe), its depth (0x7fff full; a new mode starts at 0), and which
     * channels are sent to it. 0 if the mode is unknown. */
    int ReverbMode(int mode);
    void ReverbDepth(int left, int right);
    void ReverbChannels(int on, uint32_t channels);

    /* ---- the music (pc_stream.cpp) ---- */

    /* Queue stereo PCM at `rate` Hz; the mixer resamples it. Flush drops what is queued. */
    void MusicQueue(const int16_t *stereo, int frames, int rate);
    void MusicFlush();
    /* Frames queued and not yet played; frames played so far (a clock for what has been heard). */
    unsigned MusicQueued();
    uint64_t MusicConsumed();
    /* Levels: left to left, left to right, right to right, right to left; 0x80 = 100%. */
    void MusicMix(const uint8_t levels[4]);
    void MusicMute(int mute);

    /* ---- the Dreamcast's sound banks (pc_dcsound.cpp) ---- */

    /* A PCM sample. As on the Dreamcast, a channel's pitch is relative to kPcmUnity: at 0x400
     * it plays at `rate`, at 0x800 an octave up. Returns its id, or -1 if full. Its stand-ins
     * (PcmStub, kPcmStub bytes) go in a bank's data where the sample would be: one that plays it
     * once, or one that repeats the whole sample (the Dreamcast's loops). */
    static constexpr int kPcmUnity = 0x400;
    int PcmAdd(const int16_t *pcm, uint32_t frames, int rate);
    static void PcmStub(int id, int loop, uint8_t out[kPcmStub]);

private:
    static constexpr int kMaxBanks = 8;
    static constexpr int kPcmMax = 1024;
    static constexpr int kCombs = 8;
    static constexpr int kAllpasses = 4;
    static constexpr int kEchoMax = kRate / 2;
    static constexpr unsigned kMusicFifo = 131072u;

    /* A registered PCM sample (the Dreamcast's banks). */
    struct Pcm {
        int16_t *data = nullptr;
        uint32_t frames = 0;
        unsigned mul = 0;           /* step per output frame at pitch 0x1000, in 1/0x1000 */
    };

    /* The registered PCM samples, by id. */
    class PcmTable {
    public:
        int Add(const int16_t *pcm, uint32_t frames, int rate);
        const Pcm *Get(int id) const { return id >= 0 && id < m_n ? &s_pcm[id] : nullptr; }

    private:
        static Pcm s_pcm[kPcmMax];  /* one table: static storage, not in the object */
        int m_n = 0;
    };

    /* One sample of a bank, decoded. */
    struct Sample {
        uint32_t offset = 0;        /* in the bank's data */
        int16_t *data = nullptr;    /* the sample, then (if it loops) its loop again */
        uint32_t frames = 0;        /* the sample's frames */
        uint32_t loopFrames = 0;    /* frames of the loop after them (0: plays once) */
        const Pcm *pcm = nullptr;   /* or a Dreamcast stand-in's PCM */
        int pcmLoop = 0;            /* and whether the stand-in repeats it */
    };

    /* A loaded VAB's sample data and the samples found in it. */
    class Bank {
    public:
        bool Loaded() const { return m_data != nullptr; }
        /* Copy the data and decode the samples one after another; false if out of memory. */
        bool Load(const void *data, uint32_t size, const PcmTable &pcm);
        void Free();
        /* The sample at `off`: decoded now if it isn't one the bank's scan found. */
        const Sample *Find(uint32_t off, const PcmTable &pcm);
        int Count() const { return m_n; }

    private:
        int DecodeSample(uint32_t offset, Sample *out, const PcmTable &pcm) const;
        Sample *AddSample(uint32_t offset, const PcmTable &pcm);

        uint8_t *m_data = nullptr;
        uint32_t m_size = 0;
        Sample **m_samples = nullptr; /* sorted by offset; each its own allocation (voices point at them) */
        int m_n = 0, m_cap = 0;
    };

    /* An envelope phase's rate. */
    class Ramp {
    public:
        void Set(int rate, int decreasing, int exponential);
        /* One frame of the ramp; the level it reaches. */
        int Tick(int level);

    private:
        int m_rate = 0, m_decreasing = 0, m_exponential = 0;
        int m_step = 0;             /* per counter overflow, scaled for shifts under 11 */
        unsigned m_inc = 0;         /* counter increment per frame (0x8000 = a step every frame) */
        unsigned m_counter = 0;
    };

    enum class Phase { Off, Attack, Decay, Sustain, Release };

    /* One of the 24 channels. */
    class Voice {
    public:
        void Set(const Sample *next, int pitch, int volL, int volR, uint16_t adsr1, uint16_t adsr2);
        void SetVolume(int volL, int volR);
        void SetPitch(int pitch);
        int Pitch() const { return (int)m_pitch; }
        int Level() const { return m_level; }
        bool HasNext() const { return m_next != nullptr; }
        bool Playing() const { return m_s != nullptr; }
        /* Stop at once (the banks are going). */
        void Stop();
        void KeyOn(int n, bool trace);
        void KeyOff(int n, bool trace);
        /* One output frame: false if the voice is silent, else its frame after the envelope and
         * volume in l and r. */
        bool Render(int &l, int &r);

    private:
        void EnterPhase(Phase phase);
        void EnvTick();
        int NextFrame(int *out);

        const Sample *m_next = nullptr; /* what the next key on plays */
        uint16_t m_adsr1 = 0, m_adsr2 = 0;
        int m_volL = 0, m_volR = 0;     /* kFull = 1.0 */
        unsigned m_pitch = 0;
        /* playing */
        const Sample *m_s = nullptr;
        Phase m_phase = Phase::Off;
        int m_level = 0;                /* 0..kEnvMax */
        int m_sustainLevel = 0;
        Ramp m_ramp;
        uint32_t m_pos = 0;             /* frame of m_s->data (the loop's frames follow the sample's) */
        unsigned m_frac = 0;            /* 0..0x1000 between prev and cur */
        int m_prev = 0, m_cur = 0;
    };

    /* A comb or allpass filter's delay line. */
    struct Filter {
        float *buf = nullptr;
        int len = 0, idx = 0;
        float store = 0;
        void Init(int length);
    };

    /* The native reverb. */
    class Reverb {
    public:
        void SetMode(int mode);
        void SetDepth(int left, int right);
        void SetChannels(int on, uint32_t channels);
        bool Sends(int n) const { return m_channels >> n & 1; }
        bool Active() const { return m_mode && (m_depth[0] > 0 || m_depth[1] > 0); }
        /* One frame of the reverb's wet output for its input (both in 16-bit sample units). */
        void Frame(float inL, float inR, float *outL, float *outR);

    private:
        static float s_echo[2][kEchoMax]; /* one reverb: static storage, not in the object */
        int m_mode = 0;
        float m_depth[2] = {};
        uint32_t m_channels = 0;
        Filter m_comb[2][kCombs], m_allpass[2][kAllpasses];
        int m_echoIdx = 0, m_echoLen = 0;
    };

    /* The music input: a FIFO of stereo frames at the stream's rate, resampled to kRate. */
    class Music {
    public:
        void Queue(const int16_t *stereo, int frames, int rate);
        void Flush();
        unsigned Queued() const { return m_count; }
        uint64_t Consumed() const { return m_consumed; }
        void SetMix(const uint8_t levels[4]);
        void SetMute(int mute) { m_muted = mute; }
        void SetLevels(int left, int right);
        /* The next music frame at kRate, through its mix and level. */
        void Frame(int *outL, int *outR);

    private:
        static int16_t s_buf[kMusicFifo * 2]; /* one FIFO: static storage, not in the object */
        unsigned m_read = 0, m_count = 0;
        uint64_t m_consumed = 0;
        unsigned m_rate = 37800;
        unsigned m_phase = 0;           /* 16.16 between prev and cur */
        int m_primed = 0;
        int m_prev[2] = {}, m_cur[2] = {};
        int m_mix[4] = {0x80, 0x00, 0x80, 0x00}; /* L->L, L->R, R->R, R->L */
        int m_level[2] = {kFull, kFull};
        int m_muted = 0;
    };

    static bool Ok(int ch) { return ch >= 0 && ch < kChannels; }
    void Lock();
    void Unlock();
    const Sample *FindSample(uint32_t id);
    void Mix(uint8_t *stream, int len);
    void StartPacer();
    static void DeviceCallback(void *userdata, uint8_t *stream, int len);
    static int Pacer(void *userdata);

    PcmTable m_pcm;
    Bank m_banks[kMaxBanks];
    Voice m_ch[kChannels];
    int m_sfx[2] = {kFull, kFull};
    Reverb m_rev;
    Music m_music;
    /* the device */
    uint32_t m_dev = 0;                 /* SDL_AudioDeviceID */
    SDL_mutex *m_mx = nullptr;
    int m_running = 0;
    FILE *m_dump = nullptr;
    int m_trace = 0;
};

extern Sound g_sound;

} // namespace pc

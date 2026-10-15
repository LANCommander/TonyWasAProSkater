/* Dreamcast sound banks (see pc_dcsound.h).
 *
 * A .kat (little endian) is u32 count, then count 44-byte entries {u32 type (1), offset, size,
 * rate, loop, bits (8 = signed 8-bit, 16 = s16), id, loop start, loop end (bytes; 0 or the size:
 * the whole sample), 0, 0}, then the sample data. id = program * 1000 + tone of the PS1 bank of
 * the same name (WARE.KAT for ware.vab): the same sounds, at 22050 Hz (some 11025 or 8000), most
 * of them 8-bit. A few PS1 tones have no entry (silent), and a few entries no PS1 tone.
 *
 * The VAB made from it has the PS1 layout the game reads (FUN_8004b188, playSFX): header,
 * 128 program entries, 16 tone entries of 32 bytes per program, 256 sample sizes, the samples.
 * Each entry's tone plays its PCM stand-in (pc_sound.h); the tone's centre note is 60, so the
 * game's note 60 plays the sample at its own (tuned) rate, and its ADSR is the one the PS1 banks
 * use for that kind of sound: looped ones fade in and out, one-shots start and stop at once. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_assets.h"
#include "pc_dcsound.h"
#include "pc_gamedata.h"
#include "pc_platform.h"
#include "pc_runtime.h"
#include "pc_sound.h"

#define KAT_ENTRY 44
#define VAB_PROGS_MAX 128
#define VAB_TONES 16
#define VAB_HEAD(progs) (0x820 + (progs) * 512 + 512)
#define ADSR1_ONESHOT 0x80ff
#define ADSR2_ONESHOT 0x1fc0
#define ADSR1_LOOP 0xa9ff
#define ADSR2_LOOP 0x5fcc

typedef struct {
    char name[64];       /* the .kat's file name */
    int tone[VAB_PROGS_MAX * VAB_TONES];   /* PCM sample id + 1 (0: none) */
} Bank;

#define MAX_BANKS 32
static Bank s_banks[MAX_BANKS];
static int s_nbanks;

static uint32_t Le32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* Read a .kat and register its samples (once per bank). NULL if it can't be read. */
static Bank *LoadBank(const char *kat) {
    char path[260];
    Bank *b;
    uint8_t *d;
    long size;
    uint32_t n, i;
    FILE *f;
    int k;
    for (k = 0; k < s_nbanks; k++)
        if (!_stricmp(s_banks[k].name, kat))
            return &s_banks[k];
    if (s_nbanks == MAX_BANKS || !PC_AssetsFind(kat, path, sizeof path) || !(f = fopen(path, "rb")))
        return NULL;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    d = malloc((size_t)size + 1);
    if (size < 4 || fread(d, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        free(d);
        return NULL;
    }
    fclose(f);
    n = Le32(d);
    if (n > (uint32_t)(size - 4) / KAT_ENTRY) {
        PC_Log("%s is damaged", path);
        free(d);
        return NULL;
    }
    b = &s_banks[s_nbanks++];
    memset(b, 0, sizeof *b);
    snprintf(b->name, sizeof b->name, "%s", kat);
    for (i = 0; i < n; i++) {
        const uint8_t *e = d + 4 + i * KAT_ENTRY;
        uint32_t off = Le32(e + 4), len = Le32(e + 8), rate = Le32(e + 12), loop = Le32(e + 16),
                 bits = Le32(e + 20), id = Le32(e + 24), ls = Le32(e + 28), le = Le32(e + 32);
        uint32_t prog = id / 1000, tone = id % 1000, bps = bits / 8, frames, j;
        int16_t *pcm;
        int pid;
        if (Le32(e) != 1 || (bits != 8 && bits != 16) || off > (uint32_t)size ||
            len > (uint32_t)size - off || !rate)
            continue;
        if (prog >= VAB_PROGS_MAX || tone >= VAB_TONES) {
            PC_Trace("%s: sound %u has no place in a VAB", kat, id);
            continue;
        }
        if (b->tone[prog * VAB_TONES + tone])
            continue;   /* the first of two with one id */
        frames = len / bps;
        pcm = malloc((frames ? frames : 1) * sizeof *pcm);
        for (j = 0; j < frames; j++)
            pcm[j] = bits == 8 ? (int16_t)((int8_t)d[off + j] * 256)
                               : (int16_t)(d[off + 2 * j] | d[off + 2 * j + 1] << 8);
        pid = PC_SoundPcmAdd(pcm, frames, (int)rate, loop != 0, ls / bps, le / bps);
        free(pcm);
        if (pid < 0) {
            PC_Log("%s: too many samples", kat);
            break;
        }
        b->tone[prog * VAB_TONES + tone] = pid + 1;
    }
    free(d);
    PC_Trace("%s: %u sounds", kat, n);
    return b;
}

uint8_t *PC_DcSoundVab(const char *vab, int *outSize) {
    char kat[64];
    const char *base = vab, *p;
    Bank *b;
    uint8_t *v;
    int progs = 0, vags = 1, total = 0, tones[VAB_PROGS_MAX], prog, t, size, head;
    size_t n;
    for (p = vab; *p; p++)
        if (*p == '\\' || *p == '/' || *p == ':')
            base = p + 1;
    n = strcspn(base, ";");
    if (n < 4 || n > sizeof kat - 1 || _strnicmp(base + n - 4, ".vab", 4))
        return NULL;
    snprintf(kat, sizeof kat, "%.*s.kat", (int)n - 4, base);
    if (!(b = LoadBank(kat)))
        return NULL;
    for (prog = 0; prog < VAB_PROGS_MAX; prog++) {
        tones[prog] = 0;
        for (t = 0; t < VAB_TONES; t++)
            if (b->tone[prog * VAB_TONES + t]) {
                tones[prog] = t + 1;
                progs = prog + 1;
                vags++;
            }
    }
    if (!progs)
        progs = 1;
    head = VAB_HEAD(progs);
    size = head + vags * PC_SOUND_PCM_STUB;
    v = calloc(1, (size_t)size);
    memcpy(v, "pBAV", 4);
    v[4] = 7;                                       /* version */
    *(uint32_t *)(v + 12) = (uint32_t)size;
    for (prog = 0; prog < progs; prog++)
        total += tones[prog];
    *(uint16_t *)(v + 0x12) = (uint16_t)progs;
    *(uint16_t *)(v + 0x14) = (uint16_t)total;
    *(uint16_t *)(v + 0x16) = (uint16_t)vags;
    v[0x18] = 127;                                  /* master volume, pan */
    v[0x19] = 64;
    /* sample 1 is silence (the tones the .kat lacks), then one stand-in per sound */
    v[head + 16 + 1] = 1;
    vags = 1;
    for (prog = 0; prog < progs; prog++) {
        uint8_t *pa = v + 0x20 + prog * 16;
        pa[0] = (uint8_t)tones[prog];
        pa[1] = 127;
        pa[4] = 64;
        for (t = 0; t < VAB_TONES; t++) {
            uint8_t *ta = v + 0x820 + (prog * VAB_TONES + t) * 32;
            int pid = b->tone[prog * VAB_TONES + t] - 1;
            int looped = 0;
            ta[2] = 127;                            /* volume, pan, centre note, fine, range */
            ta[3] = 64;
            ta[4] = 60;
            ta[7] = 127;
            *(int16_t *)(ta + 20) = (int16_t)prog;
            *(int16_t *)(ta + 22) = 1;
            if (pid >= 0) {
                PC_SoundPcmStub(pid, v + head + ++vags * PC_SOUND_PCM_STUB - PC_SOUND_PCM_STUB);
                *(int16_t *)(ta + 22) = (int16_t)vags;
                looped = PC_SoundPcmLooped(pid);
            }
            *(uint16_t *)(ta + 16) = looped ? ADSR1_LOOP : ADSR1_ONESHOT;
            *(uint16_t *)(ta + 18) = looped ? ADSR2_LOOP : ADSR2_ONESHOT;
        }
    }
    /* sample sizes in 8-byte units; entry 0 is not a sample */
    for (t = 1; t <= vags; t++)
        *(uint16_t *)(v + 0x820 + progs * 512 + t * 2) = PC_SOUND_PCM_STUB >> 3;
    *outSize = size;
    return v;
}

/* The Dreamcast version's sound effect table (1ST_READ.BIN 0x8c11dcd4), the PS1's DAT_800b8a9c
 * for its .kat banks. Mostly the same voices; but some sounds moved (id 0, the default rolling
 * loop, is tone 6 on the Dreamcast, where tone 0 is the one-shot that id 6 plays at take-off),
 * and many volumes changed. */
static const uint32_t k_dcSfx[GAME_SFX_COUNT * 2] = {
    0x80000040, 170, 0x80000002, 92, 4, 0x3c005a, 8, 100, 16, 0x3f0064, 32, 100, 1, 0x3c0055,
    0x80000080, 60, 256, 200, 512, 220, 1024, 220, 0x80010001, 140, 0x80010002, 170, 0x80010004,
    0x3c0096, 0x80010008, 100, 0x80010010, 100, 0x10008, 200, 0x20001, 100, 0x20002, 400, 0x20004,
    200, 0x20008, 200, 0x20008, 300, 0x20010, 130, 0x20010, 100, 0x30001, 300, 0x30002, 200,
    0x30004, 200, 0x30008, 200, 0x40001, 150, 0x40002, 150, 0x40004, 150, 0x40008, 0x3a0064,
    0x40010, 100, 0x80050001, 150, 0x50002, 100, 0x50004, 100, 0x50008, 100, 0x50010, 100, 0x50020,
    130, 0x50040, 130, 0x50080, 150, 0x60001, 100, 0x70001, 180, 0x70002, 180, 0xc0000001, 50,
    0x40000002, 100, 0x40000004, 100, 0x40000008, 100, 0x40000010, 100, 0x40000020, 100, 0x40000040,
    100, 0x40000080, 100, 0x40000100, 70, 0x40010001, 145, 0xc0020001, 100, 0xc0030001, 140,
    0xc0040001, 100, 0xc0000001, 100, 0x40000002, 300, 0xc0000004, 100, 0x40000008, 100, 0xc0000010,
    100, 0xc0000020, 300, 0x40000040, 100, 0x40000080, 100, 0x40000100, 300, 0x40000200, 100,
    0x40000400, 100, 0x40000800, 100, 0x40001000, 100, 0x80000002, 100, 0xc0010001, 100, 0xc0000002,
    100, 0xc0000004, 120, 0xc0000008, 120, 0xc0000010, 500, 0xc0000020, 300, 0xc0000040, 300,
    0xc0000080, 300, 0x40010001, 100, 0x40010002, 100, 0x40010004, 100, 0x40010008, 100, 0x40010010,
    80, 0x40010020, 60, 0x40010040, 400, 0x40010080, 250, 0x40010100, 300, 0x40010200, 300,
    0xc0020001, 500, 0xc0030001, 0x4600c8, 0x40000001, 100, 0x40000004, 100, 0x40000008, 0x3c01f4,
    0x40000010, 300, 0xc0000020, 300, 0xc0000040, 300, 0xc0000080, 100, 0x40000100, 500, 0x40000200,
    500, 0x40000400, 300, 0x40000800, 300, 0x40002000, 140, 0x40004000, 100, 0x40008000, 140,
    0xc0010001, 100, 0x40000200, 500, 0x40000400, 500, 0x40000800, 120, 0x40001000, 100, 0x40010001,
    100, 0x40000040, 100, 0x40000080, 100, 0xc0000100, 0x32004b, 0x40010010, 200, 0x40010020, 500,
    0x40010040, 500, 0x40010400, 500, 0x40010800, 500, 0xc0050001, 100, 0x40000008, 100, 0xc0000004,
    100, 0x40000020, 100, 0xc0000040, 100, 0x40000080, 200, 0x40000004, 200, 0x40000100, 200,
    0x40000800, 200, 0x40001000, 200, 0xc0010001, 100, 0x40010002, 100, 0x40010004, 100, 0xc0010008,
    100, 0xc0010010, 100, 0xc0020001, 100, 0x40000200, 100, 0x40010001, 100, 0x40010002, 200,
    0x40010004, 300, 0xc0010008, 60, 0xc0020001, 300, 0xc0020002, 100, 0xc0020004, 170, 0xc0020008,
    100, 1, 170, 2, 170, 0x80004, 170, 8, 170, 16, 170, 32, 170, 64, 170, 128, 170, 0x40000001, 130,
    0x40000004, 130, 0x40000008, 130, 0x40000010, 200, 0x40000020, 200, 0xa0010001, 250, 0xa0010002,
    250, 0x20020, 300, 0x40000200, 300, 0x20040, 300, 0x20080, 400, 0x40010010, 100, 0x40010020, 90,
    0xc0010040, 100, 0x40010080, 200, 0x40010100, 200, 0x40010200, 80, 0xc0060001, 100, 0xc0020010,
    100, 0xc0030001, 100, 0xa0010004, 250, 0xa0010008, 250, 0xa0010010, 250,
};

/* The bits that pick the voice: the bank, the program and the tones. The rest stay the PS1's: bit
 * 31 (the game keeps a looping sound's voice until it stops it, which the PS1 code does for the
 * PS1 table's loops) and the Dreamcast's bit 29, which playSFX doesn't know. */
#define SFX_VOICE_BITS 0x407fffffu

void PC_DcSoundTable(void) {
    for (int i = 0; i < GAME_SFX_COUNT; i++) {
        uint32_t ps1 = (uint32_t)DAT_800b8a9c[2 * i];
        DAT_800b8a9c[2 * i] = (int32_t)((ps1 & ~SFX_VOICE_BITS) | (k_dcSfx[2 * i] & SFX_VOICE_BITS));
        DAT_800b8a9c[2 * i + 1] = (int32_t)k_dcSfx[2 * i + 1];
    }
}

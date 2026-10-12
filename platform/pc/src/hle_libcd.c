/* libcd over a raw (2352-byte sector, MODE2/2352) disc image. Reads complete immediately; the
 * asynchronous API shape (CdRead + CdReadSync, callbacks) is kept.
 *
 * Files extracted to Data/ (pc_assets.c) replace their disc copies: CdSearchFile finds the
 * extracted file and "mounts" it at an LBA past the end of the image, and every read in that
 * range comes from the file. So the game's own CD code (Setloc + CdRead2 for the songs and the
 * movies) runs unchanged on files that no longer need the disc.
 *
 * The image itself is only opened when something has to come from it (first-startup extraction,
 * or a file that isn't in Data/), so once the data is extracted it isn't needed at all.
 *
 * Streaming (CdRead2 / CdlReadS, used for the music and the movies) reads from the current
 * location at the drive's speed, 75 or 150 sectors/s, paced on the vblank clock
 * (PC_CdStreamVBlank). With XA enabled (mode bit 0x40) audio sectors go through the XA decoder to
 * the SPU's CD input. STR video chunks are assembled into whole frames in the ring the game set
 * with StSetRing, for StGetNext/StFreeRing. A full ring drops a frame instead of stalling the
 * drive, so the music never waits on the game.
 *
 * The Dreamcast version's films (.sfd, pc_sfd.h) mount the same way, as many positions as the
 * file has 2352-byte blocks. Streaming one with XA on plays its ADX audio through the same CD
 * input, kept a little ahead of the mixer instead of paced by sectors; the position advances
 * through the file as it is read. Its video is not streamed (the movie player reads it itself:
 * pc_movie.c), so a song's end is not seen by the game's frame counter: the end of its audio
 * stops the stream and tells pc_music.c, which loops the song as the end frame would. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_assets.h"
#include "pc_hle.h"
#include "pc_runtime.h"
#include "pc_sfd.h"
#include "pc_spu.h"
#include "pc_xa.h"

typedef struct {
    uint8_t minute, second, sector, track;
} CdlLOC;

typedef struct {
    CdlLOC pos;
    uint32_t size;
    char name[16];
} CdlFILE;

enum {
    CdlNop = 1, CdlSetloc = 2, CdlPlay = 3, CdlForward = 4, CdlBackward = 5, CdlReadN = 6,
    CdlStandby = 7, CdlStop = 8, CdlPause = 9, CdlMute = 11, CdlDemute = 12, CdlSetfilter = 13,
    CdlSetmode = 14, CdlGetparam = 15, CdlGetlocL = 16, CdlGetlocP = 17, CdlGetTN = 19,
    CdlGetTD = 20, CdlSeekL = 21, CdlSeekP = 22, CdlReadS = 27
};
#define CdlComplete 2
#define CdlDataReady 1
#define CdlStatStandby 0x02

static FILE *s_disc;
static uint32_t s_sectors;
static uint32_t s_loc; /* current LBA */
static uint8_t s_mode;
static uint32_t s_readCallback, s_syncCallback;
static int s_debug;

/* ---- streaming state ---------------------------------------------------------------------- */

#define CdlModeSpeed 0x80
#define CdlModeRT 0x40      /* XA-ADPCM playback */
#define STR_MAGIC 0x0160
#define STR_HEADER 32
#define STR_PAYLOAD 2016
#define ST_MAX_FRAMES 8

enum { FR_FREE, FR_FILLING, FR_READY, FR_TAKEN };
typedef struct {
    int state;
    uint32_t addr, size; /* slot in the ring: the sector header, then the frame's payload */
    uint32_t frame, seq;
    int chunks, got;
} StFrame;

static int s_streaming, s_streamXa;
static uint32_t s_streamAcc;  /* sectors owed, in sixtieths */
static int s_jumped;          /* a Setloc/Seek moved the head: the XA history starts over */
static uint32_t s_ring, s_ringSize, s_ringWrite;
static int s_stMask;
static StFrame s_fr[ST_MAX_FRAMES];
static uint32_t s_frSeq;
static int s_filling = -1;

static void StreamStart(int mode, const char *why) {
    s_mode = (uint8_t)mode;
    s_streamXa = (mode & CdlModeRT) != 0;
    if (s_jumped)
        PC_XaReset();
    s_jumped = 0;
    s_streaming = 1;
    s_streamAcc = 0;
    PC_Trace("%s: stream from lba %u (mode %02x%s)", why, s_loc, mode & 0xff,
             s_streamXa ? ", XA" : "");
}

static void StreamStop(void) {
    if (s_streaming)
        PC_Trace("stream stopped at lba %u", s_loc);
    s_streaming = 0;
}

int CdPosToInt(CdlLOC *p);

static void Locate(const uint8_t *param) {
    uint32_t lba = (uint32_t)CdPosToInt((CdlLOC *)param);
    if (lba != s_loc)
        s_jumped = 1;
    s_loc = lba;
}

/* ---- mounted files ------------------------------------------------------------------------ */

#define MAX_MOUNTS 256
/* Mounted files start past the end of the disc (the USA disc ends at LBA 280809), whether or not
 * the image is open. CdIntToPos/CdPosToInt carry the minute's tens digit in a nibble, so
 * positions reach 159:59:74: room for the 26 STR files and a few more songs. */
#define MOUNT_ALIGN 75
#define MOUNT_BASE (63 * 60 * 75)
#define MOUNT_END (160 * 60 * 75 - 150)

typedef struct {
    char path[MAX_PATH]; /* full path */
    FILE *f;
    uint32_t lba, sectors;
    int isSfd;           /* a Dreamcast film: ADX audio streams from it (pc_sfd.h) */
    PcSfd *sfd;          /* opened on first stream */
    uint32_t sfdLoc;     /* the position the last stream left the drive at */
} Mount;

static Mount s_mounts[MAX_MOUNTS];
static int s_nmounts;
static uint32_t s_mountNext;

int PC_CdMountFile(const char *path, uint32_t *lba, uint32_t *sectors) {
    char full[MAX_PATH];
    Mount *m;
    long long size;
    int i;
    if (!GetFullPathNameA(path, sizeof full, full, NULL))
        return 0;
    for (i = 0; i < s_nmounts; i++)
        if (!_stricmp(s_mounts[i].path, full)) {
            *lba = s_mounts[i].lba;
            *sectors = s_mounts[i].sectors;
            return 1;
        }
    if (s_nmounts == MAX_MOUNTS)
        return 0;
    m = &s_mounts[s_nmounts];
    m->f = fopen(full, "rb");
    if (!m->f)
        return 0;
    _fseeki64(m->f, 0, SEEK_END);
    size = _ftelli64(m->f);
    if (!s_mountNext)
        s_mountNext = s_sectors > MOUNT_BASE ? (s_sectors + MOUNT_ALIGN) / MOUNT_ALIGN * MOUNT_ALIGN
                                             : MOUNT_BASE;
    if (s_mountNext + size / 2352 > MOUNT_END) {
        PC_Log("cannot mount %s: no CD positions left", full);
        fclose(m->f);
        return 0;
    }
    snprintf(m->path, sizeof m->path, "%s", full);
    {
        size_t n = strlen(full);
        m->isSfd = n > 4 && !_stricmp(full + n - 4, ".sfd");
    }
    m->lba = s_mountNext;
    m->sectors = (uint32_t)(size / 2352);
    s_mountNext += (m->sectors + MOUNT_ALIGN) / MOUNT_ALIGN * MOUNT_ALIGN;
    s_nmounts++;
    PC_Trace("mounted %s at lba %u (%u sectors)", full, m->lba, m->sectors);
    *lba = m->lba;
    *sectors = m->sectors;
    return 1;
}

static Mount *MountAt(uint32_t lba) {
    for (int i = 0; i < s_nmounts; i++)
        if (lba >= s_mounts[i].lba && lba < s_mounts[i].lba + s_mounts[i].sectors)
            return &s_mounts[i];
    return NULL;
}

static int FileExists(const char *p) {
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static int s_noDisc;

void PC_CdSetNoDisc(int on) { s_noDisc = on; }

int PC_CdFindImage(char *path, int n) {
    const char *dir = PC_DataDir();
    const char *fmts[] = {"%s\\thps1.bin", "%s\\..\\thps1_disc\\thps1.bin",
                          "%s\\Tony Hawk's Pro Skater (USA).bin", "%s\\game\\thps1.bin"};
    const char *e = getenv("THPS_DISC");
    int i;
    if (e) {
        size_t len = strlen(e);
        snprintf(path, n, "%s", e);
        /* a .gdi is the Dreamcast disc (pc_assets.c) */
        return len < 4 || _stricmp(e + len - 4, ".gdi") ? FileExists(path) : 0;
    }
    for (i = 0; i < 4; i++) {
        snprintf(path, n, fmts[i], dir);
        if (FileExists(path))
            return 1;
    }
    return 0;
}

int PC_CdOpen(void) {
    char path[MAX_PATH];
    if (s_disc)
        return 1;
    if (s_noDisc)
        return 0;
    if (!PC_CdFindImage(path, sizeof path))
        PC_Fatal("The game data has to be extracted from your disc first, and no disc image was "
                 "found. Put a raw .bin image (MODE2/2352) of Tony Hawk's Pro Skater (USA) next "
                 "to SLUS_008.60 as thps1.bin, or set THPS_DISC.");
    s_disc = fopen(path, "rb");
    if (!s_disc)
        PC_Fatal("cannot open %s", path);
    _fseeki64(s_disc, 0, SEEK_END);
    s_sectors = (uint32_t)(_ftelli64(s_disc) / 2352);
    PC_Log("disc image %s (%u sectors)", path, s_sectors);
    return 1;
}

int PC_CdReadDiscSectors(uint32_t lba, uint32_t n, void *dst) {
    if (!s_disc && !PC_CdOpen())
        return 0;
    if (lba >= s_sectors || n > s_sectors - lba)
        return 0;
    _fseeki64(s_disc, (long long)lba * 2352, SEEK_SET);
    return fread(dst, 2352, n, s_disc) == n;
}

const char *PC_CdMountPath(uint32_t lba) {
    Mount *m = MountAt(lba);
    return m ? m->path : NULL;
}

int PC_CdReadRawSector(uint32_t lba, void *dst) {
    Mount *m = MountAt(lba);
    if (m && m->isSfd)
        return 0;   /* no sectors: only its audio streams (StreamSfd) */
    if (m) {
        _fseeki64(m->f, (long long)(lba - m->lba) * 2352, SEEK_SET);
        return fread(dst, 1, 2352, m->f) == 2352;
    }
    return PC_CdReadDiscSectors(lba, 1, dst);
}

int PC_CdReadSector(uint32_t lba, void *dst) {
    uint8_t raw[2352];
    if (!PC_CdReadRawSector(lba, raw))
        return 0;
    memcpy(dst, raw + 24, 2048);
    return 1;
}

/* ---- positions ---------------------------------------------------------------------------- */

static int Bcd(int v) { return (v / 10) << 4 | (v % 10); }
static int UnBcd(int v) { return (v >> 4) * 10 + (v & 15); }

CdlLOC *CdIntToPos(int i, CdlLOC *p) {
    i += 150;
    p->minute = (uint8_t)Bcd(i / (75 * 60));
    p->second = (uint8_t)Bcd(i / 75 % 60);
    p->sector = (uint8_t)Bcd(i % 75);
    p->track = 0;
    return p;
}

int CdPosToInt(CdlLOC *p) {
    return (UnBcd(p->minute) * 60 + UnBcd(p->second)) * 75 + UnBcd(p->sector) - 150;
}

/* ---- ISO 9660 ----------------------------------------------------------------------------- */

static int FindInDir(uint32_t lba, uint32_t size, const char *name, int len, uint32_t *outLba,
                     uint32_t *outSize) {
    uint8_t sec[2048];
    uint32_t off;
    for (off = 0; off < size; off += 2048) {
        int p = 0;
        if (!PC_CdReadSector(lba + off / 2048, sec))
            return 0;
        while (p < 2048 && sec[p]) {
            int rl = sec[p], nl = sec[p + 32];
            const char *n = (const char *)sec + p + 33;
            /* compare ignoring the ";1" version on either side */
            const char *rs = memchr(n, ';', nl);
            int cmp = rs ? (int)(rs - n) : nl;
            {
                int want = len;
                const char *semi = memchr(name, ';', len);
                if (semi)
                    want = (int)(semi - name);
                if (cmp == want && !_strnicmp(n, name, want)) {
                    memcpy(outLba, sec + p + 2, 4);
                    memcpy(outSize, sec + p + 10, 4);
                    return 1;
                }
            }
            p += rl;
        }
    }
    return 0;
}

int PC_CdFindDiscFile(const char *name, uint32_t *outLba, uint32_t *outSize) {
    uint8_t pvd[2048];
    uint32_t lba, size;
    const char *p = name;
    if (!PC_CdReadSector(16, pvd))
        return 0;
    memcpy(&lba, pvd + 156 + 2, 4);
    memcpy(&size, pvd + 156 + 10, 4);
    while (*p) {
        const char *e;
        while (*p == '\\' || *p == '/')
            p++;
        if (!*p)
            break;
        e = p;
        while (*e && *e != '\\' && *e != '/')
            e++;
        if (!FindInDir(lba, size, p, (int)(e - p), &lba, &size))
            return 0;
        p = e;
    }
    *outLba = lba;
    *outSize = size;
    return 1;
}

CdlFILE *CdSearchFile(CdlFILE *fp, const char *name) {
    char path[MAX_PATH];
    uint32_t lba, size;
    const char *base;
    if (PC_AssetsFindFilm(name, path, sizeof path) && PC_CdMountFile(path, &lba, &size)) {
        size *= 2048; /* ISO sizes count user data */
    } else if (!PC_CdFindDiscFile(name, &lba, &size)) {
        PC_Log("CdSearchFile(%s): not found", name);
        return NULL;
    }
    CdIntToPos((int)lba, &fp->pos);
    fp->size = size;
    base = strrchr(name, '\\');
    snprintf(fp->name, sizeof fp->name, "%s", base ? base + 1 : name);
    PC_Trace("CdSearchFile(%s) -> lba %u size %u", name, lba, size);
    return fp;
}

/* ---- commands ----------------------------------------------------------------------------- */

static void ResultStat(uint8_t *result) {
    if (result)
        result[0] = CdlStatStandby;
}

int CdControl(uint8_t com, uint8_t *param, uint8_t *result) {
    switch (com) {
    case CdlSetloc:
        if (param)
            Locate(param);
        break;
    case CdlSetmode:
        if (param)
            s_mode = param[0];
        break;
    case CdlReadS:
        StreamStart(s_mode, "CdlReadS");
        break;
    case CdlPause:
    case CdlStop:
        StreamStop();
        break;
    case CdlMute:
        PC_SpuCdMute(1);
        break;
    case CdlDemute:
        PC_SpuCdMute(0);
        break;
    case CdlGetlocL:
        if (result) {
            CdlLOC l;
            CdIntToPos((int)s_loc, &l);
            memcpy(result, &l, 3);
        }
        return 1;
    case CdlSeekL:
    case CdlSeekP:
        if (param)
            Locate(param);
        StreamStop();
        break;
    default:
        PC_Trace("CdControl(%d)", com);
        break;
    }
    ResultStat(result);
    return 1;
}

int CdControlB(uint8_t com, uint8_t *param, uint8_t *result) { return CdControl(com, param, result); }
int CdControlF(uint8_t com, uint8_t *param) { return CdControl(com, param, NULL); }

int CdInit(void) {
    return 1;
}

int hle_8008f208(int level) { /* CdSetDebug */
    int old = s_debug;
    s_debug = level;
    return old;
}

static int s_pending; /* sectors of the last read that "remain" (always 0: reads are immediate) */

int CdRead(int sectors, uint32_t buf, int mode) {
    int i;
    uint8_t *dst = PSX_PTR(buf);
    int size = (mode & 0x20) ? 2340 : 2048; /* CdlModeSize1: whole sector minus sync */
    PC_Trace("CdRead(%d sectors at lba %u -> %08x, mode %02x)", sectors, s_loc, buf, mode);
    StreamStop(); /* the drive does one read at a time */
    for (i = 0; i < sectors; i++) {
        if (size == 2048) {
            PC_CdReadSector(s_loc, dst);
        } else {
            uint8_t raw[2352];
            PC_CdReadRawSector(s_loc, raw);
            memcpy(dst, raw + 12, 2340);
        }
        dst += size;
        s_loc++;
    }
    s_pending = 0;
    if (s_readCallback) {
        uint8_t res[8] = {CdlStatStandby};
        ((void (*)(int, uint8_t *))PSX_PTR(s_readCallback))(CdlComplete, res);
    }
    return 1;
}

int CdReadSync(int mode, uint8_t *result) {
    (void)mode;
    PC_TickVBlank();
    ResultStat(result);
    return s_pending;
}

int CdSync(int mode, uint8_t *result) {
    (void)mode;
    PC_TickVBlank();
    ResultStat(result);
    return CdlComplete;
}

uint32_t CdReadCallback(uint32_t func) {
    uint32_t old = s_readCallback;
    s_readCallback = func;
    return old;
}

uint32_t CdSyncCallback(uint32_t func) {
    uint32_t old = s_syncCallback;
    s_syncCallback = func;
    return old;
}

uint32_t CdReadyCallback(uint32_t func) {
    (void)func;
    return 0;
}

int CdRead2(int mode) {
    StreamStart(mode, "CdRead2");
    return 1;
}

/* CdMix(CdlATV *): the CD-audio-to-SPU mixer levels. */
int CdMix(void *vol) {
    if (vol)
        PC_SpuCdSetMix((const uint8_t *)vol);
    return 1;
}

int CdStatus(void) { return CdlStatStandby; }
int CdMode(void) { return s_mode; }
int CdLastCom(void) { return 0; }

/* ---- streaming: the St* ring library -------------------------------------------------------- */

static void ClearFrames(void) {
    memset(s_fr, 0, sizeof s_fr);
    s_filling = -1;
    s_ringWrite = s_ring;
}

static int Overlaps(uint32_t a, uint32_t n) {
    for (int i = 0; i < ST_MAX_FRAMES; i++)
        if (s_fr[i].state != FR_FREE && a < s_fr[i].addr + s_fr[i].size && s_fr[i].addr < a + n)
            return 1;
    return 0;
}

/* A slot of `need` bytes in the ring for a new frame, or -1 if the ring is full. */
static int AllocFrame(uint32_t need) {
    int i;
    uint32_t a = s_ringWrite;
    need = (need + 3) & ~3u;
    if (!s_ring || need > s_ringSize)
        return -1;
    for (i = 0; i < ST_MAX_FRAMES && s_fr[i].state != FR_FREE; i++)
        ;
    if (i == ST_MAX_FRAMES)
        return -1;
    if (a + need > s_ring + s_ringSize)
        a = s_ring;
    if (Overlaps(a, need))
        return -1;
    s_fr[i].addr = a;
    s_fr[i].size = need;
    s_ringWrite = a + need;
    return i;
}

/* One STR video chunk (2048 bytes of user data: 32-byte header, 2016 bytes of payload). */
static void StreamData(const uint8_t *u) {
    StFrame *f;
    int chunk, chunks;
    uint32_t frame;
    if ((u[0] | u[1] << 8) != STR_MAGIC || !s_ring || s_stMask)
        return;
    chunk = u[4] | u[5] << 8;
    chunks = u[6] | u[7] << 8;
    frame = u[8] | u[9] << 8 | u[10] << 16 | (uint32_t)u[11] << 24;
    if (chunks <= 0 || chunks > 64 || chunk >= chunks)
        return;
    f = s_filling >= 0 ? &s_fr[s_filling] : NULL;
    if (f && (f->frame != frame || f->chunks != chunks)) { /* the last one never completed */
        f->state = FR_FREE;
        f = NULL;
        s_filling = -1;
    }
    if (!f) {
        int i = AllocFrame(STR_HEADER + (uint32_t)chunks * STR_PAYLOAD);
        if (i < 0)
            return;                         /* ring full: drop this frame, keep streaming */
        f = &s_fr[i];
        f->state = FR_FILLING;
        f->frame = frame;
        f->chunks = chunks;
        f->got = 0;
        memcpy(PSX_PTR(f->addr), u, STR_HEADER);
        s_filling = i;
    }
    memcpy(PSX_PTR(f->addr + STR_HEADER + (uint32_t)chunk * STR_PAYLOAD), u + STR_HEADER,
           STR_PAYLOAD);
    if (++f->got >= f->chunks) {
        f->state = FR_READY;
        f->seq = s_frSeq++;
        s_filling = -1;
    }
}

/* ---- streaming a Dreamcast film ------------------------------------------------------------ */

#define SFD_AHEAD 12288     /* frames kept queued for the mixer (its prebuffer is 8192) */
#define SFD_CHUNK 1024

static Mount *s_sfdStream;      /* the film whose audio plays */
static uint64_t s_sfdHeardFrom; /* the mixer's frame count at which its first frame plays */
static int s_sfdRate;

static void StreamSfd(Mount *m) {
    static int16_t pcm[SFD_CHUNK * 2];
    uint64_t pos;
    if (!m->sfd && !(m->sfd = PC_SfdOpen(m->path))) {
        PC_Log("cannot read %s", m->path);
        StreamStop();
        return;
    }
    if (s_loc != m->sfdLoc || s_sfdStream != m) {
        /* the game moved the head (the start of a song or a movie): read from there */
        PC_SfdSeek(m->sfd, (long long)(s_loc - m->lba) * 2352);
        s_sfdStream = m;
        s_sfdHeardFrom = PC_SpuCdConsumed() + PC_SpuCdQueued();
        s_sfdRate = 0;
        PC_Trace("film %s from position %u", m->path, s_loc - m->lba);
    }
    while (s_streamXa && PC_SpuCdQueued() < SFD_AHEAD) {
        int rate, n = PC_SfdAudio(m->sfd, pcm, SFD_CHUNK, &rate);
        if (!n) {
            PC_Trace("film %s: end of its audio", m->path);
            StreamStop();
            m->sfdLoc = s_loc = m->lba + m->sectors;
            PC_MusicStreamEnded();
            return;
        }
        s_sfdRate = rate;
        PC_SpuCdQueue(pcm, n, rate);
    }
    pos = (uint64_t)PC_SfdTell(m->sfd) / 2352;
    m->sfdLoc = s_loc = m->lba + (pos < m->sectors ? (uint32_t)pos : m->sectors - 1);
}

double PC_CdFilmTime(uint32_t lba) {
    uint64_t now = PC_SpuCdConsumed();
    if (!s_sfdStream || MountAt(lba) != s_sfdStream || !s_sfdRate || now < s_sfdHeardFrom)
        return 0.0;
    return (double)(now - s_sfdHeardFrom) / s_sfdRate;
}

/* Called once per vblank: read the sectors the drive would have read since the last one. */
void PC_CdStreamVBlank(void) {
    static uint8_t raw[2352];
    static int16_t pcm[PC_XA_MAX_FRAMES * 2];
    Mount *m;
    if (!s_streaming)
        return;
    if ((m = MountAt(s_loc)) && m->isSfd) {
        StreamSfd(m);
        return;
    }
    s_streamAcc += (s_mode & CdlModeSpeed) ? 150 : 75;
    while (s_streamAcc >= 60) {
        s_streamAcc -= 60;
        if (!PC_CdReadRawSector(s_loc, raw)) {
            StreamStop();
            return;
        }
        s_loc++;
        if (raw[15] != 2)
            continue;
        if (raw[18] & 0x04) {               /* submode: audio */
            int rate, n;
            if (s_streamXa && (n = PC_XaDecodeSector(raw, pcm, &rate)))
                PC_SpuCdQueue(pcm, n, rate);
        } else {
            StreamData(raw + 24);
        }
    }
}

void StSetRing(uint32_t ring, int size) {
    s_ring = ring;
    s_ringSize = (uint32_t)size * 2048;
    ClearFrames();
}

void StClearRing(void) { ClearFrames(); }

void StUnSetRing(void) {
    s_ring = 0;
    s_ringSize = 0;
    ClearFrames();
}

void StSetStream(int mode, int start, int end, uint32_t f1, uint32_t f2) {
    (void)mode; (void)start; (void)end; (void)f1; (void)f2;
    ClearFrames();
}

/* StGetNext(&addr, &header): the oldest complete frame. 0 = got one, 1 = none ready. */
int StGetNext(uint32_t *addr, uint32_t *header) {
    StFrame *best = NULL;
    for (int i = 0; i < ST_MAX_FRAMES; i++)
        if (s_fr[i].state == FR_READY && (!best || s_fr[i].seq < best->seq))
            best = &s_fr[i];
    if (!best)
        return 1;
    best->state = FR_TAKEN;
    *addr = best->addr + STR_HEADER;
    *header = best->addr;
    return 0;
}

int StFreeRing(uint32_t base) {
    for (int i = 0; i < ST_MAX_FRAMES; i++)
        if (s_fr[i].state != FR_FREE && s_fr[i].addr + STR_HEADER == base) {
            s_fr[i].state = FR_FREE;
            return 0;
        }
    return 1;
}

void StSetMask(int mask, int start, int end) {
    (void)start; (void)end;
    s_stMask = mask;
}

void StCdInterrupt(void) {}

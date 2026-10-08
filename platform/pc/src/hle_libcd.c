/* libcd over a raw (2352-byte sector, MODE2/2352) disc image. Reads complete immediately; the
 * asynchronous API shape (CdRead + CdReadSync, callbacks) is kept. Streaming (XA audio, STR
 * movies) is not implemented yet: CdlReadS just reports success. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_hle.h"
#include "pc_runtime.h"

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

static int FileExists(const char *p) {
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

int PC_CdOpen(void) {
    char path[MAX_PATH];
    const char *dir = PC_DataDir();
    const char *fmts[] = {"%s\\thps1.bin", "%s\\..\\thps1_disc\\thps1.bin",
                          "%s\\Tony Hawk's Pro Skater (USA).bin", "%s\\game\\thps1.bin"};
    int i, found = 0;
    if (s_disc)
        return 1;
    if (getenv("THPS_DISC")) {
        snprintf(path, sizeof path, "%s", getenv("THPS_DISC"));
        found = FileExists(path);
    }
    for (i = 0; !found && i < 4; i++) {
        snprintf(path, sizeof path, fmts[i], dir);
        found = FileExists(path);
    }
    if (!found)
        PC_Fatal("No disc image found. Put a raw .bin image (MODE2/2352) of Tony Hawk's Pro "
                 "Skater (USA) next to SLUS_008.60 as thps1.bin, or set THPS_DISC.");
    s_disc = fopen(path, "rb");
    if (!s_disc)
        PC_Fatal("cannot open %s", path);
    _fseeki64(s_disc, 0, SEEK_END);
    s_sectors = (uint32_t)(_ftelli64(s_disc) / 2352);
    PC_Log("disc image %s (%u sectors)", path, s_sectors);
    return 1;
}

int PC_CdReadRawSector(uint32_t lba, void *dst) {
    if (!s_disc && !PC_CdOpen())
        return 0;
    if (lba >= s_sectors)
        return 0;
    _fseeki64(s_disc, (long long)lba * 2352, SEEK_SET);
    return fread(dst, 1, 2352, s_disc) == 2352;
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

CdlFILE *CdSearchFile(CdlFILE *fp, const char *name) {
    uint8_t pvd[2048];
    uint32_t lba, size;
    const char *p = name, *base;
    if (!PC_CdReadSector(16, pvd))
        return NULL;
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
        if (!FindInDir(lba, size, p, (int)(e - p), &lba, &size)) {
            PC_Log("CdSearchFile(%s): not found", name);
            return NULL;
        }
        p = e;
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
            s_loc = (uint32_t)CdPosToInt((CdlLOC *)param);
        break;
    case CdlSetmode:
        if (param)
            s_mode = param[0];
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
            s_loc = (uint32_t)CdPosToInt((CdlLOC *)param);
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
    PC_CdOpen();
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
    PC_Trace("CdRead2(%02x) at lba %u: streaming not implemented", mode, s_loc);
    return 1;
}

int CdMix(void *vol) {
    (void)vol;
    return 1;
}

int CdStatus(void) { return CdlStatStandby; }
int CdMode(void) { return s_mode; }
int CdLastCom(void) { return 0; }

/* Streaming library (STR movies / XA): inert until movies are supported. */
void StSetRing(uint32_t ring, int size) { (void)ring; (void)size; }
void StClearRing(void) {}
void StUnSetRing(void) {}
void StSetStream(int mode, int start, int end, uint32_t f1, uint32_t f2) {
    (void)mode; (void)start; (void)end; (void)f1; (void)f2;
}
int StGetNext(uint32_t *addr, uint32_t *header) {
    (void)addr; (void)header;
    return 1; /* no data */
}
int StFreeRing(uint32_t base) { (void)base; return 0; }
void StSetMask(int mask, int start, int end) { (void)mask; (void)start; (void)end; }
void StCdInterrupt(void) {}

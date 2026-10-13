/* The PS1 disc image, for extracting the game's files (see pc_disc.h). It is opened on the first
 * read, so once Data/ is complete it isn't needed at all. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_disc.h"
#include "pc_platform.h"
#include "pc_runtime.h"

#define SECTOR 2352

static FILE *s_disc;
static uint32_t s_sectors;
static int s_none;

void PC_DiscSetNone(int on) { s_none = on; }

int PC_DiscFindImage(char *path, int n) {
    const char *dir = PC_DataDir();
    /* next to thps.exe, or the development layout (thps.exe in build/pc or build/pc/release) */
#define S PC_SEP
    const char *fmts[] = {"%s" S "thps1.bin", "%s" S ".." S ".." S "extern" S "thps1_disc" S "thps1.bin",
                          "%s" S ".." S ".." S ".." S "extern" S "thps1_disc" S "thps1.bin",
                          "%s" S "Tony Hawk's Pro Skater (USA).bin", "%s" S "game" S "thps1.bin"};
#undef S
    const char *e = getenv("THPS_DISC");
    int i;
    if (e) {
        size_t len = strlen(e);
        snprintf(path, n, "%s", e);
        /* a .gdi is the Dreamcast disc (pc_assets.c) */
        return len < 4 || _stricmp(e + len - 4, ".gdi") ? PC_FileExists(path) : 0;
    }
    for (i = 0; i < 5; i++) {
        snprintf(path, n, fmts[i], dir);
        if (PC_FileExists(path))
            return 1;
    }
    return 0;
}

static int Open(void) {
    char path[MAX_PATH];
    if (s_disc)
        return 1;
    if (s_none)
        return 0;
    if (!PC_DiscFindImage(path, sizeof path))
        PC_Fatal("The game data has to be extracted from your disc first, and no disc image was "
                 "found. Put a raw .bin image (MODE2/2352) of Tony Hawk's Pro Skater (USA) next "
                 "to " PC_EXE_NAME " as thps1.bin, or set THPS_DISC.");
    s_disc = fopen(path, "rb");
    if (!s_disc)
        PC_Fatal("cannot open %s", path);
    PC_FSEEK(s_disc, 0, SEEK_END);
    s_sectors = (uint32_t)(PC_FTELL(s_disc) / SECTOR);
    PC_Log("disc image %s (%u sectors)", path, s_sectors);
    return 1;
}

int PC_DiscReadSectors(uint32_t lba, uint32_t n, void *dst) {
    if (!Open())
        return 0;
    if (lba >= s_sectors || n > s_sectors - lba)
        return 0;
    PC_FSEEK(s_disc, (long long)lba * SECTOR, SEEK_SET);
    return fread(dst, SECTOR, n, s_disc) == n;
}

/* Mode 2 form 1 user data. */
static int ReadUserData(uint32_t lba, uint8_t *dst2048) {
    uint8_t raw[SECTOR];
    if (!PC_DiscReadSectors(lba, 1, raw))
        return 0;
    memcpy(dst2048, raw + 24, 2048);
    return 1;
}

/* ---- ISO 9660 ----------------------------------------------------------------------------- */

static int FindInDir(uint32_t lba, uint32_t size, const char *name, int len, uint32_t *outLba,
                     uint32_t *outSize) {
    uint8_t sec[2048];
    uint32_t off;
    for (off = 0; off < size; off += 2048) {
        int p = 0;
        if (!ReadUserData(lba + off / 2048, sec))
            return 0;
        while (p < 2048 && sec[p]) {
            int rl = sec[p], nl = sec[p + 32];
            const char *n = (const char *)sec + p + 33;
            /* compare ignoring the ";1" version on either side */
            const char *rs = memchr(n, ';', nl);
            int cmp = rs ? (int)(rs - n) : nl;
            int want = len;
            const char *semi = memchr(name, ';', len);
            if (semi)
                want = (int)(semi - name);
            if (cmp == want && !_strnicmp(n, name, want)) {
                memcpy(outLba, sec + p + 2, 4);
                memcpy(outSize, sec + p + 10, 4);
                return 1;
            }
            p += rl;
        }
    }
    return 0;
}

int PC_DiscFindFile(const char *name, uint32_t *outLba, uint32_t *outSize) {
    uint8_t pvd[2048];
    uint32_t lba, size;
    const char *p = name;
    if (!ReadUserData(16, pvd))
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

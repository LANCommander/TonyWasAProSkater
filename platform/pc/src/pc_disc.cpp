/* The PS1 disc image, for extracting the game's files (see pc_disc.h). It is opened on the first
 * read, so once Data/ is complete it isn't needed at all. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/disc_image.h"
#include "pc_runtime.h"
#include "pc/platform.h"
#include "pc/runtime.h"

namespace pc {

namespace {
constexpr int kSector = 2352;
}

DiscImage g_discImage;

int DiscImage::FindImage(char *path, int n, bool nearOnly) {
    const char *dir = g_runtime.DataDir();
    /* next to thps.exe, or the development layout (thps.exe in build/pc or build/pc/release) */
#define S PC_SEP
    const char *fmts[] = {"%s" S "thps1.bin", "%s" S ".." S ".." S "extern" S "thps1_disc" S "thps1.bin",
                          "%s" S ".." S ".." S ".." S "extern" S "thps1_disc" S "thps1.bin",
                          "%s" S "Tony Hawk's Pro Skater (USA).bin", "%s" S "game" S "thps1.bin"};
#undef S
    const char *e = getenv("THPS_DISC");
    if (e) {
        size_t len = strlen(e);
        snprintf(path, n, "%s", e);
        /* a .gdi is the Dreamcast disc (pc_assets.cpp) */
        return len < 4 || _stricmp(e + len - 4, ".gdi") ? os::IsFile(path) : 0;
    }
    for (const char *fmt : fmts) {
        if (nearOnly && strstr(fmt, ".."))
            continue;
        snprintf(path, n, fmt, dir);
        if (os::IsFile(path))
            return 1;
    }
    return 0;
}

bool DiscImage::Open() {
    char path[MAX_PATH];
    if (m_file)
        return true;
    if (m_none)
        return false;
    if (!FindImage(path, sizeof path))
        PC_Fatal("The game data has to be extracted from your disc first, and no disc image was "
                 "found. Put a raw .bin image (MODE2/2352) of Tony Hawk's Pro Skater (USA) next "
                 "to " PC_EXE_NAME " as thps1.bin, or set THPS_DISC.");
    m_file = fopen(path, "rb");
    if (!m_file)
        PC_Fatal("cannot open %s", path);
    PC_FSEEK(m_file, 0, SEEK_END);
    m_sectors = (uint32_t)(PC_FTELL(m_file) / kSector);
    PC_Log("disc image %s (%u sectors)", path, m_sectors);
    return true;
}

bool DiscImage::ReadSectors(uint32_t lba, uint32_t n, void *dst) {
    if (!Open())
        return false;
    if (lba >= m_sectors || n > m_sectors - lba)
        return false;
    PC_FSEEK(m_file, (long long)lba * kSector, SEEK_SET);
    return fread(dst, kSector, n, m_file) == n;
}

/* Mode 2 form 1 user data. */
bool DiscImage::ReadUserData(uint32_t lba, uint8_t *dst2048) {
    uint8_t raw[kSector];
    if (!ReadSectors(lba, 1, raw))
        return false;
    memcpy(dst2048, raw + 24, 2048);
    return true;
}

/* ---- ISO 9660 ----------------------------------------------------------------------------- */

bool DiscImage::FindInDir(uint32_t lba, uint32_t size, const char *name, int len,
                          uint32_t *outLba, uint32_t *outSize) {
    uint8_t sec[2048];
    for (uint32_t off = 0; off < size; off += 2048) {
        int p = 0;
        if (!ReadUserData(lba + off / 2048, sec))
            return false;
        while (p < 2048 && sec[p]) {
            int rl = sec[p], nl = sec[p + 32];
            const char *n = (const char *)sec + p + 33;
            /* compare ignoring the ";1" version on either side */
            const char *rs = (const char *)memchr(n, ';', nl);
            int cmp = rs ? (int)(rs - n) : nl;
            int want = len;
            const char *semi = (const char *)memchr(name, ';', len);
            if (semi)
                want = (int)(semi - name);
            if (cmp == want && !_strnicmp(n, name, want)) {
                memcpy(outLba, sec + p + 2, 4);
                memcpy(outSize, sec + p + 10, 4);
                return true;
            }
            p += rl;
        }
    }
    return false;
}

bool DiscImage::FindFile(const char *name, uint32_t *outLba, uint32_t *outSize) {
    uint8_t pvd[2048];
    uint32_t lba, size;
    const char *p = name;
    if (!ReadUserData(16, pvd))
        return false;
    memcpy(&lba, pvd + 156 + 2, 4);
    memcpy(&size, pvd + 156 + 10, 4);
    while (*p) {
        while (*p == '\\' || *p == '/')
            p++;
        if (!*p)
            break;
        const char *e = p;
        while (*e && *e != '\\' && *e != '/')
            e++;
        if (!FindInDir(lba, size, p, (int)(e - p), &lba, &size))
            return false;
        p = e;
    }
    *outLba = lba;
    *outSize = size;
    return true;
}

} // namespace pc

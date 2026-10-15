/* Reading files off a Dreamcast GD-ROM dump (see pc_gdrom.h).
 *
 * A .gdi lists the tracks: number, start LBA, type (4 = data, 0 = audio), sector size, file name
 * (quoted if it has spaces), offset. The disc's single-density area (tracks 1-2) only holds a
 * warning; the game is in the high-density area from LBA 45000: its first data track starts
 * with the ISO 9660 file system (volume descriptor at +16) whose extents are absolute LBAs that
 * can reach into later data tracks. Data sectors are 2048 bytes of user data, at offset 16 in
 * 2352-byte (mode 1) sectors. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>

#include "pc/gdrom.h"
#include "pc/platform.h"
#include "pc_runtime.h"

namespace pc {

void GdRom::Close()
{
    for (int i = 0; i < m_ntracks; i++)
        if (m_tracks[i].f)
            fclose(m_tracks[i].f);
    m_ntracks = 0;
}

bool GdRom::ReadSector(uint32_t lba, uint8_t *dst)
{
    for (int i = 0; i < m_ntracks; i++) {
        Track *t = &m_tracks[i];
        if (!t->data || lba < t->lba || lba >= t->lba + t->count)
            continue;
        PC_FSEEK(t->f, (long long)(lba - t->lba) * t->ssize + (t->ssize == 2352 ? 16 : 0),
                  SEEK_SET);
        return fread(dst, 1, 2048, t->f) == 2048;
    }
    return false;
}

bool GdRom::Read(uint32_t lba, uint32_t size, void *dst)
{
    uint8_t sec[2048], *o = (uint8_t *)dst;
    for (uint32_t done = 0; done < size; done += 2048, lba++) {
        uint32_t n = size - done < 2048 ? size - done : 2048;
        if (n == 2048) {
            if (!ReadSector(lba, o + done))
                return false;
        } else {
            if (!ReadSector(lba, sec))
                return false;
            memcpy(o + done, sec, n);
        }
    }
    return true;
}

bool GdRom::Open(const char *gdi)
{
    char line[1024], dir[1024], path[1200];
    FILE *f = fopen(gdi, "r");
    Close();
    if (!f)
        return false;
    snprintf(dir, sizeof dir, "%s", gdi);
    {
        char *s1 = strrchr(dir, '\\'), *s2 = strrchr(dir, '/');
        char *s = s1 > s2 ? s1 : s2;
        if (s)
            s[1] = 0;
        else
            dir[0] = 0;
    }
    if (!fgets(line, sizeof line, f)) {
        fclose(f);
        return false;
    }
    while (m_ntracks < kMaxTracks && fgets(line, sizeof line, f)) {
        unsigned num, lba, type, ssize;
        int pos;
        char name[512], *p;
        if (sscanf(line, "%u %u %u %u %n", &num, &lba, &type, &ssize, &pos) != 4)
            continue;
        p = line + pos;
        if (*p == '"') {
            char *e = strchr(p + 1, '"');
            if (!e)
                continue;
            snprintf(name, sizeof name, "%.*s", (int)(e - p - 1), p + 1);
        } else if (sscanf(p, "%511s", name) != 1) {
            continue;
        }
        Track *t = &m_tracks[m_ntracks];
        snprintf(path, sizeof path, "%s%s", dir, name);
        t->f = fopen(path, "rb");
        if (!t->f) {
            PC_Log("dc: %s: track %u (%s) is missing", gdi, num, path);
            fclose(f);
            Close();
            return false;
        }
        PC_FSEEK(t->f, 0, SEEK_END);
        t->lba = lba;
        t->ssize = ssize;
        t->data = type == 4;
        t->count = (uint32_t)(PC_FTELL(t->f) / ssize);
        m_ntracks++;
    }
    fclose(f);
    for (int i = 0; i < m_ntracks; i++) {
        uint8_t pvd[2048];
        if (!m_tracks[i].data || m_tracks[i].lba < kHdArea)
            continue;
        if (ReadSector(m_tracks[i].lba + 16, pvd) && !memcmp(pvd + 1, "CD001", 5)) {
            memcpy(&m_rootLba, pvd + 156 + 2, 4);
            memcpy(&m_rootSize, pvd + 156 + 10, 4);
            return true;
        }
        break;
    }
    PC_Log("dc: %s has no file system in its high-density area", gdi);
    Close();
    return false;
}

void GdRom::Walk(uint32_t lba, uint32_t size, const char *prefix, FileFn fn, void *ctx, int depth)
{
    if (depth > 8 || size > (16u << 20))
        return;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[size + 2048]);
    uint8_t *d = buf.get();
    if (!Read(lba, size, d))
        return;
    for (uint32_t p = 0; p < size;) {
        uint32_t rl = d[p], ext, len;
        char name[256], full[512];
        if (!rl) {
            p = (p / 2048 + 1) * 2048;   /* records don't cross sectors */
            continue;
        }
        if (p + 33 > size || p + rl > size)
            break;
        memcpy(&ext, d + p + 2, 4);
        memcpy(&len, d + p + 10, 4);
        int flags = d[p + 25], nl = d[p + 32];
        if (nl == 1 && (d[p + 33] == 0 || d[p + 33] == 1)) {
            p += rl;
            continue;   /* . and .. */
        }
        snprintf(name, sizeof name, "%.*s", nl, (const char *)d + p + 33);
        char *semi = strchr(name, ';');
        if (semi)
            *semi = 0;
        snprintf(full, sizeof full, "%s%s", prefix, name);
        if (flags & 2) {
            strcat(full, "/");
            Walk(ext, len, full, fn, ctx, depth + 1);
        } else {
            fn(full, ext, len, ctx);
        }
        p += rl;
    }
}

void GdRom::List(FileFn fn, void *ctx)
{
    if (m_ntracks)
        Walk(m_rootLba, m_rootSize, "", fn, ctx, 0);
}

} // namespace pc

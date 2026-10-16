/* Saves: a file per career and per replay, see pc_saves.h. Saved Games\Neversoft\Tony Hawk's Pro
 * Skater\Saves (THPS_SAVE_DIR overrides it) has a folder per type. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "pc_runtime.h"
#include "pc_saves.h"
#include "pc/saves.h"
#include "pc/platform.h"
#include "pc/runtime.h"

namespace pc {

Saves g_saves;

namespace {

constexpr char kMagic[] = "THPS";
constexpr int kVersion = 1;

/* the game's directory table: 15 libapi DIRENTRYs */
constexpr int kDirEntries = 15;
constexpr int kDirEntrySize = 40;
constexpr int kOtherFile = 3;

/* PS1 saves: the card header Card_Write writes in front of each buffer's data, and the data */
constexpr int kCardHead = 512;
constexpr int kCareerFile = 3200;
constexpr int kReplayFile = 24576;
constexpr int kMcsHeader = 128;

struct Header {
    char magic[4];
    uint16_t version;
    uint16_t type;
    uint32_t size;
    uint32_t reserved;
};
static_assert(sizeof(Header) == 16, "the save file header is 16 bytes");

const char *const s_typeDir[2] = {"Careers", "Replays"};
const char *const s_typeExt[2] = {".career", ".replay"};

bool ValidType(int type) { return type == PC_SAVE_CAREER || type == PC_SAVE_REPLAY; }

/* A save name: at most 10 characters, trailing blanks dropped. 0 if nothing is left. */
bool CleanName(const char *name, char *out) {
    int n = 0;
    while (n < PC_SAVE_NAME_LEN && name[n])
        n++;
    while (n > 0 && name[n - 1] == ' ')
        n--;
    memcpy(out, name, (size_t)n);
    out[n] = 0;
    return n > 0;
}

/* Names Windows keeps for devices. The name entry only has letters and spaces, so of the
 * reserved names only these four can be typed. */
bool Reserved(const char *stem) {
    static const char *const names[] = {"CON", "PRN", "AUX", "NUL"};
    for (const char *name : names)
        if (!_stricmp(stem, name))
            return true;
    return false;
}

bool WriteSaveFile(const char *path, int type, int n, const PCSaveChunk *chunks) {
    char tmp[MAX_PATH];
    Header h;
    FILE *f;
    int i, ok;

    memset(&h, 0, sizeof h);
    memcpy(h.magic, kMagic, 4);
    h.version = kVersion;
    h.type = (uint16_t)type;
    for (i = 0; i < n; i++)
        h.size += chunks[i].n;

    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "wb");
    if (!f) {
        PC_Log("saves: cannot write %s", tmp);
        return false;
    }
    ok = fwrite(&h, 1, sizeof h, f) == sizeof h;
    for (i = 0; ok && i < n; i++)
        ok = fwrite(chunks[i].p, 1, chunks[i].n, f) == chunks[i].n;
    ok &= fclose(f) == 0;
    if (ok)
        ok = os::RenameOver(tmp, path);
    if (!ok) {
        PC_Log("saves: writing %s failed", path);
        os::RemoveFile(tmp);
    }
    return ok;
}

/* The header of a save file of `type`, or 0. */
FILE *OpenSave(const char *path, int type, Header *h) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return nullptr;
    if (fread(h, 1, sizeof *h, f) != sizeof *h || memcmp(h->magic, kMagic, 4) || h->version != kVersion ||
        h->type != type) {
        fclose(f);
        return nullptr;
    }
    return f;
}

/* The name in a PS1 save's Shift-JIS title (as Front_ExtractASCIINameFromSJIS reads it). */
void NameFromTitle(const uint8_t *s, char *name) {
    int i, n = 0;
    for (i = 0; i < PC_SAVE_NAME_LEN && s[0]; i++, s += 2) {
        if (s[0] == 0x82)
            name[n++] = (char)(s[1] - 31);
        else if (s[0] == 0x81)
            name[n++] = ' ';
        else
            break;
    }
    name[n] = 0;
}

struct Found {
    char name[PC_SAVE_NAME_LEN + 1];
    long long written;
};

int NewestFirst(const void *a, const void *b) {
    const Found *x = (const Found *)a, *y = (const Found *)b;
    int c = (y->written > x->written) - (y->written < x->written);
    return c ? c : strcmp(x->name, y->name);
}

}   // namespace

const char *Saves::RootDir() {
    char *dir = m_rootDir;
    if (!dir[0]) {
        if (getenv("THPS_SAVE_DIR"))
            snprintf(dir, MAX_PATH, "%s", getenv("THPS_SAVE_DIR"));
        else
            snprintf(dir, MAX_PATH, "%s" PC_SEP "Saves", g_runtime.UserDir());
        os::PathFix(dir);
        os::MakeDirs(dir);
        PC_Log("saves: %s", dir);
    }
    return dir;
}

const char *Saves::TypeDir(int type) {
    char *dir = m_typeDirs[type];
    if (!dir[0]) {
        snprintf(dir, MAX_PATH, "%s" PC_SEP "%s", RootDir(), s_typeDir[type]);
        os::MakeDirs(dir);
    }
    return dir;
}

/* The file for a save: its name, with characters Windows rejects replaced and a '_' after a
 * reserved device name. */
bool Saves::SavePath(int type, const char *name, char *out, int n) {
    char clean[PC_SAVE_NAME_LEN + 1];
    int i;
    if (!ValidType(type) || !CleanName(name, clean))
        return false;
    for (i = 0; clean[i]; i++)
        if (strchr("\\/:*?\"<>|.", clean[i]) || (unsigned char)clean[i] < 32)
            clean[i] = '_';
    snprintf(out, n, "%s" PC_SEP "%s%s%s", TypeDir(type), clean, Reserved(clean) ? "_" : "", s_typeExt[type]);
    return true;
}

bool Saves::CareerModsPath(const char *name, char *out, int n) {
    if (!SavePath(PC_SAVE_CAREER, name, out, n))
        return false;
    size_t len = strlen(out), ext = strlen(s_typeExt[PC_SAVE_CAREER]);
    snprintf(out + len - ext, n - (len - ext), ".mods.ini");
    return true;
}

/* ---- PS1 saves ----------------------------------------------------------------------------- */

/* One .mcs file: a 128-byte card directory frame, then what Card_Write wrote. */
void Saves::ImportMcs(const char *path) {
    char name[PC_SAVE_NAME_LEN + 1], dest[MAX_PATH], done[MAX_PATH];
    PCSaveChunk chunks[2];
    std::vector<uint8_t> data;
    uint8_t *d;
    long size;
    int type, n = 1;
    FILE *f = fopen(path, "rb");
    if (!f)
        return;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size > kMcsHeader)
        data.resize((size_t)size);
    d = data.data();
    if (data.empty() || fread(d, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        return;
    }
    fclose(f);

    if ((d[0] & 0xf0) != 0x50 || memcmp(d + 10, "BASLUS-00860THPS", 16))
        return;
    type = d[26] == 'G' ? PC_SAVE_CAREER : d[26] == 'V' ? PC_SAVE_REPLAY : -1;
    size -= kMcsHeader;
    if (type < 0 || size < (type == PC_SAVE_CAREER ? kCareerFile : kReplayFile)) {
        PC_Log("saves: %s isn't a THPS save", path);
        return;
    }
    NameFromTitle(d + kMcsHeader + (type == PC_SAVE_CAREER ? 32 : 30), name);
    if (!CleanName(name, name))
        strcpy(name, type == PC_SAVE_CAREER ? "CAREER" : "VIDEO");
    chunks[0].p = d + kMcsHeader + kCardHead;
    chunks[0].n = (type == PC_SAVE_CAREER ? kCareerFile : kReplayFile) - kCardHead;
    if (type == PC_SAVE_REPLAY && size >= 2 * kReplayFile) {
        chunks[1].p = d + kMcsHeader + kReplayFile + kCardHead;
        chunks[1].n = kReplayFile - kCardHead;
        n = 2;
    }

    SavePath(type, name, dest, sizeof dest);
    if (os::Exists(dest))
        PC_Log("saves: %s not imported: %s already exists", path, dest);
    else if (WriteSaveFile(dest, type, n, chunks)) {
        snprintf(done, sizeof done, "%s.imported", path);
        os::RenameOver(path, done);
        PC_Log("saves: imported %s as %s", path, dest);
    }
}

void Saves::ImportPs1Saves() {
    char path[MAX_PATH];
    os::DirEntry fd;
    if (m_imported)
        return;
    m_imported = true;
    os::DirList list(RootDir());
    if (!list)
        return;
    while (list.Next(&fd)) {
        size_t len = strlen(fd.name);
        if (fd.isDir || len < 4 || _stricmp(fd.name + len - 4, ".mcs"))
            continue;
        snprintf(path, sizeof path, "%s" PC_SEP "%s", RootDir(), fd.name);
        ImportMcs(path);
    }
}

/* ---- the game's card layer ----------------------------------------------------------------- */

int Saves::ListDir(int type, void *slotDir, int32_t *fileTypes) {
    char path[MAX_PATH];
    os::DirEntry fd;
    std::vector<Found> found;
    int n, i;

    for (i = 0; i < kDirEntries; i++)
        fileTypes[i] = kOtherFile;
    memset(slotDir, 0, kDirEntries * kDirEntrySize);
    if (!ValidType(type))
        return 0;
    m_listType = type;
    ImportPs1Saves();

    os::DirList list(TypeDir(type));
    if (list) {
        while (list.Next(&fd)) {
            char stem[MAX_PATH];
            size_t len = strlen(fd.name), ext = strlen(s_typeExt[type]);
            Header hd;
            FILE *f;
            if (fd.isDir || len <= ext || _stricmp(fd.name + len - ext, s_typeExt[type]))
                continue;
            len -= ext;
            memcpy(stem, fd.name, len);
            stem[len] = 0;
            if (len > 1 && stem[len - 1] == '_') {
                stem[len - 1] = 0;
                if (Reserved(stem))
                    len--;
                else
                    stem[len - 1] = '_';
            }
            stem[len] = 0;
            if (len == 0 || len > PC_SAVE_NAME_LEN) {
                PC_Trace("saves: %s left out (name longer than %d)", fd.name, PC_SAVE_NAME_LEN);
                continue;
            }
            snprintf(path, sizeof path, "%s" PC_SEP "%s", TypeDir(type), fd.name);
            f = OpenSave(path, type, &hd);
            if (!f) {
                PC_Log("saves: %s isn't a save", path);
                continue;
            }
            fclose(f);
            Found e;
            strcpy(e.name, stem);
            e.written = fd.mtime;
            found.push_back(e);
        }
    }
    n = (int)found.size();
    qsort(found.data(), (size_t)n, sizeof(Found), NewestFirst);
    if (n > kDirEntries) {
        PC_Log("saves: %d %s, the game lists the newest %d", n, s_typeDir[type], kDirEntries);
        n = kDirEntries;
    }
    for (i = 0; i < n; i++) {
        strcpy((char *)slotDir + i * kDirEntrySize, found[i].name);
        fileTypes[i] = type;
    }
    return n;
}

void Saves::Select(const char *name) { CleanName(name, m_selected); }

void Saves::CardHeader(void *head, int type, const char *name) {
    uint8_t *s = (uint8_t *)head + (type == PC_SAVE_CAREER ? 32 : 30);
    int i;
    memset(head, 0, kCardHead);
    for (i = 0; i < PC_SAVE_NAME_LEN && name[i]; i++) {
        if ((unsigned char)(name[i] - 'A') < 26) {
            *s++ = 0x82;
            *s++ = (uint8_t)(name[i] + 31);
        } else {
            *s++ = 0x81;
            *s++ = 0x40;
        }
    }
}

bool Saves::Exists(int type, const char *name) {
    char path[MAX_PATH];
    return SavePath(type, name, path, sizeof path) && os::Exists(path);
}

bool Saves::Write(int type, const char *name, int n, const PCSaveChunk *chunks) {
    char path[MAX_PATH];
    if (!SavePath(type, name, path, sizeof path))
        return false;
    if (!WriteSaveFile(path, type, n, chunks))
        return false;
    PC_Log("saves: saved %s", path);
    return true;
}

bool Saves::Read(int type, const char *name, int n, const PCSaveChunk *chunks) {
    char path[MAX_PATH];
    Header h;
    uint32_t need = 0;
    FILE *f;
    int i;
    bool ok = true;
    if (!SavePath(type, name, path, sizeof path))
        return false;
    f = OpenSave(path, type, &h);
    if (!f) {
        PC_Log("saves: cannot read %s", path);
        return false;
    }
    for (i = 0; i < n; i++)
        need += chunks[i].n;
    if (h.size < need)
        ok = false;
    for (i = 0; ok && i < n; i++)
        ok = fread(chunks[i].p, 1, chunks[i].n, f) == chunks[i].n;
    fclose(f);
    if (!ok)
        PC_Log("saves: %s is too short", path);
    return ok;
}

}   // namespace pc

/* ---- the game's interface (pc_saves.h) ---- */

int PC_SaveListDir(int type, void *slotDir, int32_t *fileTypes) { return pc::g_saves.ListDir(type, slotDir, fileTypes); }
int PC_SaveListType(void) { return pc::g_saves.ListType(); }
void PC_SaveSelect(const char *name) { pc::g_saves.Select(name); }
const char *PC_SaveSelected(void) { return pc::g_saves.Selected(); }
void PC_SaveCardHeader(void *head, int type, const char *name) { pc::Saves::CardHeader(head, type, name); }
int PC_SaveExists(int type, const char *name) { return pc::g_saves.Exists(type, name); }
int PC_SaveWrite(int type, const char *name, int n, const PCSaveChunk *chunks) { return pc::g_saves.Write(type, name, n, chunks); }
int PC_SaveRead(int type, const char *name, int n, const PCSaveChunk *chunks) { return pc::g_saves.Read(type, name, n, chunks); }

/* Saves: a file per career and per replay, see pc_saves.h. Saved Games\Neversoft\Tony Hawk's Pro
 * Skater\Saves (THPS_SAVE_DIR overrides it) has a folder per type. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_platform.h"
#include "pc_runtime.h"
#include "pc_saves.h"

#define MAGIC "THPS"
#define VERSION 1
#define HEADER_SIZE 16

/* the game's directory table: 15 libapi DIRENTRYs */
#define DIR_ENTRIES 15
#define DIR_ENTRY_SIZE 40
#define OTHER_FILE 3

/* PS1 saves: the card header Card_Write writes in front of each buffer's data, and the data */
#define CARD_HEAD 512
#define CAREER_FILE 3200
#define REPLAY_FILE 24576
#define MCS_HEADER 128

typedef struct {
    char magic[4];
    uint16_t version;
    uint16_t type;
    uint32_t size;
    uint32_t reserved;
} Header;

static const char *const s_typeDir[2] = {"Careers", "Replays"};
static const char *const s_typeExt[2] = {".career", ".replay"};

static int s_listType;
static char s_selected[PC_SAVE_NAME_LEN + 1];

static int ValidType(int type) { return type == PC_SAVE_CAREER || type == PC_SAVE_REPLAY; }

#define MakeDirs PC_MakeDirs

static const char *RootDir(void) {
    static char dir[MAX_PATH];
    if (!dir[0]) {
        if (getenv("THPS_SAVE_DIR"))
            snprintf(dir, sizeof dir, "%s", getenv("THPS_SAVE_DIR"));
        else
            snprintf(dir, sizeof dir, "%s" PC_SEP "Saves", PC_UserDir());
        PC_PathFix(dir);
        MakeDirs(dir);
        PC_Log("saves: %s", dir);
    }
    return dir;
}

static const char *TypeDir(int type) {
    static char dirs[2][MAX_PATH];
    char *dir = dirs[type];
    if (!dir[0]) {
        snprintf(dir, MAX_PATH, "%s" PC_SEP "%s", RootDir(), s_typeDir[type]);
        MakeDirs(dir);
    }
    return dir;
}

/* A save name: at most 10 characters, trailing blanks dropped. 0 if nothing is left. */
static int CleanName(const char *name, char *out) {
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
static int Reserved(const char *stem) {
    static const char *const names[] = {"CON", "PRN", "AUX", "NUL"};
    int i;
    for (i = 0; i < 4; i++)
        if (!_stricmp(stem, names[i]))
            return 1;
    return 0;
}

/* The file for a save: its name, with characters Windows rejects replaced and a '_' after a
 * reserved device name. */
static int SavePath(int type, const char *name, char *out, int n) {
    char clean[PC_SAVE_NAME_LEN + 1];
    int i;
    if (!ValidType(type) || !CleanName(name, clean))
        return 0;
    for (i = 0; clean[i]; i++)
        if (strchr("\\/:*?\"<>|.", clean[i]) || (unsigned char)clean[i] < 32)
            clean[i] = '_';
    snprintf(out, n, "%s" PC_SEP "%s%s%s", TypeDir(type), clean, Reserved(clean) ? "_" : "", s_typeExt[type]);
    return 1;
}

static int WriteFile_(const char *path, int type, int n, const PCSaveChunk *chunks) {
    char tmp[MAX_PATH];
    Header h;
    FILE *f;
    int i, ok;

    memset(&h, 0, sizeof h);
    memcpy(h.magic, MAGIC, 4);
    h.version = VERSION;
    h.type = (uint16_t)type;
    for (i = 0; i < n; i++)
        h.size += chunks[i].n;

    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "wb");
    if (!f) {
        PC_Log("saves: cannot write %s", tmp);
        return 0;
    }
    ok = fwrite(&h, 1, sizeof h, f) == sizeof h;
    for (i = 0; ok && i < n; i++)
        ok = fwrite(chunks[i].p, 1, chunks[i].n, f) == chunks[i].n;
    ok &= fclose(f) == 0;
    if (ok)
        ok = PC_ReplaceFile(tmp, path);
    if (!ok) {
        PC_Log("saves: writing %s failed", path);
        PC_DeleteFile(tmp);
    }
    return ok;
}

/* The header of a save file of `type`, or 0. */
static FILE *OpenSave(const char *path, int type, Header *h) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    if (fread(h, 1, sizeof *h, f) != sizeof *h || memcmp(h->magic, MAGIC, 4) || h->version != VERSION ||
        h->type != type) {
        fclose(f);
        return NULL;
    }
    return f;
}

/* ---- PS1 saves ----------------------------------------------------------------------------- */

/* The name in a PS1 save's Shift-JIS title (as Front_ExtractASCIINameFromSJIS reads it). */
static void NameFromTitle(const uint8_t *s, char *name) {
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

/* One .mcs file: a 128-byte card directory frame, then what Card_Write wrote. */
static void ImportMcs(const char *path) {
    char name[PC_SAVE_NAME_LEN + 1], dest[MAX_PATH], done[MAX_PATH];
    PCSaveChunk chunks[2];
    uint8_t *d;
    long size;
    int type, n = 1;
    FILE *f = fopen(path, "rb");
    if (!f)
        return;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    d = size > MCS_HEADER ? malloc((size_t)size) : NULL;
    if (!d || fread(d, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        free(d);
        return;
    }
    fclose(f);

    if ((d[0] & 0xf0) != 0x50 || memcmp(d + 10, "BASLUS-00860THPS", 16)) {
        free(d);
        return;
    }
    type = d[26] == 'G' ? PC_SAVE_CAREER : d[26] == 'V' ? PC_SAVE_REPLAY : -1;
    size -= MCS_HEADER;
    if (type < 0 || size < (type == PC_SAVE_CAREER ? CAREER_FILE : REPLAY_FILE)) {
        PC_Log("saves: %s isn't a THPS save", path);
        free(d);
        return;
    }
    NameFromTitle(d + MCS_HEADER + (type == PC_SAVE_CAREER ? 32 : 30), name);
    if (!CleanName(name, name))
        strcpy(name, type == PC_SAVE_CAREER ? "CAREER" : "VIDEO");
    chunks[0].p = d + MCS_HEADER + CARD_HEAD;
    chunks[0].n = (type == PC_SAVE_CAREER ? CAREER_FILE : REPLAY_FILE) - CARD_HEAD;
    if (type == PC_SAVE_REPLAY && size >= 2 * REPLAY_FILE) {
        chunks[1].p = d + MCS_HEADER + REPLAY_FILE + CARD_HEAD;
        chunks[1].n = REPLAY_FILE - CARD_HEAD;
        n = 2;
    }

    SavePath(type, name, dest, sizeof dest);
    if (PC_PathExists(dest))
        PC_Log("saves: %s not imported: %s already exists", path, dest);
    else if (WriteFile_(dest, type, n, chunks)) {
        snprintf(done, sizeof done, "%s.imported", path);
        PC_ReplaceFile(path, done);
        PC_Log("saves: imported %s as %s", path, dest);
    }
    free(d);
}

static void ImportPs1Saves(void) {
    static int done;
    char path[MAX_PATH];
    PcDirEntry fd;
    PcDir *h;
    if (done)
        return;
    done = 1;
    h = PC_DirOpen(RootDir());
    if (!h)
        return;
    while (PC_DirNext(h, &fd)) {
        size_t len = strlen(fd.name);
        if (fd.isDir || len < 4 || _stricmp(fd.name + len - 4, ".mcs"))
            continue;
        snprintf(path, sizeof path, "%s" PC_SEP "%s", RootDir(), fd.name);
        ImportMcs(path);
    }
    PC_DirClose(h);
}

/* ---- the game's card layer ----------------------------------------------------------------- */

typedef struct {
    char name[PC_SAVE_NAME_LEN + 1];
    long long written;
} Found;

static int NewestFirst(const void *a, const void *b) {
    const Found *x = a, *y = b;
    int c = (y->written > x->written) - (y->written < x->written);
    return c ? c : strcmp(x->name, y->name);
}

int PC_SaveListDir(int type, void *slotDir, int32_t *fileTypes) {
    char path[MAX_PATH];
    PcDirEntry fd;
    PcDir *h;
    Found *found = NULL;
    int n = 0, cap = 0, i;

    for (i = 0; i < DIR_ENTRIES; i++)
        fileTypes[i] = OTHER_FILE;
    memset(slotDir, 0, DIR_ENTRIES * DIR_ENTRY_SIZE);
    if (!ValidType(type))
        return 0;
    s_listType = type;
    ImportPs1Saves();

    h = PC_DirOpen(TypeDir(type));
    if (h) {
        while (PC_DirNext(h, &fd)) {
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
            if (n == cap) {
                cap = cap ? cap * 2 : 16;
                found = realloc(found, (size_t)cap * sizeof *found);
            }
            strcpy(found[n].name, stem);
            found[n].written = fd.mtime;
            n++;
        }
        PC_DirClose(h);
    }
    qsort(found, (size_t)n, sizeof *found, NewestFirst);
    if (n > DIR_ENTRIES) {
        PC_Log("saves: %d %s, the game lists the newest %d", n, s_typeDir[type], DIR_ENTRIES);
        n = DIR_ENTRIES;
    }
    for (i = 0; i < n; i++) {
        strcpy((char *)slotDir + i * DIR_ENTRY_SIZE, found[i].name);
        fileTypes[i] = type;
    }
    free(found);
    return n;
}

int PC_SaveListType(void) { return s_listType; }

void PC_SaveSelect(const char *name) { CleanName(name, s_selected); }
const char *PC_SaveSelected(void) { return s_selected; }

void PC_SaveCardHeader(void *head, int type, const char *name) {
    uint8_t *s = (uint8_t *)head + (type == PC_SAVE_CAREER ? 32 : 30);
    int i;
    memset(head, 0, CARD_HEAD);
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

int PC_SaveExists(int type, const char *name) {
    char path[MAX_PATH];
    return SavePath(type, name, path, sizeof path) && PC_PathExists(path);
}

int PC_SaveWrite(int type, const char *name, int n, const PCSaveChunk *chunks) {
    char path[MAX_PATH];
    if (!SavePath(type, name, path, sizeof path))
        return 0;
    if (!WriteFile_(path, type, n, chunks))
        return 0;
    PC_Log("saves: saved %s", path);
    return 1;
}

int PC_SaveRead(int type, const char *name, int n, const PCSaveChunk *chunks) {
    char path[MAX_PATH];
    Header h;
    uint32_t need = 0;
    FILE *f;
    int i, ok = 1;
    if (!SavePath(type, name, path, sizeof path))
        return 0;
    f = OpenSave(path, type, &h);
    if (!f) {
        PC_Log("saves: cannot read %s", path);
        return 0;
    }
    for (i = 0; i < n; i++)
        need += chunks[i].n;
    if (h.size < need)
        ok = 0;
    for (i = 0; ok && i < n; i++)
        ok = fread(chunks[i].p, 1, chunks[i].n, f) == chunks[i].n;
    fclose(f);
    if (!ok)
        PC_Log("saves: %s is too short", path);
    return ok;
}

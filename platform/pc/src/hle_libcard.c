/* The memory card, as a folder of save files: %USERPROFILE%\Neversoft\Tony Hawk's Pro Skater\Saves
 * (THPS_SAVE_DIR overrides it).
 *
 * The game's card code (decomp/src/card) is unchanged. It talks to the card through two layers,
 * both answered here:
 * - libcard + card events: _card_info / _card_load report the card in slot 1 as present,
 *   formatted and unchanged (SwCARD EvSpIOE); slot 2 stays empty (EvSpTIMOUT).
 * - the BIOS file API on "bu00:": open/read/write/close/erase/firstfile/nextfile/format. Each
 *   card file is one file in the folder.
 *
 * Every file is stored as a single-save .mcs (a 128-byte card directory frame, then the file's
 * blocks), the format MemcardRex and the emulators import and export, so saves can move between
 * the port, emulators and real cards. The card name ("BASLUS-00860THPSG..") is the one in the
 * header; the file on disk is named after it. A file is read whole on open and, if written,
 * rewritten through a temporary file on close.
 *
 * Like a real card it holds 15 blocks of 8 KB: the game's directory table has 15 entries and it
 * counts free blocks for its "not enough space" checks. Files that don't fit are left alone. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_hle.h"
#include "pc_runtime.h"

#define HwCARD 0xf0000011u
#define SwCARD 0xf4000001u
#define EvSpIOE 0x0004
#define EvSpTIMOUT 0x0100

#define BLOCK_SIZE 8192
#define CARD_BLOCKS 15
#define NAME_LEN 20
#define MCS_HEADER 128

/* BIOS open() flags */
#define FREAD 0x0001
#define FWRITE 0x0002
#define FCREAT 0x0200

/* libapi's struct DIRENTRY, as the game's SLOT_DIR table holds it */
typedef struct {
    char name[NAME_LEN];
    int32_t attr;
    int32_t size;
    uint32_t next;
    int32_t head;
    char system[4];
} DirEntry;

typedef struct {
    char name[NAME_LEN];
    char path[MAX_PATH];
    uint32_t size; /* bytes, a multiple of BLOCK_SIZE */
} CardFile;

static CardFile s_files[CARD_BLOCKS];
static int s_numFiles;

#define MAX_FDS 4
typedef struct {
    int used, dirty, mode;
    char name[NAME_LEN];
    char path[MAX_PATH];
    uint8_t *data;
    uint32_t size, pos;
} Fd;
static Fd s_fds[MAX_FDS];

/* firstfile/nextfile iteration */
static char s_findPattern[NAME_LEN + 1];
static int s_findNext;

/* ---- the save folder ---------------------------------------------------------------------- */

static const char *SaveDir(void) {
    static char dir[MAX_PATH];
    if (!dir[0]) {
        char *s;
        if (getenv("THPS_SAVE_DIR"))
            snprintf(dir, sizeof dir, "%s", getenv("THPS_SAVE_DIR"));
        else
            snprintf(dir, sizeof dir, "%s\\Saves", PC_UserDir());
        for (s = dir; *s; s++)
            if (*s == '/')
                *s = '\\';
        for (s = strchr(dir + 3, '\\'); s; s = strchr(s + 1, '\\')) {
            *s = 0;
            CreateDirectoryA(dir, NULL);
            *s = '\\';
        }
        CreateDirectoryA(dir, NULL);
        PC_Log("memory card: %s", dir);
    }
    return dir;
}

static uint8_t McsChecksum(const uint8_t *h) {
    uint8_t x = 0;
    int i;
    for (i = 0; i < MCS_HEADER - 1; i++)
        x ^= h[i];
    return x;
}

/* Read a .mcs file's header: its card name and data size. */
static int ReadMcsHeader(const char *path, char *name, uint32_t *size) {
    uint8_t h[MCS_HEADER];
    uint32_t sz;
    long long fileSize;
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fread(h, 1, MCS_HEADER, f) != MCS_HEADER || (h[0] & 0xf0) != 0x50) {
        fclose(f);
        return 0;
    }
    fclose(f);
    memcpy(&sz, h + 4, 4);
    if (!sz || sz % BLOCK_SIZE)
        sz = (uint32_t)((fileSize - MCS_HEADER + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE);
    if (!sz)
        return 0;
    memcpy(name, h + 10, NAME_LEN);
    name[NAME_LEN - 1] = 0;
    *size = sz;
    return name[0] != 0;
}

typedef struct {
    CardFile f;
    FILETIME created;
} Found;

static int CompareFound(const void *a, const void *b) {
    const Found *x = a, *y = b;
    int c = CompareFileTime(&x->created, &y->created);
    return c ? c : strcmp(x->f.name, y->f.name);
}

/* The card's directory: the .mcs files in the folder, oldest first (a real card lists them in
 * the order they were made), as many as fit in 15 blocks. */
static void ScanCard(void) {
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    Found found[64];
    int n = 0, i, blocks = 0;

    snprintf(pattern, sizeof pattern, "%s\\*.mcs", SaveDir());
    h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            Found *e = &found[n];
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            snprintf(e->f.path, sizeof e->f.path, "%s\\%s", SaveDir(), fd.cFileName);
            if (!ReadMcsHeader(e->f.path, e->f.name, &e->f.size))
                continue;
            e->created = fd.ftCreationTime;
            n++;
        } while (n < 64 && FindNextFileA(h, &fd));
        FindClose(h);
    }
    qsort(found, n, sizeof found[0], CompareFound);

    s_numFiles = 0;
    for (i = 0; i < n; i++) {
        int j, dup = 0;
        uint32_t b = found[i].f.size / BLOCK_SIZE;
        for (j = 0; j < s_numFiles; j++)
            dup |= !strcmp(s_files[j].name, found[i].f.name);
        if (dup || blocks + b > CARD_BLOCKS) {
            PC_Trace("memory card: %s left out (%s)", found[i].f.path, dup ? "same name" : "card full");
            continue;
        }
        blocks += b;
        s_files[s_numFiles++] = found[i].f;
    }
}

static int UsedBlocks(void) {
    int i, b = 0;
    for (i = 0; i < s_numFiles; i++)
        b += s_files[i].size / BLOCK_SIZE;
    return b;
}

static CardFile *FindFile(const char *name) {
    int i;
    for (i = 0; i < s_numFiles; i++)
        if (!strcmp(s_files[i].name, name))
            return &s_files[i];
    return NULL;
}

/* "bu00:NAME" -> "NAME" for the card in slot 1; NULL for any other device (slot 2 is empty). */
static const char *CardName(const char *path) {
    if (!path || strncmp(path, "bu", 2) != 0 || path[4] != ':')
        return NULL;
    if (path[2] != '0' || path[3] != '0')
        return NULL;
    return path + 5;
}

/* A file name on disk for a card name: the name itself, minus characters Windows rejects. */
static void DiskPath(const char *name, char *out, int n) {
    char safe[NAME_LEN];
    int i;
    for (i = 0; name[i] && i < NAME_LEN - 1; i++)
        safe[i] = strchr("\\/:*?\"<>|", name[i]) || (unsigned char)name[i] < 32 ? '_' : name[i];
    safe[i] = 0;
    snprintf(out, n, "%s\\%s.mcs", SaveDir(), safe);
}

static int SaveFile(const char *path, const char *name, const uint8_t *data, uint32_t size) {
    char tmp[MAX_PATH];
    uint8_t h[MCS_HEADER];
    uint16_t next = 0xffff;
    FILE *f;
    int ok;

    memset(h, 0, sizeof h);
    h[0] = 0x51; /* in use, first block */
    memcpy(h + 4, &size, 4);
    memcpy(h + 8, &next, 2);
    strncpy((char *)h + 10, name, NAME_LEN - 1);
    h[MCS_HEADER - 1] = McsChecksum(h);

    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "wb");
    if (!f) {
        PC_Log("memory card: cannot write %s", tmp);
        return 0;
    }
    ok = fwrite(h, 1, sizeof h, f) == sizeof h && fwrite(data, 1, size, f) == size;
    ok &= fclose(f) == 0;
    if (ok)
        ok = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok) {
        PC_Log("memory card: writing %s failed", path);
        DeleteFileA(tmp);
    }
    return ok;
}

static uint8_t *LoadFile(const char *path, uint32_t size) {
    uint8_t *d = calloc(1, size);
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, MCS_HEADER, SEEK_SET);
        if (fread(d, 1, size, f) == 0)
            PC_Log("memory card: %s has no data", path);
        fclose(f);
    }
    return d;
}

/* ---- BIOS file API ------------------------------------------------------------------------ */

static Fd *GetFd(int fd) { return fd >= 0 && fd < MAX_FDS && s_fds[fd].used ? &s_fds[fd] : NULL; }

int hle_open(const char *path, int mode) {
    const char *name = CardName(path);
    CardFile *cf;
    int i;
    if (!name || !name[0])
        return -1;
    ScanCard();
    cf = FindFile(name);
    if (mode & FCREAT) {
        int blocks = (mode >> 16) ? (mode >> 16) : 1;
        char diskPath[MAX_PATH];
        uint8_t *zero;
        int ok;
        if (cf || blocks + UsedBlocks() > CARD_BLOCKS || strlen(name) >= NAME_LEN) {
            PC_Trace("memory card: create %s (%d blocks) refused", name, blocks);
            return -1;
        }
        DiskPath(name, diskPath, sizeof diskPath);
        zero = calloc(1, (size_t)blocks * BLOCK_SIZE);
        ok = SaveFile(diskPath, name, zero, (uint32_t)blocks * BLOCK_SIZE);
        free(zero);
        if (!ok)
            return -1;
        PC_Trace("memory card: created %s (%d blocks)", name, blocks);
        ScanCard();
        cf = FindFile(name);
        if (!cf)
            return -1;
    } else if (!cf) {
        return -1;
    }
    for (i = 0; i < MAX_FDS; i++)
        if (!s_fds[i].used) {
            Fd *f = &s_fds[i];
            memset(f, 0, sizeof *f);
            f->used = 1;
            f->mode = mode;
            snprintf(f->name, sizeof f->name, "%s", cf->name);
            snprintf(f->path, sizeof f->path, "%s", cf->path);
            f->size = cf->size;
            f->data = LoadFile(cf->path, cf->size);
            PC_Trace("memory card: open %s mode %x -> %d", name, mode, i);
            return i;
        }
    return -1;
}

int hle_lseek(int fd, int offset, int whence) {
    Fd *f = GetFd(fd);
    int pos;
    if (!f)
        return -1;
    pos = whence == 1 ? (int)f->pos + offset : offset;
    if (pos < 0 || (uint32_t)pos > f->size)
        return -1;
    f->pos = (uint32_t)pos;
    return pos;
}

int hle_read(int fd, void *buf, int n) {
    Fd *f = GetFd(fd);
    if (!f || !(f->mode & FREAD) || n < 0)
        return -1;
    if ((uint32_t)n > f->size - f->pos)
        n = (int)(f->size - f->pos);
    memcpy(buf, f->data + f->pos, (size_t)n);
    f->pos += (uint32_t)n;
    return n;
}

int hle_write(int fd, const void *buf, int n) {
    Fd *f = GetFd(fd);
    if (!f || !(f->mode & FWRITE) || n < 0)
        return -1;
    if ((uint32_t)n > f->size - f->pos)
        n = (int)(f->size - f->pos);
    memcpy(f->data + f->pos, buf, (size_t)n);
    f->pos += (uint32_t)n;
    f->dirty = 1;
    return n;
}

int hle_close(int fd) {
    Fd *f = GetFd(fd);
    int ok = 1;
    if (!f)
        return -1;
    if (f->dirty) {
        ok = SaveFile(f->path, f->name, f->data, f->size);
        PC_Log("memory card: saved %s", f->path);
    }
    free(f->data);
    f->used = 0;
    return ok ? fd : -1;
}

int hle_erase(const char *path) {
    const char *name = CardName(path);
    CardFile *cf;
    if (!name)
        return 0;
    ScanCard();
    cf = FindFile(name);
    if (!cf || !DeleteFileA(cf->path))
        return 0;
    PC_Log("memory card: deleted %s", cf->path);
    return 1;
}

/* The BIOS matches names with '?' for any character and '*' for the rest of the name. */
static int NameMatches(const char *pat, const char *name) {
    for (; *pat; pat++, name++) {
        if (*pat == '*')
            return 1;
        if (!*name || (*pat != '?' && *pat != *name))
            return 0;
    }
    return !*name;
}

static DirEntry *FindFrom(DirEntry *d) {
    while (s_findNext < s_numFiles) {
        CardFile *cf = &s_files[s_findNext++];
        if (NameMatches(s_findPattern, cf->name)) {
            memset(d, 0, sizeof *d);
            memcpy(d->name, cf->name, NAME_LEN);
            d->attr = 0x50;
            d->size = (int32_t)cf->size;
            return d;
        }
    }
    return NULL;
}

void *hle_firstfile(const char *path, void *dir) {
    DirEntry *d = dir;
    const char *name = CardName(path);
    if (!name)
        return NULL;
    ScanCard();
    snprintf(s_findPattern, sizeof s_findPattern, "%s", name[0] ? name : "*");
    s_findNext = 0;
    return FindFrom(d);
}

void *hle_nextfile(void *dir) { return FindFrom(dir); }

/* The card is always formatted; Card_FormatCard then waits for a HwCARD event. */
int hle_format(const char *path) {
    if (!CardName(path))
        return 0;
    PC_DeliverEvent(HwCARD, EvSpIOE);
    return 1;
}

/* ---- libcard ------------------------------------------------------------------------------- */

void InitCARD(int pad) { (void)pad; }
void InitCARD2(int pad) { (void)pad; }
int StartCARD(void) { return 1; }
int StartCARD2(void) { return 1; }
int StopCARD(void) { return 1; }
void hle__bu_init(void) { SaveDir(); }

/* chan = port * 16 + slot: the card is in port 0 slot 0, nothing anywhere else. */
int hle__card_info(int chan) {
    PC_DeliverEvent(SwCARD, chan == 0 ? EvSpIOE : EvSpTIMOUT);
    return 1;
}
int hle__card_load(int chan) { return hle__card_info(chan); }
int hle__card_clear(int chan) {
    PC_DeliverEvent(HwCARD, chan == 0 ? EvSpIOE : EvSpTIMOUT);
    return 1;
}
int hle__new_card(void) { return 1; }
/* Raw sector writes: the game only uses the file API. */
int hle__card_write(int chan, int block, uint32_t buf) {
    (void)block; (void)buf;
    PC_DeliverEvent(SwCARD, chan == 0 ? EvSpIOE : EvSpTIMOUT);
    return 1;
}

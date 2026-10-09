/* First-startup extraction of the game's data from the user's disc into Data/ (see pc_assets.h).
 *
 * Which files exist, and what they are, comes from the game itself:
 * - The retail executable's movie table (movieDetails, 26 STR files) lists every STR on the disc;
 *   the soundtrack entries are the ones with the 96x64 thumbnail video. The table's order is the
 *   order the game cycles the songs in, which becomes the default Data/Music/Playlist.m3u.
 * - Everything else the game loads (levels, models, textures, scripts, sound banks, fonts...) is
 *   packed in CD.WAD, indexed by CD.HED: {name, pad to 4, byte offset, size} entries ending in
 *   a 255 byte. Each file is extracted under its own name, into a folder for its type. The level
 *   codes come from the level scripts (sk<code>_t.trg); sk<code>*.psx are that level's geometry.
 *
 * Data/Manifest.txt lists the extracted CD.WAD files and is written last, so a missing manifest
 * or a missing listed file means extraction has to (re)run, which needs the disc image. Once
 * everything is there the disc image is not opened at all.
 *
 * At runtime the game finds files by name (PC_AssetsFind): every file under Data/ is indexed by
 * its file name, whatever folder it is in. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_assets.h"
#include "pc_hle.h"
#include "pc_runtime.h"

#define MOVIE_DETAILS 0x800a63e8u /* movieDetails[26], 32 bytes each */
#define MOVIE_COUNT 26
#define MUSIC_WIDTH 96            /* soundtrack entries carry a 96x64 video */
#define STREAM_RATE 150           /* sectors/s: the songs stream at 2x */
#define COPY_SECTORS 64
#define MAX_WAD_FILES 1024
#define MAX_LEVELS 32

/* Song credits for the playlist, by file. Order does not come from here. */
static const struct {
    const char *file, *title;
} s_credits[] = {
    {"SUICIDE.STR", "The Suicide Machines - New Girl"},
    {"GOLDFING.STR", "Goldfinger - Superman"},
    {"SPEED.STR", "Speedealer - Screamer/Nothing to Me"},
    {"DK.STR", "Dead Kennedys - Police Truck"},
    {"PRIMUS.STR", "Primus - Jerry Was a Race Car Driver"},
    {"UNSANE.STR", "Unsane - Committed"},
    {"VANDALS.STR", "The Vandals - Euro-Barge"},
    {"ST.STR", "Suicidal Tendencies - Cyco Vision"},
    {"EVENRUDE.STR", "Even Rude - Vilified"},
    {"ERNIES.STR", "The Ernies - Here & Now"},
};

/* CD.WAD folders by file type. */
static const struct {
    const char *ext, *dir;
} s_folders[] = {
    {"psx", "Models"}, {"bmp", "Textures"}, {"trg", "Scripts"}, {"vab", "Sounds"},
    {"fnt", "Fonts"},  {"pre", "Packs"},    {"rec", "Demos"},   {"txt", "Text"},
};

static char s_root[MAX_PATH];

static int Exists(const char *p) {
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static const char *Root(void) {
    if (!s_root[0]) {
        char *slash;
        if (getenv("THPS_DATA_DIR")) {
            snprintf(s_root, sizeof s_root, "%s", getenv("THPS_DATA_DIR"));
        } else {
            GetModuleFileNameA(NULL, s_root, sizeof s_root - 8);
            slash = strrchr(s_root, '\\');
            strcpy(slash ? slash + 1 : s_root, "Data");
        }
    }
    return s_root;
}

void PC_AssetsPath(char *out, int n, const char *rel) { snprintf(out, n, "%s\\%s", Root(), rel); }

/* "\\SUICIDE.STR;1" -> "SUICIDE.STR" */
static void BaseName(const char *discName, char *out, int n) {
    const char *b = discName, *p;
    int len;
    for (p = discName; *p; p++)
        if (*p == '\\' || *p == '/')
            b = p + 1;
    p = strchr(b, ';');
    len = p ? (int)(p - b) : (int)strlen(b);
    snprintf(out, n, "%.*s", len, b);
}

static void MakeDirs(const char *rel) {
    char p[MAX_PATH], *s;
    PC_AssetsPath(p, sizeof p, rel);
    CreateDirectoryA(Root(), NULL);
    for (s = p + strlen(Root()) + 1; (s = strchr(s, '\\')); s++) {
        *s = 0;
        CreateDirectoryA(p, NULL);
        *s = '\\';
    }
    CreateDirectoryA(p, NULL);
}

static long long FileSize(const char *p) {
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(p, GetFileExInfoStandard, &a))
        return 0;
    return (long long)a.nFileSizeHigh << 32 | a.nFileSizeLow;
}

/* Write `dst` through a .part file, so an interrupted extraction is redone next time instead of
 * leaving a truncated file behind. */
static FILE *Create(const char *dst, char *part, int n) {
    FILE *f;
    snprintf(part, n, "%s.part", dst);
    f = fopen(part, "wb");
    if (!f)
        PC_Fatal("cannot create %s", part);
    return f;
}

static void Commit(FILE *f, const char *part, const char *dst) {
    if (fclose(f) || !MoveFileExA(part, dst, MOVEFILE_REPLACE_EXISTING))
        PC_Fatal("cannot write %s (disk full?)", dst);
}

/* ---- STR files (movies and music) ----------------------------------------------------------- */

typedef struct {
    char name[64]; /* "SUICIDE.STR" */
    int music;
} StrFile;

/* The STR files, in movie-table order. */
static int ListStrFiles(StrFile *e) {
    int i, n = 0;
    for (i = 0; i < MOVIE_COUNT; i++) {
        const uint8_t *d = PSX_PTR(MOVIE_DETAILS + i * 32);
        uint32_t name;
        memcpy(&name, d, 4);
        if (!name)
            continue;
        BaseName(PSX_PTR(name), e[n].name, sizeof e[n].name);
        e[n].music = (d[4] | d[5] << 8) == MUSIC_WIDTH;
        n++;
    }
    return n;
}

static void StrPath(const StrFile *e, char *path, int n) {
    char rel[96];
    snprintf(rel, sizeof rel, "%s\\%.63s", e->music ? "Music" : "Movies", e->name);
    PC_AssetsPath(path, n, rel);
}

/* Copy one STR's raw sectors out of the disc image. */
static void ExtractStr(const char *name, const char *dst) {
    static uint8_t buf[COPY_SECTORS * 2352];
    char disc[80], part[MAX_PATH];
    uint32_t lba, size, sectors, done;
    FILE *f;
    snprintf(disc, sizeof disc, "\\%.63s;1", name);
    if (!PC_CdFindDiscFile(disc, &lba, &size))
        PC_Fatal("%s is not on the disc image. Is it Tony Hawk's Pro Skater (USA)?", name);
    sectors = (size + 2047) / 2048;
    f = Create(dst, part, sizeof part);
    for (done = 0; done < sectors;) {
        uint32_t n = sectors - done < COPY_SECTORS ? sectors - done : COPY_SECTORS;
        if (!PC_CdReadDiscSectors(lba + done, n, buf))
            PC_Fatal("cannot read %s from the disc image (sector %u)", name, lba + done);
        if (fwrite(buf, 2352, n, f) != n)
            PC_Fatal("cannot write %s (disk full?)", part);
        done += n;
    }
    Commit(f, part, dst);
}

static void ExtractStrFiles(const StrFile *e, int n) {
    char path[MAX_PATH];
    int i, missing = 0, done = 0;
    for (i = 0; i < n; i++) {
        StrPath(&e[i], path, sizeof path);
        missing += !Exists(path);
    }
    if (!missing)
        return;
    PC_Log("extracting %d movies and songs from the disc to %s", missing, Root());
    MakeDirs("Music");
    MakeDirs("Movies");
    for (i = 0; i < n; i++) {
        StrPath(&e[i], path, sizeof path);
        if (Exists(path))
            continue;
        PC_Log("  [%d/%d] %s", ++done, missing, e[i].name);
        ExtractStr(e[i].name, path);
    }
}

/* The default playlist: the songs in the order the game cycles them. Left alone once it exists,
 * so the player's edits stick. */
static void WritePlaylist(const StrFile *e, int n) {
    char path[MAX_PATH], song[MAX_PATH], part[MAX_PATH];
    FILE *f;
    int i, k;
    PC_AssetsPath(path, sizeof path, "Music\\Playlist.m3u");
    if (Exists(path))
        return;
    MakeDirs("Music");
    f = Create(path, part, sizeof part);
    fputs("#EXTM3U\n", f);
    for (i = 0; i < n; i++) {
        const char *title = e[i].name;
        if (!e[i].music)
            continue;
        for (k = 0; k < (int)(sizeof s_credits / sizeof *s_credits); k++)
            if (!_stricmp(s_credits[k].file, e[i].name))
                title = s_credits[k].title;
        StrPath(&e[i], song, sizeof song);
        fprintf(f, "#EXTINF:%d,%s\n%s\n", (int)(FileSize(song) / 2352 / STREAM_RATE), title,
                e[i].name);
    }
    Commit(f, part, path);
    PC_Log("wrote %s", path);
}

/* ---- CD.WAD ------------------------------------------------------------------------------- */

typedef struct {
    char name[32];
    uint32_t offset, size;
    char rel[64]; /* where it goes under Data/ */
} WadFile;

/* Read a whole disc file's user data (mode 2 form 1). Returns a malloc'd buffer. */
static uint8_t *ReadDiscFile(const char *name, uint32_t *size, uint32_t *lba) {
    uint32_t sectors, i;
    uint8_t *buf, raw[2352];
    if (!PC_CdFindDiscFile(name, lba, size))
        PC_Fatal("%s is not on the disc image. Is it Tony Hawk's Pro Skater (USA)?", name);
    sectors = (*size + 2047) / 2048;
    buf = malloc((size_t)sectors * 2048);
    for (i = 0; i < sectors; i++) {
        if (!PC_CdReadDiscSectors(*lba + i, 1, raw))
            PC_Fatal("cannot read %s from the disc image", name);
        memcpy(buf + i * 2048, raw + 24, 2048);
    }
    return buf;
}

static int LevelCode(const char *name, char *code, int n) {
    int len = (int)strlen(name);
    if (len < 8 || _strnicmp(name, "sk", 2) || _stricmp(name + len - 6, "_t.trg"))
        return 0;
    snprintf(code, n, "%.*s", len - 8, name + 2);
    return 1;
}

/* The CD.HED entries, first occurrence of each name (the game's lookup takes the first), each
 * with its place under Data/. */
static int ListWadFiles(WadFile *w) {
    uint32_t hedSize, hedLba, p = 0;
    uint8_t *hed = ReadDiscFile("\\CD.HED;1", &hedSize, &hedLba);
    char codes[MAX_LEVELS][16];
    int n = 0, ncodes = 0, i, k;
    while (p < hedSize && hed[p] != 255 && n < MAX_WAD_FILES) {
        const char *name = (const char *)hed + p;
        int len = (int)strnlen(name, hedSize - p), dup = 0;
        p = (p + len + 1 + 3) & ~3u;
        if (p + 8 > hedSize || len >= (int)sizeof w->name)
            PC_Fatal("CD.HED is damaged");
        for (i = 0; i < n && !dup; i++)
            dup = !_stricmp(w[i].name, name);
        if (!dup) {
            snprintf(w[n].name, sizeof w[n].name, "%s", name);
            memcpy(&w[n].offset, hed + p, 4);
            memcpy(&w[n].size, hed + p + 4, 4);
            if (ncodes < MAX_LEVELS && LevelCode(name, codes[ncodes], sizeof codes[0]))
                ncodes++;
            n++;
        }
        p += 8;
    }
    free(hed);
    for (i = 0; i < n; i++) {
        const char *ext = strrchr(w[i].name, '.'), *dir = "Misc";
        ext = ext ? ext + 1 : "";
        for (k = 0; k < (int)(sizeof s_folders / sizeof *s_folders); k++)
            if (!_stricmp(ext, s_folders[k].ext))
                dir = s_folders[k].dir;
        if (!_stricmp(ext, "psx"))
            for (k = 0; k < ncodes; k++)
                if (!_strnicmp(w[i].name + 2, codes[k], strlen(codes[k])) &&
                    !_strnicmp(w[i].name, "sk", 2))
                    dir = "Levels";
        snprintf(w[i].rel, sizeof w[i].rel, "%s\\%s", dir, w[i].name);
    }
    return n;
}

/* Data/Manifest.txt: 1 if it exists and every file it lists does too. */
static int WadComplete(void) {
    char path[MAX_PATH], line[MAX_PATH], file[MAX_PATH];
    FILE *f;
    int ok = 1, any = 0;
    PC_AssetsPath(path, sizeof path, "Manifest.txt");
    f = fopen(path, "r");
    if (!f)
        return 0;
    while (ok && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0] || line[0] == '#')
            continue;
        PC_AssetsPath(file, sizeof file, line);
        ok = Exists(file);
        any = 1;
    }
    fclose(f);
    return ok && any;
}

static void ExtractWadFiles(void) {
    static WadFile w[MAX_WAD_FILES];
    char path[MAX_PATH], part[MAX_PATH], dir[64];
    uint32_t wadSize, wadLba;
    uint8_t raw[2352], *buf = NULL;
    size_t cap = 0;
    FILE *f;
    int n, i, done = 0, missing = 0;
    if (WadComplete())
        return;
    n = ListWadFiles(w);
    if (!PC_CdFindDiscFile("\\CD.WAD;1", &wadLba, &wadSize))
        PC_Fatal("CD.WAD is not on the disc image. Is it Tony Hawk's Pro Skater (USA)?");
    for (i = 0; i < n; i++) {
        PC_AssetsPath(path, sizeof path, w[i].rel);
        missing += !Exists(path);
    }
    PC_Log("extracting %d game files from the disc to %s", missing, Root());
    for (i = 0; i < n; i++) {
        uint32_t sectors = (w[i].size + 2047) / 2048, s;
        PC_AssetsPath(path, sizeof path, w[i].rel);
        if (Exists(path))
            continue;
        if (w[i].offset % 2048 || w[i].offset + w[i].size > wadSize)
            PC_Fatal("CD.HED entry %s is outside CD.WAD", w[i].name);
        if (cap < (size_t)sectors * 2048) {
            cap = (size_t)sectors * 2048;
            buf = realloc(buf, cap);
        }
        for (s = 0; s < sectors; s++) {
            if (!PC_CdReadDiscSectors(wadLba + w[i].offset / 2048 + s, 1, raw))
                PC_Fatal("cannot read %s from the disc image", w[i].name);
            memcpy(buf + s * 2048, raw + 24, 2048);
        }
        snprintf(dir, sizeof dir, "%.*s", (int)(strchr(w[i].rel, '\\') - w[i].rel), w[i].rel);
        MakeDirs(dir);
        if (++done % 50 == 0 || done == missing)
            PC_Log("  [%d/%d] %s", done, missing, w[i].rel);
        f = Create(path, part, sizeof part);
        if (fwrite(buf, 1, w[i].size, f) != w[i].size)
            PC_Fatal("cannot write %s (disk full?)", part);
        Commit(f, part, path);
    }
    free(buf);
    PC_AssetsPath(path, sizeof path, "Manifest.txt");
    f = Create(path, part, sizeof part);
    fputs("# Game files extracted from CD.WAD (deleting this re-checks them on the next start)\n", f);
    for (i = 0; i < n; i++)
        fprintf(f, "%s\n", w[i].rel);
    Commit(f, part, path);
}

/* ---- index: file name -> path -------------------------------------------------------------- */

typedef struct {
    char *name, *path;
} IndexEntry;

static IndexEntry *s_index;
static int s_nindex, s_capindex;

static void IndexDir(const char *dir) {
    char pat[MAX_PATH], sub[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    snprintf(pat, sizeof pat, "%s\\*", dir);
    h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (fd.cFileName[0] == '.')
            continue;
        snprintf(sub, sizeof sub, "%s\\%s", dir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            IndexDir(sub);
            continue;
        }
        if (s_nindex == s_capindex) {
            s_capindex = s_capindex ? s_capindex * 2 : 1024;
            s_index = realloc(s_index, sizeof *s_index * s_capindex);
        }
        s_index[s_nindex].name = _strdup(fd.cFileName);
        s_index[s_nindex].path = _strdup(sub);
        s_nindex++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static int CmpIndex(const void *a, const void *b) {
    return _stricmp(((const IndexEntry *)a)->name, ((const IndexEntry *)b)->name);
}

static void BuildIndex(void) {
    int i;
    IndexDir(Root());
    qsort(s_index, s_nindex, sizeof *s_index, CmpIndex);
    for (i = 1; i < s_nindex; i++)
        if (!_stricmp(s_index[i - 1].name, s_index[i].name))
            PC_Log("note: %s and %s have the same name; the game sees one of them",
                   s_index[i - 1].path, s_index[i].path);
    PC_Trace("indexed %d files under %s", s_nindex, Root());
}

int PC_AssetsFind(const char *discName, char *path, int n) {
    char base[MAX_PATH];
    IndexEntry key, *e;
    BaseName(discName, base, sizeof base);
    key.name = base;
    e = bsearch(&key, s_index, s_nindex, sizeof *s_index, CmpIndex);
    if (!e)
        return 0;
    snprintf(path, n, "%s", e->path);
    return 1;
}

void PC_AssetsInit(void) {
    StrFile e[MOVIE_COUNT];
    int n = ListStrFiles(e);
    ExtractStrFiles(e, n);
    ExtractWadFiles();
    WritePlaylist(e, n);
    BuildIndex();
}

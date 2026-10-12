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
 * its file name, whatever folder it is in.
 *
 * The Dreamcast disc (a .gdi dump; Treyarch's port) can be the source instead of the PS1 one:
 * its files are loose on the disc and go to the same folders. Its models and levels are version
 * 6 .psx files with 16-bit textures (pc_dctex.h), its soundtrack and movies .sfd files, its sound
 * banks .kat files. All are extracted unchanged; the images it redrew larger are shown at the
 * PS1 layout's size with all their pixels (PC_ImageLayoutSize). The first line of Manifest.txt
 * says which disc the files came from; with the Dreamcast's, the game gets more RAM (its files are bigger), never opens a PS1
 * disc image, and the skaters' front-end models (the Dreamcast has none) are their full ones. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "pc_assets.h"
#include "pc_gdrom.h"
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

/* ---- the Dreamcast disc --------------------------------------------------------------------- */

#define DC_MANIFEST_TAG "source: dreamcast"
#define STATS 0x800a44fc  /* Stats[]: skaters (SStats, 0x154 bytes): PSXName, PSXName2, FrontName */
#define STATS_SIZE 0x154
#define STATS_COUNT 14

static int s_dc;   /* Data/ holds the Dreamcast version's files */

int PC_AssetsDreamcast(void) { return s_dc; }

/* Manifest.txt says the files came from the Dreamcast disc */
static int ManifestIsDreamcast(void) {
    char path[MAX_PATH], line[256];
    FILE *f;
    int dc = 0;
    PC_AssetsPath(path, sizeof path, "Manifest.txt");
    f = fopen(path, "r");
    if (f) {
        dc = fgets(line, sizeof line, f) && strstr(line, DC_MANIFEST_TAG);
        fclose(f);
    }
    return dc;
}

/* A .gdi: THPS_DISC, else next to thps.exe or SLUS_008.60 (or in a folder there), or at the top
 * of the repository (development layout). */
static int FindGdiIn(const char *dir, char *out, int n, int depth) {
    char pat[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    snprintf(pat, sizeof pat, "%s\\*.gdi", dir);
    h = FindFirstFileA(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        snprintf(out, n, "%s\\%s", dir, fd.cFileName);
        FindClose(h);
        return 1;
    }
    if (!depth)
        return 0;
    snprintf(pat, sizeof pat, "%s\\*", dir);
    h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    do {
        char sub[MAX_PATH];
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.' ||
            !_stricmp(fd.cFileName, "Data"))
            continue;
        snprintf(sub, sizeof sub, "%s\\%s", dir, fd.cFileName);
        if (FindGdiIn(sub, out, n, depth - 1)) {
            FindClose(h);
            return 1;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return 0;
}

static int FindGdi(char *out, int n) {
    char exeDir[MAX_PATH], dir[MAX_PATH], *slash;
    const char *e = getenv("THPS_DISC");
    if (e) {
        size_t len = strlen(e);
        snprintf(out, n, "%s", e);
        return len >= 4 && !_stricmp(e + len - 4, ".gdi") && Exists(out);
    }
    GetModuleFileNameA(NULL, exeDir, sizeof exeDir);
    slash = strrchr(exeDir, '\\');
    if (slash)
        *slash = 0;
    if (FindGdiIn(exeDir, out, n, 1) || FindGdiIn(PC_DataDir(), out, n, 1))
        return 1;
    snprintf(dir, sizeof dir, "%s\\..\\..", exeDir);
    return FindGdiIn(dir, out, n, 1);
}

typedef struct {
    char name[64];
    uint32_t lba, size;
    char rel[96];
} DcFile;

typedef struct {
    DcFile *f;
    int n, cap;
} DcList;

static void DcAdd(const char *name, uint32_t lba, uint32_t size, void *ctx) {
    DcList *l = ctx;
    if (strchr(name, '/') || strlen(name) >= sizeof l->f->name)
        return;   /* the game's files are all in the root */
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 1024;
        l->f = realloc(l->f, sizeof *l->f * l->cap);
    }
    snprintf(l->f[l->n].name, sizeof l->f->name, "%s", name);
    l->f[l->n].lba = lba;
    l->f[l->n].size = size;
    l->f[l->n].rel[0] = 0;
    l->n++;
}

/* Folders as for the PS1 files; .kat sound banks with the .vab ones, .pvr images with the .bmp
 * ones, .sfd films in Music (the songs the movie table lists) or Movies. */
static const struct {
    const char *ext, *dir;
} s_dcFolders[] = {
    {"psx", "Models"}, {"bmp", "Textures"}, {"pvr", "Textures"}, {"trg", "Scripts"},
    {"kat", "Sounds"}, {"fnt", "Fonts"},    {"pre", "Packs"},    {"rec", "Demos"},
    {"txt", "Text"},
};

/* Not game data: the Dreamcast executable, boot files and leftovers. */
static const char *const s_dcSkip[] = {"1ST_READ.BIN", "IP.BIN", "TEST.BIN", "SIZES.FOO",
                                       "VSSVER.SCC", "AUDIO64.DRV", "SKVANS_T.PSX"};

static void DcPlace(DcFile *f, const StrFile *str, int nstr, char codes[][16], int ncodes) {
    char lower[64], base[64];
    const char *ext = strrchr(f->name, '.'), *dir = "Misc";
    int k;
    for (k = 0; k < (int)(sizeof s_dcSkip / sizeof *s_dcSkip); k++)
        if (!_stricmp(f->name, s_dcSkip[k]))
            return;
    for (k = 0; f->name[k] && k < (int)sizeof lower - 1; k++)
        lower[k] = (char)tolower((unsigned char)f->name[k]);
    lower[k] = 0;
    ext = ext ? ext + 1 : "";
    for (k = 0; k < (int)(sizeof s_dcFolders / sizeof *s_dcFolders); k++)
        if (!_stricmp(ext, s_dcFolders[k].ext))
            dir = s_dcFolders[k].dir;
    if (!_stricmp(ext, "psx") && !strncmp(lower, "sk", 2))
        for (k = 0; k < ncodes; k++) {
            /* sk<code>.psx, sk<code>_*.psx, sk<code>l2/o2.psx: the level's */
            size_t cl = strlen(codes[k]);
            const char *rest = lower + 2 + cl;
            if (!strncmp(lower + 2, codes[k], cl) &&
                (*rest == '.' || *rest == '_' || !strncmp(rest, "l2.", 3) ||
                 !strncmp(rest, "o2.", 3)))
                dir = "Levels";
        }
    if (!_stricmp(ext, "sfd")) {
        dir = "Movies";
        snprintf(base, sizeof base, "%.*s.STR", (int)(strrchr(f->name, '.') - f->name), f->name);
        for (k = 0; k < nstr; k++)
            if (str[k].music && !_stricmp(str[k].name, base))
                dir = "Music";
    }
    snprintf(f->rel, sizeof f->rel, "%s\\%s", dir, lower);
}

/* ---- images drawn at another size ---------------------------------------------------------- */

/* The images the Dreamcast redrew at a higher resolution, with the size the PS1 game lays them out
 * at (its own images' sizes; the front-end screens are line-doubled to 640x480 when loaded). They
 * are extracted as they are and shown at that size with all their pixels (pc_assets.h). */
static const struct {
    const char *name;
    int w, h;          /* the PS1 image */
    int doubled;       /* loaded line-doubled (DownloadBMP16x2) */
} s_layout[] = {
    {"title_h.bmp", 640, 240, 1},  {"main_h.bmp", 640, 240, 1},   {"options.bmp", 640, 240, 1},
    {"opt_ctr.bmp", 640, 240, 1},  {"opt_snd.bmp", 640, 240, 1},  {"selectp.bmp", 640, 240, 1},
    {"select_c.bmp", 640, 240, 1}, {"select2p.bmp", 640, 240, 1}, {"select_b.bmp", 640, 240, 1},
    {"select2b.bmp", 640, 240, 1}, {"load_hor.bmp", 512, 240, 0}, {"paused.bmp", 256, 44, 0},
};

int PC_ImageLayoutSize(const char *name, int w, int h, int *lw, int *lh) {
    char base[MAX_PATH];
    int k;
    if (!s_dc)
        return 0;
    BaseName(name, base, sizeof base);
    for (k = 0; k < (int)(sizeof s_layout / sizeof *s_layout); k++)
        if (!_stricmp(base, s_layout[k].name)) {
            if (w == s_layout[k].w && h == s_layout[k].h)
                return 0;   /* the PS1's size */
            *lw = s_layout[k].w;
            *lh = s_layout[k].h << s_layout[k].doubled;
            return 1;
        }
    return 0;
}

static void ExtractDreamcast(const StrFile *str, int nstr) {
    DcList l = {0};
    char gdi[MAX_PATH], path[MAX_PATH], part[MAX_PATH], dir[64], codes[MAX_LEVELS][16];
    uint8_t *buf = NULL;
    size_t cap = 0;
    int i, k, ncodes = 0, missing = 0, done = 0;
    FILE *f;
    if (WadComplete())
        return;
    if (!FindGdi(gdi, sizeof gdi))
        PC_Fatal("The Dreamcast disc image is gone, and some files are missing from the Data "
                 "folder. Put the .gdi of Tony Hawk's Pro Skater (Dreamcast) and its track files "
                 "next to thps.exe, or set THPS_DISC to the .gdi.");
    if (!PC_GdOpen(gdi))
        PC_Fatal("%s is not a Dreamcast disc image of Tony Hawk's Pro Skater", gdi);
    PC_GdList(DcAdd, &l);
    for (i = 0; i < l.n; i++) {
        char lower[64];
        for (k = 0; l.f[i].name[k] && k < 63; k++)
            lower[k] = (char)tolower((unsigned char)l.f[i].name[k]);
        lower[k] = 0;
        if (ncodes < MAX_LEVELS && LevelCode(lower, codes[ncodes], sizeof codes[0]))
            ncodes++;
    }
    for (i = 0; i < l.n; i++)
        DcPlace(&l.f[i], str, nstr, codes, ncodes);
    for (i = k = 0; i < l.n; i++)
        if (l.f[i].rel[0]) {
            if (!strcmp(l.f[i].rel, "Models\\skmedals.psx") || !strcmp(l.f[i].rel, "Models\\hawk.psx"))
                k++;
            PC_AssetsPath(path, sizeof path, l.f[i].rel);
            missing += !Exists(path);
        }
    if (k != 2)
        PC_Fatal("%s is not Tony Hawk's Pro Skater for the Dreamcast", gdi);
    PC_Log("extracting %d game files from the Dreamcast disc %s to %s", missing, gdi, Root());
    for (i = 0; i < l.n; i++) {
        DcFile *e = &l.f[i];
        uint32_t size = e->size;
        if (!e->rel[0])
            continue;
        PC_AssetsPath(path, sizeof path, e->rel);
        if (Exists(path))
            continue;
        snprintf(dir, sizeof dir, "%.*s", (int)(strchr(e->rel, '\\') - e->rel), e->rel);
        MakeDirs(dir);
        if (++done % 50 == 0 || done == missing)
            PC_Log("  [%d/%d] %s", done, missing, e->rel);
        f = Create(path, part, sizeof part);
        if (size > (8u << 20)) {
            /* the films: in pieces */
            uint32_t o;
            if (cap < (1u << 20)) {
                cap = 1u << 20;
                buf = realloc(buf, cap);
            }
            for (o = 0; o < size; o += 1u << 20) {
                uint32_t n = size - o < (1u << 20) ? size - o : (1u << 20);
                if (!PC_GdRead(e->lba + o / 2048, n, buf))
                    PC_Fatal("cannot read %s from %s", e->name, gdi);
                if (fwrite(buf, 1, n, f) != n)
                    PC_Fatal("cannot write %s (disk full?)", part);
            }
            Commit(f, part, path);
            continue;
        }
        if (cap < size + 2048) {
            cap = size + 2048;
            buf = realloc(buf, cap);
        }
        if (!PC_GdRead(e->lba, size, buf))
            PC_Fatal("cannot read %s from %s", e->name, gdi);
        if (fwrite(buf, 1, size, f) != size)
            PC_Fatal("cannot write %s (disk full?)", part);
        Commit(f, part, path);
    }
    free(buf);
    PC_GdClose();
    PC_AssetsPath(path, sizeof path, "Manifest.txt");
    f = Create(path, part, sizeof part);
    fputs("# Game files extracted from the Dreamcast disc (" DC_MANIFEST_TAG "; deleting this "
          "re-checks them on the next start)\n", f);
    for (i = 0; i < l.n; i++)
        if (l.f[i].rel[0])
            fprintf(f, "%s\n", l.f[i].rel);
    Commit(f, part, path);
    free(l.f);
}

/* After indexing, with the Dreamcast's files: each skater's front-end model is its full model
 * where there is no front-end one (the Dreamcast's skater table leaves those names empty and
 * the select screen shows the full model; only Muska has one). */
static void DreamcastFixups(void) {
    char name[64], path[MAX_PATH];
    int i;
    for (i = 0; i < STATS_COUNT; i++) {
        uint32_t e = STATS + i * STATS_SIZE, full, front;
        memcpy(&full, PSX_PTR(e), 4);
        memcpy(&front, PSX_PTR(e + 8), 4);
        if (full < PSX_EXE_BASE || full >= PSX_RAM_BASE + PSX_RAM_SIZE || front < PSX_EXE_BASE ||
            front >= PSX_RAM_BASE + PSX_RAM_SIZE || !*(const char *)PSX_PTR(front))
            continue;
        snprintf(name, sizeof name, "%.50s.psx", (const char *)PSX_PTR(front));
        if (PC_AssetsFind(name, path, sizeof path))
            continue;
        PC_Trace("dc: %s has no front-end model: using %s", name, (const char *)PSX_PTR(full));
        memcpy(PSX_PTR(e + 8), &full, 4);
    }
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
    char path[MAX_PATH];
    int n = ListStrFiles(e);
    /* the disc the files come from: what Manifest.txt says, else (first startup) the PS1 image
     * if there is one, else a Dreamcast .gdi */
    s_dc = ManifestIsDreamcast();
    if (!s_dc && !WadComplete() && !PC_CdFindImage(path, sizeof path)) {
        if (!FindGdi(path, sizeof path))
            PC_Fatal("The game data has to be extracted from your disc first, and no disc image "
                     "was found. Put a raw .bin image (MODE2/2352) of Tony Hawk's Pro Skater "
                     "(USA) for the PlayStation next to SLUS_008.60 as thps1.bin, or the .gdi "
                     "and track files of the Dreamcast version next to thps.exe; or set "
                     "THPS_DISC to either.");
        s_dc = 1;
    }
    if (s_dc) {
        ExtractDreamcast(e, n);
        PC_CdSetNoDisc(1);
        pc_ram_top = PSX_RAM_BASE + PSX_RAM_MAPPED;
    } else {
        ExtractStrFiles(e, n);
        ExtractWadFiles();
        WritePlaylist(e, n);
    }
    BuildIndex();
    if (s_dc) {
        DreamcastFixups();
        PC_Log("using the Dreamcast version's files");
    }
}

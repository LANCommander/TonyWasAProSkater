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
 * At runtime the game finds files by name (Assets::Find): every file under Data/ is indexed by
 * its file name, whatever folder it is in.
 *
 * The Dreamcast disc (a .gdi dump; Treyarch's port) can be the source instead of the PS1 one:
 * its files are loose on the disc and go to the same folders. Its models and levels are version
 * 6 .psx files with 16-bit textures (pc_dctex.h), its soundtrack and movies .sfd files, its sound
 * banks .kat files. All are extracted unchanged; the images it redrew larger are shown at the
 * PS1 layout's size with all their pixels (PC_ImageLayoutSize). The first line of Manifest.txt
 * says which disc the files came from; with the Dreamcast's, the game gets more RAM (its files are bigger), never opens a PS1
 * disc image, and the skaters' front-end models (the Dreamcast has none) are their full ones. */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "pc/assets.h"
#include "pc/disc_image.h"
#include "pc/gdrom.h"
#include "pc_assets.h"
#include "pc/game_data.h"
#include "pc_runtime.h"
#include "pc/platform.h"
#include "pc/runtime.h"
#include "pc/sfd.h"

/* the Manifest.txt tag of the Dreamcast's files (pasted into its header line) */
#define DC_MANIFEST_TAG "source: dreamcast"

namespace pc {

struct Assets::StrFile {
    char name[64]; /* "SUICIDE.STR" */
    int music;
};

Assets g_assets;

namespace {

constexpr int kMovieCount = GAME_MOVIE_COUNT;
constexpr int kMusicWidth = 96;      /* soundtrack entries carry a 96x64 video */
constexpr int kStreamRate = 150;     /* sectors/s: the songs stream at 2x */
constexpr uint32_t kCopySectors = 64;
constexpr int kMaxWadFiles = 1024;
constexpr int kMaxLevels = 32;

/* Song credits for the playlist, by file. Order does not come from here. */
constexpr struct {
    const char *file, *title;
} kCredits[] = {
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
constexpr struct {
    const char *ext, *dir;
} kFolders[] = {
    {"psx", "Models"}, {"bmp", "Textures"}, {"trg", "Scripts"}, {"vab", "Sounds"},
    {"fnt", "Fonts"},  {"pre", "Packs"},    {"rec", "Demos"},   {"txt", "Text"},
};

/* Folders as for the PS1 files; .kat sound banks with the .vab ones, .pvr images with the .bmp
 * ones, .sfd films in Music (the songs the movie table lists) or Movies. */
constexpr struct {
    const char *ext, *dir;
} kDcFolders[] = {
    {"psx", "Models"}, {"bmp", "Textures"}, {"pvr", "Textures"}, {"trg", "Scripts"},
    {"kat", "Sounds"}, {"fnt", "Fonts"},    {"pre", "Packs"},    {"rec", "Demos"},
    {"txt", "Text"},
};

/* Not game data: the Dreamcast executable, boot files and leftovers. */
constexpr const char *kDcSkip[] = {"1ST_READ.BIN", "IP.BIN", "TEST.BIN", "SIZES.FOO",
                                   "VSSVER.SCC", "AUDIO64.DRV", "SKVANS_T.PSX"};

/* The images the Dreamcast redrew at a higher resolution, with the size the PS1 game lays them out
 * at (its own images' sizes; the front-end screens are line-doubled to 640x480 when loaded). They
 * are extracted as they are and shown at that size with all their pixels (pc_assets.h). */
constexpr struct {
    const char *name;
    int w, h;          /* the PS1 image */
    int doubled;       /* loaded line-doubled (DownloadBMP16x2) */
} kLayout[] = {
    {"title_h.bmp", 640, 240, 1},  {"main_h.bmp", 640, 240, 1},   {"options.bmp", 640, 240, 1},
    {"opt_ctr.bmp", 640, 240, 1},  {"opt_snd.bmp", 640, 240, 1},  {"selectp.bmp", 640, 240, 1},
    {"select_c.bmp", 640, 240, 1}, {"select2p.bmp", 640, 240, 1}, {"select_b.bmp", 640, 240, 1},
    {"select2b.bmp", 640, 240, 1}, {"load_hor.bmp", 512, 240, 0}, {"paused.bmp", 256, 44, 0},
};

/* The Dreamcast's names for the PS1's films where they differ (the PS1's are 8.3 names). */
constexpr struct {
    const char *str, *sfd;
} kFilms[] = {
    {"ATVILOGO", "activision"}, {"LOGO", "never"}, {"BURNQUIS", "burnquist"},
};

/* "\\SUICIDE.STR;1" -> "SUICIDE.STR" */
void BaseName(const char *discName, char *out, int n) {
    const char *b = discName, *p;
    int len;
    for (p = discName; *p; p++)
        if (*p == '\\' || *p == '/')
            b = p + 1;
    p = strchr(b, ';');
    len = p ? (int)(p - b) : (int)strlen(b);
    snprintf(out, n, "%.*s", len, b);
}

/* Write `dst` through a .part file, so an interrupted extraction is redone next time instead of
 * leaving a truncated file behind. */
FILE *Create(const char *dst, char *part, int n) {
    snprintf(part, n, "%s.part", dst);
    FILE *f = fopen(part, "wb");
    if (!f)
        PC_Fatal("cannot create %s", part);
    return f;
}

void Commit(FILE *f, const char *part, const char *dst) {
    if (fclose(f) || !os::RenameOver(part, dst))
        PC_Fatal("cannot write %s (disk full?)", dst);
}

/* "SUICIDE.STR" -> "suicide.sfd" */
void FilmName(const char *str, char *out, int n) {
    char base[64], *dot;
    snprintf(base, sizeof base, "%s", str);
    if ((dot = strrchr(base, '.')))
        *dot = 0;
    for (const auto &film : kFilms)
        if (!_stricmp(base, film.str))
            snprintf(base, sizeof base, "%s", film.sfd);
    for (int k = 0; base[k]; k++)
        base[k] = (char)tolower((unsigned char)base[k]);
    snprintf(out, n, "%s.sfd", base);
}

/* ---- STR files (movies and music) ----------------------------------------------------------- */

/* The STR files, in movie-table order. */
int ListStrFiles(Assets::StrFile *e) {
    int n = 0;
    for (int i = 0; i < kMovieCount; i++) {
        const GameMovieDetails *d = &movieDetails[i];
        if (!d->name)
            continue;
        BaseName(d->name, e[n].name, sizeof e[n].name);
        e[n].music = d->width == kMusicWidth;
        n++;
    }
    return n;
}

/* Copy one STR's raw sectors out of the disc image. */
void ExtractStr(const char *name, const char *dst) {
    static uint8_t buf[kCopySectors * 2352];
    char disc[80], part[MAX_PATH];
    uint32_t lba, size, sectors, done;
    snprintf(disc, sizeof disc, "\\%.63s;1", name);
    if (!g_discImage.FindFile(disc, &lba, &size))
        PC_Fatal("%s is not on the disc image. Is it Tony Hawk's Pro Skater (USA)?", name);
    sectors = (size + 2047) / 2048;
    FILE *f = Create(dst, part, sizeof part);
    for (done = 0; done < sectors;) {
        uint32_t n = sectors - done < kCopySectors ? sectors - done : kCopySectors;
        if (!g_discImage.ReadSectors(lba + done, n, buf))
            PC_Fatal("cannot read %s from the disc image (sector %u)", name, lba + done);
        if (fwrite(buf, 2352, n, f) != n)
            PC_Fatal("cannot write %s (disk full?)", part);
        done += n;
    }
    Commit(f, part, dst);
}

/* ---- CD.WAD ------------------------------------------------------------------------------- */

struct WadFile {
    char name[32];
    uint32_t offset, size;
    char rel[64]; /* where it goes under Data/ */
};

/* Read a whole disc file's user data (mode 2 form 1). Returns a malloc'd buffer. */
uint8_t *ReadDiscFile(const char *name, uint32_t *size, uint32_t *lba) {
    uint32_t sectors;
    uint8_t *buf, raw[2352];
    if (!g_discImage.FindFile(name, lba, size))
        PC_Fatal("%s is not on the disc image. Is it Tony Hawk's Pro Skater (USA)?", name);
    sectors = (*size + 2047) / 2048;
    buf = (uint8_t *)malloc((size_t)sectors * 2048);
    for (uint32_t i = 0; i < sectors; i++) {
        if (!g_discImage.ReadSectors(*lba + i, 1, raw))
            PC_Fatal("cannot read %s from the disc image", name);
        memcpy(buf + i * 2048, raw + 24, 2048);
    }
    return buf;
}

bool LevelCode(const char *name, char *code, int n) {
    int len = (int)strlen(name);
    if (len < 8 || _strnicmp(name, "sk", 2) || _stricmp(name + len - 6, "_t.trg"))
        return false;
    snprintf(code, n, "%.*s", len - 8, name + 2);
    return true;
}

/* The CD.HED entries, first occurrence of each name (the game's lookup takes the first), each
 * with its place under Data/. */
int ListWadFiles(WadFile *w) {
    uint32_t hedSize, hedLba, p = 0;
    uint8_t *hed = ReadDiscFile("\\CD.HED;1", &hedSize, &hedLba);
    char codes[kMaxLevels][16];
    int n = 0, ncodes = 0, i;
    while (p < hedSize && hed[p] != 255 && n < kMaxWadFiles) {
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
            if (ncodes < kMaxLevels && LevelCode(name, codes[ncodes], sizeof codes[0]))
                ncodes++;
            n++;
        }
        p += 8;
    }
    free(hed);
    for (i = 0; i < n; i++) {
        const char *ext = strrchr(w[i].name, '.'), *dir = "Misc";
        ext = ext ? ext + 1 : "";
        for (const auto &folder : kFolders)
            if (!_stricmp(ext, folder.ext))
                dir = folder.dir;
        if (!_stricmp(ext, "psx"))
            for (int k = 0; k < ncodes; k++)
                if (!_strnicmp(w[i].name + 2, codes[k], strlen(codes[k])) &&
                    !_strnicmp(w[i].name, "sk", 2))
                    dir = "Levels";
        snprintf(w[i].rel, sizeof w[i].rel, "%s\\%s", dir, w[i].name);
    }
    return n;
}

/* ---- the Dreamcast disc --------------------------------------------------------------------- */

/* A .gdi: THPS_DISC, else next to thps.exe (or in a folder there), or at the top
 * of the repository (development layout). */
bool FindGdiIn(const char *dir, char *out, int n, int depth) {
    os::DirEntry fd;
    {
        os::DirList list(dir);
        if (!list)
            return false;
        while (list.Next(&fd)) {
            size_t len = strlen(fd.name);
            if (!fd.isDir && len >= 4 && !_stricmp(fd.name + len - 4, ".gdi")) {
                snprintf(out, n, "%s" PC_SEP "%s", dir, fd.name);
                return true;
            }
        }
    }
    if (!depth)
        return false;
    os::DirList list(dir);
    if (!list)
        return false;
    while (list.Next(&fd)) {
        char sub[MAX_PATH];
        if (!fd.isDir || fd.name[0] == '.' || !_stricmp(fd.name, "Data"))
            continue;
        snprintf(sub, sizeof sub, "%s" PC_SEP "%s", dir, fd.name);
        if (FindGdiIn(sub, out, n, depth - 1))
            return true;
    }
    return false;
}

bool FindGdi(char *out, int n) {
    char exeDir[MAX_PATH], dir[MAX_PATH];
    const char *e = getenv("THPS_DISC");
    if (e) {
        size_t len = strlen(e);
        snprintf(out, n, "%s", e);
        return len >= 4 && !_stricmp(e + len - 4, ".gdi") && os::IsFile(out);
    }
    os::ExeDir(exeDir, sizeof exeDir);
    if (FindGdiIn(exeDir, out, n, 1) || FindGdiIn(g_runtime.DataDir(), out, n, 1))
        return true;
    snprintf(dir, sizeof dir, "%s" PC_SEP ".." PC_SEP "..", exeDir);
    return FindGdiIn(dir, out, n, 1);
}

struct DcFile {
    char name[64];
    uint32_t lba, size;
    char rel[96];
};

struct DcList {
    DcFile *f;
    int n, cap;
};

void DcAdd(const char *name, uint32_t lba, uint32_t size, void *ctx) {
    DcList *l = (DcList *)ctx;
    if (strchr(name, '/') || strlen(name) >= sizeof l->f->name)
        return;   /* the game's files are all in the root */
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 1024;
        l->f = (DcFile *)realloc(l->f, sizeof *l->f * l->cap);
    }
    snprintf(l->f[l->n].name, sizeof l->f->name, "%s", name);
    l->f[l->n].lba = lba;
    l->f[l->n].size = size;
    l->f[l->n].rel[0] = 0;
    l->n++;
}

void DcPlace(DcFile *f, const Assets::StrFile *str, int nstr, char codes[][16], int ncodes) {
    char lower[64], base[64];
    const char *ext = strrchr(f->name, '.'), *dir = "Misc";
    int k;
    for (const char *skip : kDcSkip)
        if (!_stricmp(f->name, skip))
            return;
    for (k = 0; f->name[k] && k < (int)sizeof lower - 1; k++)
        lower[k] = (char)tolower((unsigned char)f->name[k]);
    lower[k] = 0;
    ext = ext ? ext + 1 : "";
    for (const auto &folder : kDcFolders)
        if (!_stricmp(ext, folder.ext))
            dir = folder.dir;
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

} // namespace

const char *Assets::Root() {
    if (!m_root[0]) {
        if (getenv("THPS_DATA_DIR")) {
            snprintf(m_root, sizeof m_root, "%s", getenv("THPS_DATA_DIR"));
        } else {
            os::ExeDir(m_root, sizeof m_root - 8);
            strcat(m_root, PC_SEP "Data");
        }
    }
    return m_root;
}

/* Relative paths are spelled with backslashes (as in Manifest.txt) on every system. */
void Assets::Path(char *out, int n, const char *rel) {
    snprintf(out, n, "%s" PC_SEP "%s", Root(), rel);
    os::PathFix(out);
}

void Assets::MakeDirs(const char *rel) {
    char p[MAX_PATH];
    Path(p, sizeof p, rel);
    os::MakeDirs(p);
}

/* ---- STR files (movies and music) ----------------------------------------------------------- */

void Assets::StrPath(const StrFile *e, char *path, int n) {
    char rel[96];
    snprintf(rel, sizeof rel, "%s\\%.63s", e->music ? "Music" : "Movies", e->name);
    Path(path, n, rel);
}

void Assets::ExtractStrFiles(const StrFile *e, int n) {
    char path[MAX_PATH];
    int i, missing = 0, done = 0;
    for (i = 0; i < n; i++) {
        StrPath(&e[i], path, sizeof path);
        missing += !os::IsFile(path);
    }
    if (!missing)
        return;
    PC_Log("extracting %d movies and songs from the disc to %s", missing, Root());
    MakeDirs("Music");
    MakeDirs("Movies");
    for (i = 0; i < n; i++) {
        StrPath(&e[i], path, sizeof path);
        if (os::IsFile(path))
            continue;
        PC_Log("  [%d/%d] %s", ++done, missing, e[i].name);
        ExtractStr(e[i].name, path);
    }
}

/* The default playlist: the songs in the order the game cycles them (the Dreamcast's films of
 * them with its files). Left alone once it exists, so the player's edits stick. */
void Assets::WritePlaylist(const StrFile *e, int n) {
    char path[MAX_PATH], song[MAX_PATH], part[MAX_PATH], film[80];
    Path(path, sizeof path, "Music\\Playlist.m3u");
    if (os::IsFile(path))
        return;
    MakeDirs("Music");
    FILE *f = Create(path, part, sizeof part);
    fputs("#EXTM3U\n", f);
    for (int i = 0; i < n; i++) {
        const char *title = e[i].name;
        if (!e[i].music)
            continue;
        for (const auto &credit : kCredits)
            if (!_stricmp(credit.file, e[i].name))
                title = credit.title;
        if (m_dc) {
            char rel[96];
            FilmName(e[i].name, film, sizeof film);
            snprintf(rel, sizeof rel, "Music\\%s", film);
            Path(song, sizeof song, rel);
            fprintf(f, "#EXTINF:%d,%s\n%s\n", SfdFilm::AudioSeconds(song), title, film);
            continue;
        }
        StrPath(&e[i], song, sizeof song);
        fprintf(f, "#EXTINF:%d,%s\n%s\n", (int)(os::FileSize(song) / 2352 / kStreamRate), title,
                e[i].name);
    }
    Commit(f, part, path);
    PC_Log("wrote %s", path);
}

/* ---- CD.WAD ------------------------------------------------------------------------------- */

/* Data/Manifest.txt: true if it exists and every file it lists does too. */
bool Assets::WadComplete() {
    char path[MAX_PATH], line[MAX_PATH], file[MAX_PATH];
    bool ok = true, any = false;
    Path(path, sizeof path, "Manifest.txt");
    FILE *f = fopen(path, "r");
    if (!f)
        return false;
    while (ok && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0] || line[0] == '#')
            continue;
        Path(file, sizeof file, line);
        ok = os::IsFile(file);
        any = true;
    }
    fclose(f);
    return ok && any;
}

void Assets::ExtractWadFiles() {
    static WadFile w[kMaxWadFiles];
    char path[MAX_PATH], part[MAX_PATH], dir[64];
    uint32_t wadSize, wadLba;
    uint8_t raw[2352], *buf = nullptr;
    size_t cap = 0;
    FILE *f;
    int n, i, done = 0, missing = 0;
    if (WadComplete())
        return;
    n = ListWadFiles(w);
    if (!g_discImage.FindFile("\\CD.WAD;1", &wadLba, &wadSize))
        PC_Fatal("CD.WAD is not on the disc image. Is it Tony Hawk's Pro Skater (USA)?");
    for (i = 0; i < n; i++) {
        Path(path, sizeof path, w[i].rel);
        missing += !os::IsFile(path);
    }
    PC_Log("extracting %d game files from the disc to %s", missing, Root());
    for (i = 0; i < n; i++) {
        uint32_t sectors = (w[i].size + 2047) / 2048;
        Path(path, sizeof path, w[i].rel);
        if (os::IsFile(path))
            continue;
        if (w[i].offset % 2048 || w[i].offset + w[i].size > wadSize)
            PC_Fatal("CD.HED entry %s is outside CD.WAD", w[i].name);
        if (cap < (size_t)sectors * 2048) {
            cap = (size_t)sectors * 2048;
            buf = (uint8_t *)realloc(buf, cap);
        }
        for (uint32_t s = 0; s < sectors; s++) {
            if (!g_discImage.ReadSectors(wadLba + w[i].offset / 2048 + s, 1, raw))
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
    Path(path, sizeof path, "Manifest.txt");
    f = Create(path, part, sizeof part);
    fputs("# Game files extracted from CD.WAD (deleting this re-checks them on the next start)\n", f);
    for (i = 0; i < n; i++)
        fprintf(f, "%s\n", w[i].rel);
    Commit(f, part, path);
}

/* ---- the Dreamcast disc --------------------------------------------------------------------- */

/* Manifest.txt says the files came from the Dreamcast disc */
bool Assets::ManifestIsDreamcast() {
    char path[MAX_PATH], line[256];
    bool dc = false;
    Path(path, sizeof path, "Manifest.txt");
    FILE *f = fopen(path, "r");
    if (f) {
        dc = fgets(line, sizeof line, f) && strstr(line, DC_MANIFEST_TAG);
        fclose(f);
    }
    return dc;
}

/* ---- images drawn at another size ---------------------------------------------------------- */

bool Assets::ImageLayoutSize(const char *name, int w, int h, int *lw, int *lh) const {
    char base[MAX_PATH];
    if (!m_dc)
        return false;
    BaseName(name, base, sizeof base);
    for (const auto &image : kLayout)
        if (!_stricmp(base, image.name)) {
            if (w == image.w && h == image.h)
                return false;   /* the PS1's size */
            *lw = image.w;
            *lh = image.h << image.doubled;
            return true;
        }
    return false;
}

void Assets::ExtractDreamcast(const StrFile *str, int nstr) {
    DcList l = {};
    GdRom gd;
    char gdi[MAX_PATH], path[MAX_PATH], part[MAX_PATH], dir[64], codes[kMaxLevels][16];
    uint8_t *buf = nullptr;
    size_t cap = 0;
    int i, k, ncodes = 0, missing = 0, done = 0;
    FILE *f;
    if (WadComplete())
        return;
    if (!FindGdi(gdi, sizeof gdi))
        PC_Fatal("The Dreamcast disc image is gone, and some files are missing from the Data "
                 "folder. Put the .gdi of Tony Hawk's Pro Skater (Dreamcast) and its track files "
                 "next to " PC_EXE_NAME ", or set THPS_DISC to the .gdi.");
    if (!gd.Open(gdi))
        PC_Fatal("%s is not a Dreamcast disc image of Tony Hawk's Pro Skater", gdi);
    gd.List(DcAdd, &l);
    for (i = 0; i < l.n; i++) {
        char lower[64];
        for (k = 0; l.f[i].name[k] && k < 63; k++)
            lower[k] = (char)tolower((unsigned char)l.f[i].name[k]);
        lower[k] = 0;
        if (ncodes < kMaxLevels && LevelCode(lower, codes[ncodes], sizeof codes[0]))
            ncodes++;
    }
    for (i = 0; i < l.n; i++)
        DcPlace(&l.f[i], str, nstr, codes, ncodes);
    for (i = k = 0; i < l.n; i++)
        if (l.f[i].rel[0]) {
            if (!strcmp(l.f[i].rel, "Models\\skmedals.psx") || !strcmp(l.f[i].rel, "Models\\hawk.psx"))
                k++;
            Path(path, sizeof path, l.f[i].rel);
            missing += !os::IsFile(path);
        }
    if (k != 2)
        PC_Fatal("%s is not Tony Hawk's Pro Skater for the Dreamcast", gdi);
    PC_Log("extracting %d game files from the Dreamcast disc %s to %s", missing, gdi, Root());
    for (i = 0; i < l.n; i++) {
        DcFile *e = &l.f[i];
        uint32_t size = e->size;
        if (!e->rel[0])
            continue;
        Path(path, sizeof path, e->rel);
        if (os::IsFile(path))
            continue;
        snprintf(dir, sizeof dir, "%.*s", (int)(strchr(e->rel, '\\') - e->rel), e->rel);
        MakeDirs(dir);
        if (++done % 50 == 0 || done == missing)
            PC_Log("  [%d/%d] %s", done, missing, e->rel);
        f = Create(path, part, sizeof part);
        if (size > (8u << 20)) {
            /* the films: in pieces */
            if (cap < (1u << 20)) {
                cap = 1u << 20;
                buf = (uint8_t *)realloc(buf, cap);
            }
            for (uint32_t o = 0; o < size; o += 1u << 20) {
                uint32_t n = size - o < (1u << 20) ? size - o : (1u << 20);
                if (!gd.Read(e->lba + o / 2048, n, buf))
                    PC_Fatal("cannot read %s from %s", e->name, gdi);
                if (fwrite(buf, 1, n, f) != n)
                    PC_Fatal("cannot write %s (disk full?)", part);
            }
            Commit(f, part, path);
            continue;
        }
        if (cap < size + 2048) {
            cap = size + 2048;
            buf = (uint8_t *)realloc(buf, cap);
        }
        if (!gd.Read(e->lba, size, buf))
            PC_Fatal("cannot read %s from %s", e->name, gdi);
        if (fwrite(buf, 1, size, f) != size)
            PC_Fatal("cannot write %s (disk full?)", part);
        Commit(f, part, path);
    }
    free(buf);
    gd.Close();
    Path(path, sizeof path, "Manifest.txt");
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
void Assets::DreamcastFixups() {
    char name[64], path[MAX_PATH];
    for (int i = 0; i < GAME_SKATERS; i++) {
        GameStats *e = &::Stats[i];
        if (!e->PSXName || !e->FrontName || !*e->FrontName)
            continue;
        snprintf(name, sizeof name, "%.50s.psx", e->FrontName);
        if (Find(name, path, sizeof path))
            continue;
        PC_Trace("dc: %s has no front-end model: using %s", name, e->PSXName);
        e->FrontName = e->PSXName;
    }
}

/* ---- index: file name -> path -------------------------------------------------------------- */

int Assets::CmpIndex(const void *a, const void *b) {
    return _stricmp(((const IndexEntry *)a)->name, ((const IndexEntry *)b)->name);
}

void Assets::IndexDir(const char *dir) {
    char sub[MAX_PATH];
    os::DirEntry fd;
    os::DirList list(dir);
    if (!list)
        return;
    while (list.Next(&fd)) {
        if (fd.name[0] == '.')
            continue;
        snprintf(sub, sizeof sub, "%s" PC_SEP "%s", dir, fd.name);
        if (fd.isDir) {
            IndexDir(sub);
            continue;
        }
        if (m_nindex == m_capindex) {
            m_capindex = m_capindex ? m_capindex * 2 : 1024;
            m_index = (IndexEntry *)realloc(m_index, sizeof *m_index * m_capindex);
        }
        m_index[m_nindex].name = _strdup(fd.name);
        m_index[m_nindex].path = _strdup(sub);
        m_nindex++;
    }
}

void Assets::BuildIndex() {
    IndexDir(Root());
    qsort(m_index, m_nindex, sizeof *m_index, CmpIndex);
    for (int i = 1; i < m_nindex; i++)
        if (!_stricmp(m_index[i - 1].name, m_index[i].name))
            PC_Log("note: %s and %s have the same name; the game sees one of them",
                   m_index[i - 1].path, m_index[i].path);
    PC_Trace("indexed %d files under %s", m_nindex, Root());
}

bool Assets::Find(const char *discName, char *path, int n) const {
    char base[MAX_PATH];
    IndexEntry key;
    BaseName(discName, base, sizeof base);
    key.name = base;
    const IndexEntry *e =
        (const IndexEntry *)bsearch(&key, m_index, m_nindex, sizeof *m_index, CmpIndex);
    if (!e)
        return false;
    snprintf(path, n, "%s", e->path);
    return true;
}

bool Assets::FindFilm(const char *discName, char *path, int n) const {
    char base[MAX_PATH], film[80];
    BaseName(discName, base, sizeof base);
    if (Find(base, path, n))
        return true;
    FilmName(base, film, sizeof film);
    return Find(film, path, n);
}

void Assets::Init() {
    StrFile e[kMovieCount];
    char path[MAX_PATH];
    int n = ListStrFiles(e);
    /* the disc the files come from: what Manifest.txt says, else (first startup) the PS1 image
     * if there is one, else a Dreamcast .gdi */
    m_dc = ManifestIsDreamcast();
    if (!m_dc && !WadComplete() && !DiscImage::FindImage(path, sizeof path)) {
        if (!FindGdi(path, sizeof path))
            PC_Fatal("The game data has to be extracted from your disc first, and no disc image "
                     "was found. Put a raw .bin image (MODE2/2352) of Tony Hawk's Pro Skater "
                     "(USA) for the PlayStation next to " PC_EXE_NAME " as thps1.bin, or the .gdi "
                     "and track files of the Dreamcast version next to " PC_EXE_NAME "; or set "
                     "THPS_DISC to either.");
        m_dc = true;
    }
    if (m_dc) {
        ExtractDreamcast(e, n);
        WritePlaylist(e, n);
        g_discImage.SetNone(true);
    } else {
        ExtractStrFiles(e, n);
        ExtractWadFiles();
        WritePlaylist(e, n);
    }
    BuildIndex();
    if (m_dc) {
        DreamcastFixups();
        PC_Log("using the Dreamcast version's files");
    }
}

} // namespace pc

/* ---- the game's interface (pc_assets.h) ---- */

extern "C" {

int PC_ImageLayoutSize(const char *name, int w, int h, int *lw, int *lh) { return pc::g_assets.ImageLayoutSize(name, w, h, lw, lh); }

}

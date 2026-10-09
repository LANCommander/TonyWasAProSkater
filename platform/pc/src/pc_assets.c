/* First-startup extraction of the game's data from the user's disc into Data/ (see pc_assets.h).
 *
 * Which files exist, and what they are, comes from the game itself: the retail executable's
 * movie table (movieDetails, 26 STR files) lists every STR on the disc, and the soundtrack
 * entries are the ones with the 96x64 thumbnail video. The table's order is the order the game
 * cycles the songs in, which becomes the default Data/Music/Playlist.m3u. */
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

int PC_AssetsFind(const char *discName, char *path, int n) {
    static const char *dirs[] = {"Music", "Movies"};
    char base[64], rel[96];
    int i;
    BaseName(discName, base, sizeof base);
    for (i = 0; i < 2; i++) {
        snprintf(rel, sizeof rel, "%s\\%s", dirs[i], base);
        PC_AssetsPath(path, n, rel);
        if (Exists(path))
            return 1;
    }
    return 0;
}

typedef struct {
    char name[64]; /* "SUICIDE.STR" */
    int music;
} Entry;

/* The STR files, in movie-table order. */
static int ListEntries(Entry *e) {
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

static void MakeDir(const char *rel) {
    char p[MAX_PATH];
    CreateDirectoryA(Root(), NULL);
    PC_AssetsPath(p, sizeof p, rel);
    CreateDirectoryA(p, NULL);
}

/* Copy one file's raw sectors out of the disc image. Written to a .part file and renamed, so an
 * interrupted extraction is redone next time instead of leaving a truncated file behind. */
static void Extract(const char *discName, const char *dst) {
    static uint8_t buf[COPY_SECTORS * 2352];
    char part[MAX_PATH];
    uint32_t lba, size, sectors, done;
    FILE *f;
    if (!PC_CdFindDiscFile(discName, &lba, &size))
        PC_Fatal("%s is not on the disc image. Is it Tony Hawk's Pro Skater (USA)?", discName);
    sectors = (size + 2047) / 2048;
    snprintf(part, sizeof part, "%s.part", dst);
    f = fopen(part, "wb");
    if (!f)
        PC_Fatal("cannot create %s", part);
    for (done = 0; done < sectors;) {
        uint32_t n = sectors - done < COPY_SECTORS ? sectors - done : COPY_SECTORS;
        if (!PC_CdReadDiscSectors(lba + done, n, buf))
            PC_Fatal("cannot read %s from the disc image (sector %u)", discName, lba + done);
        if (fwrite(buf, 2352, n, f) != n)
            PC_Fatal("cannot write %s (disk full?)", part);
        done += n;
    }
    if (fclose(f) || !MoveFileExA(part, dst, MOVEFILE_REPLACE_EXISTING))
        PC_Fatal("cannot write %s", dst);
}

static long long FileSize(const char *p) {
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(p, GetFileExInfoStandard, &a))
        return 0;
    return (long long)a.nFileSizeHigh << 32 | a.nFileSizeLow;
}

/* The default playlist: the songs in the order the game cycles them. Left alone once it exists,
 * so the player's edits stick. */
static void WritePlaylist(const Entry *e, int n) {
    char path[MAX_PATH], rel[96], song[MAX_PATH], part[MAX_PATH];
    FILE *f;
    int i, k;
    PC_AssetsPath(path, sizeof path, "Music\\Playlist.m3u");
    if (Exists(path))
        return;
    snprintf(part, sizeof part, "%s.part", path);
    f = fopen(part, "w");
    if (!f)
        PC_Fatal("cannot create %s", part);
    fputs("#EXTM3U\n", f);
    for (i = 0; i < n; i++) {
        const char *title = e[i].name;
        if (!e[i].music)
            continue;
        for (k = 0; k < (int)(sizeof s_credits / sizeof *s_credits); k++)
            if (!_stricmp(s_credits[k].file, e[i].name))
                title = s_credits[k].title;
        snprintf(rel, sizeof rel, "Music\\%s", e[i].name);
        PC_AssetsPath(song, sizeof song, rel);
        fprintf(f, "#EXTINF:%d,%s\n%s\n", (int)(FileSize(song) / 2352 / STREAM_RATE), title,
                e[i].name);
    }
    if (fclose(f) || !MoveFileExA(part, path, MOVEFILE_REPLACE_EXISTING))
        PC_Fatal("cannot write %s", path);
    PC_Log("wrote %s", path);
}

void PC_AssetsInit(void) {
    Entry e[MOVIE_COUNT];
    char path[MAX_PATH], rel[96];
    int n = ListEntries(e), i, missing = 0, done = 0;
    for (i = 0; i < n; i++) {
        snprintf(rel, sizeof rel, "%s\\%s", e[i].music ? "Music" : "Movies", e[i].name);
        PC_AssetsPath(path, sizeof path, rel);
        missing += !Exists(path);
    }
    if (missing) {
        PC_Log("extracting %d files from the disc to %s", missing, Root());
        MakeDir("Music");
        MakeDir("Movies");
        for (i = 0; i < n; i++) {
            char disc[80];
            snprintf(rel, sizeof rel, "%s\\%s", e[i].music ? "Music" : "Movies", e[i].name);
            PC_AssetsPath(path, sizeof path, rel);
            if (Exists(path))
                continue;
            snprintf(disc, sizeof disc, "\\%s;1", e[i].name);
            PC_Log("  [%d/%d] %s", ++done, missing, rel);
            Extract(disc, path);
        }
    }
    MakeDir("Music");
    WritePlaylist(e, n);
}

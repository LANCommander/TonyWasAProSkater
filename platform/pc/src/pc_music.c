/* The soundtrack order comes from Data/Music/Playlist.m3u (written on first startup, see
 * pc_assets.c) instead of the retail rotation through the movie table.
 *
 * FUN_80023168 (GameFMV_SetStartTrack) picks what the next StartFMV streams: a negative argument
 * is a fixed movie-table entry (~arg; the movies, and the songs the front end previews), anything
 * else means "the next song", which the retail code takes from its CycleTracks cursor. Here the
 * next song is the next playlist entry instead. The fixed entries keep using the movie table.
 *
 * A playlist entry is a file of raw STR sectors (relative to Data/Music, or absolute). Files the
 * movie table knows take its settings (video size, end frame, volume); any other file plays with
 * the settings of the table's first song and loops at its last video frame.
 *
 * Entries can also be Dreamcast films (.sfd, pc_sfd.h): "goldfing.sfd" takes GOLDFING.STR's
 * settings. Their audio plays through the same CD stream (hle_libcd.c), which reports the end of
 * a song's audio here, and the song starts over as the PS1's does at its end frame. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_assets.h"
#include "pc_hle.h"
#include "pc_platform.h"
#include "pc_runtime.h"

#define MOVIE_DETAILS 0x800a63e8u
#define MOVIE_COUNT 26
#define MUSIC_WIDTH 96
#define MAX_SONGS 256

/* movieDetails entry (decomp/src/unnamed/80023168.c) */
typedef struct {
    uint32_t name;
    uint16_t width, height, endFrame, pad;
    int32_t vlcBufferSize, ringBufferSectors, is24Bit, startSector, musicScale;
} MovieDetails;

#define GameFMV_Active (*(int32_t *)PSX_PTR(0x800d1634))
#define GameFMV_CurrentTrack (*(uint8_t *)PSX_PTR(0x800d167d))
#define GameFMV_Width (*(uint16_t *)PSX_PTR(0x800d1672))
#define GameFMV_Height (*(uint16_t *)PSX_PTR(0x800d1674))
#define GameFMV_EndFrame (*(int32_t *)PSX_PTR(0x800d1660))
#define GameFMV_VLCBufferSize (*(int32_t *)PSX_PTR(0x800d1664))
#define GameFMV_RingBufferSectors (*(int32_t *)PSX_PTR(0x800d1668))
#define GameFMV_Is24Bit (*(uint8_t *)PSX_PTR(0x800d167c))
#define GameFMV_MusicScale (*(int32_t *)PSX_PTR(0x800d1680))
#define GameFMV_StartSector (*(int32_t *)PSX_PTR(0x800d165c))
#define GameFMV_IsMovie (*(int32_t *)PSX_PTR(0x800d162c))
#define GameFMV_RewindSwitch (*(int32_t *)PSX_PTR(0x800d1630))

static char *s_songs[MAX_SONGS];
static int s_nsongs, s_next, s_loaded;

static void LoadPlaylist(void) {
    char path[MAX_PATH], line[MAX_PATH];
    FILE *f;
    s_loaded = 1;
    PC_AssetsPath(path, sizeof path, "Music\\Playlist.m3u");
    f = fopen(path, "r");
    if (!f) {
        PC_Log("no %s: songs play in the original order", path);
        return;
    }
    while (s_nsongs < MAX_SONGS && fgets(line, sizeof line, f)) {
        char *p = line, *e;
        if (!memcmp(p, "\xef\xbb\xbf", 3)) /* UTF-8 BOM */
            p += 3;
        while (*p == ' ' || *p == '\t')
            p++;
        e = p + strlen(p);
        while (e > p && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ' || e[-1] == '\t'))
            *--e = 0;
        if (!*p || *p == '#')
            continue;
        s_songs[s_nsongs++] = _strdup(p);
    }
    fclose(f);
    PC_Log("playlist %s: %d songs", path, s_nsongs);
}

static const MovieDetails *Entry(int i) { return PSX_PTR(MOVIE_DETAILS + i * sizeof(MovieDetails)); }

static int IsSfd(const char *name) {
    size_t n = strlen(name);
    return n > 4 && !_stricmp(name + n - 4, ".sfd");
}

/* The movie-table entry for a file name, or -1. A film matches the STR of the same name. */
static int FindEntry(const char *song) {
    const char *base = song, *p;
    char name[64];
    int i, len;
    for (p = song; *p; p++)
        if (*p == '\\' || *p == '/')
            base = p + 1;
    len = (int)strlen(base) - (IsSfd(base) ? 4 : 0);
    for (i = 0; i < MOVIE_COUNT; i++) {
        const char *n = Entry(i)->name ? PSX_PTR(Entry(i)->name) : "", *semi;
        while (*n == '\\')
            n++;
        semi = strchr(n, ';');
        snprintf(name, sizeof name, "%.*s", semi ? (int)(semi - n) : (int)strlen(n), n);
        if (IsSfd(base) && strrchr(name, '.'))
            *strrchr(name, '.') = 0;
        if ((int)strlen(name) == len && !_strnicmp(name, base, (size_t)len))
            return i;
    }
    return -1;
}

/* The frame number of a mounted STR's last video frame, or 0. */
static int LastFrame(uint32_t lba, uint32_t sectors) {
    uint8_t raw[2352];
    uint32_t i;
    for (i = sectors; i-- > 0 && sectors - i < 1024;) {
        const uint8_t *u = raw + 24;
        if (!PC_CdReadRawSector(lba + i, raw) || (raw[18] & 0x04))
            continue;
        if ((u[0] | u[1] << 8) == 0x0160)
            return u[8] | u[9] << 8 | u[10] << 16 | u[11] << 24;
    }
    return 0;
}

/* Set up the next playable playlist entry; 0 if there is none. */
static int NextSong(void) {
    int tries, i;
    for (tries = 0; tries < s_nsongs; tries++) {
        const char *song = s_songs[s_next];
        char path[MAX_PATH];
        uint32_t lba, sectors;
        const MovieDetails *m;
        int entry;
        s_next = (s_next + 1) % s_nsongs;
        if (strchr(song, ':') || song[0] == '\\' || song[0] == '/') {
            snprintf(path, sizeof path, "%s", song);
        } else {
            char rel[MAX_PATH];
            snprintf(rel, sizeof rel, "Music\\%s", song);
            PC_AssetsPath(path, sizeof path, rel);
        }
        if (!PC_CdMountFile(path, &lba, &sectors) || !sectors) {
            PC_Log("playlist: cannot open %s, skipped", path);
            continue;
        }
        entry = FindEntry(song);
        if (entry < 0 || Entry(entry)->width != MUSIC_WIDTH) {
            for (i = 0; i < MOVIE_COUNT && Entry(i)->width != MUSIC_WIDTH; i++)
                ;
            if (i == MOVIE_COUNT)
                return 0;
            m = Entry(i);
            GameFMV_CurrentTrack = (uint8_t)i;
            /* a film's song loops at the end of its audio (PC_MusicStreamEnded) */
            GameFMV_EndFrame = IsSfd(song) ? 0x7fffffff : LastFrame(lba, sectors);
            GameFMV_MusicScale = 128;
        } else {
            m = Entry(entry);
            GameFMV_CurrentTrack = (uint8_t)entry;
            GameFMV_EndFrame = m->endFrame;
            GameFMV_MusicScale = m->musicScale;
        }
        GameFMV_Width = m->width;
        GameFMV_Height = m->height;
        GameFMV_VLCBufferSize = m->vlcBufferSize;
        GameFMV_RingBufferSectors = m->ringBufferSectors;
        GameFMV_Is24Bit = (uint8_t)m->is24Bit;
        GameFMV_StartSector = (int32_t)lba;
        PC_Trace("playlist: %s (lba %u, end frame %d)", song, lba, GameFMV_EndFrame);
        return 1;
    }
    return 0;
}

void PC_MusicStreamEnded(void) {
    if (GameFMV_Active && !GameFMV_IsMovie)
        GameFMV_RewindSwitch = 1;
}

void hleov_80023168(int track) {
    extern void pcfn_80023168(int);
    if (GameFMV_Active)
        return;
    if (!s_loaded)
        LoadPlaylist();
    if (track < 0 || !NextSong())
        pcfn_80023168(track);
}

/* The soundtrack order comes from Data/Music/Playlist.m3u (written on first startup, see
 * pc_assets.c) instead of the retail rotation through the movie table.
 *
 * GameFMV_SetStartTrack (game/src/gamefmv.cpp) picks what the next StartFMV streams: a
 * negative argument is a fixed movie-table entry (~arg; the movies, and the songs the front end
 * previews), anything else means "the next song", which the retail code takes from its CycleTracks
 * cursor. It asks PC_MusicNextSong for the next playlist entry instead. The fixed
 * entries keep using the movie table.
 *
 * A playlist entry is a file of raw STR sectors (relative to Data/Music, or absolute). Files the
 * movie table knows take its settings (video size, end frame, volume); any other file plays with
 * the settings of the table's first song and loops at its last video frame.
 *
 * Entries can also be Dreamcast films (.sfd, pc_sfd.h): "goldfing.sfd" takes GOLDFING.STR's
 * settings. Their audio plays through the same stream (pc_stream.c), which reports the end of a
 * song's audio here, and the song starts over as the PS1's does at its end frame. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_assets.h"
#include "pc_gamedata.h"
#include "pc_platform.h"
#include "pc_runtime.h"
#include "pc_stream.h"

#define MOVIE_COUNT GAME_MOVIE_COUNT
#define MUSIC_WIDTH 96
#define MAX_SONGS 256

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

static const GameMovieDetails *Entry(int i) { return &movieDetails[i]; }

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
        const char *n = Entry(i)->name ? Entry(i)->name : "", *semi;
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

int PC_MusicNextSong(void) {
    int tries, i;
    if (!s_loaded)
        LoadPlaylist();
    for (tries = 0; tries < s_nsongs; tries++) {
        const char *song = s_songs[s_next];
        char path[MAX_PATH];
        const GameMovieDetails *m;
        int entry, id;
        s_next = (s_next + 1) % s_nsongs;
        if (strchr(song, ':') || song[0] == '\\' || song[0] == '/') {
            snprintf(path, sizeof path, "%s", song);
        } else {
            char rel[MAX_PATH];
            snprintf(rel, sizeof rel, "Music\\%s", song);
            PC_AssetsPath(path, sizeof path, rel);
        }
        if (!(id = PC_StreamFile(path))) {
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
            GameFMV_CurrentTrack[0] = (uint8_t)i;
            /* a film's song loops at the end of its audio (PC_MusicStreamEnded) */
            GameFMV_EndFrame[0] = IsSfd(song) ? 0x7fffffff : PC_StreamLastFrame(id);
            GameFMV_MusicScale[0] = 128;
        } else {
            m = Entry(entry);
            GameFMV_CurrentTrack[0] = (uint8_t)entry;
            GameFMV_EndFrame[0] = m->endframe;
            GameFMV_MusicScale[0] = m->musicScale;
        }
        GameFMV_Width[0] = m->width;
        GameFMV_Height[0] = m->height;
        GameFMV_VLCBufferSize[0] = m->vlcbuffersize;
        GameFMV_RingBufferSectors[0] = m->ringbuffersectors;
        GameFMV_Is24Bit[0] = (uint8_t)m->is24bit;
        GameFMV_StartSector[0] = id; /* the stream, where the PS1 kept the file's position */
        PC_Trace("playlist: %s (stream %d, end frame %d)", song, id, GameFMV_EndFrame[0]);
        return 1;
    }
    return 0;
}

void PC_MusicStreamEnded(void) {
    if (GameFMV_Active[0] && !GameFMV_IsMovie[0])
        GameFMV_RewindSwitch[0] = 1;
}

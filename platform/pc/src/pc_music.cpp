/* The soundtrack order comes from Data/Music/Playlist.m3u (written on first startup, see
 * pc_assets.cpp) instead of the retail rotation through the movie table.
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
 * settings. Their audio plays through the same stream (pc_stream.cpp), which reports the end of a
 * song's audio here, and the song starts over as the PS1's does at its end frame. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/playlist.h"
#include "pc/stream.h"
#include "pc_assets.h"
#include "pc/game_data.h"
#include "pc/platform.h"
#include "pc_runtime.h"
#include "pc_stream.h"
#include "pc/assets.h"

namespace pc {

Playlist g_playlist;

namespace {

constexpr int kMovieCount = GAME_MOVIE_COUNT;
constexpr int kMusicWidth = 96;

const GameMovieDetails *Entry(int i) { return &movieDetails[i]; }

bool IsSfd(const char *name) {
    size_t n = strlen(name);
    return n > 4 && !_stricmp(name + n - 4, ".sfd");
}

/* The movie-table entry for a file name, or -1. A film matches the STR of the same name. */
int FindEntry(const char *song) {
    const char *base = song, *p;
    char name[64];
    int i, len;
    for (p = song; *p; p++)
        if (*p == '\\' || *p == '/')
            base = p + 1;
    len = (int)strlen(base) - (IsSfd(base) ? 4 : 0);
    for (i = 0; i < kMovieCount; i++) {
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

} // namespace

void Playlist::Load() {
    char path[MAX_PATH], line[MAX_PATH];
    FILE *f;
    m_loaded = true;
    g_assets.Path(path, sizeof path, "Music\\Playlist.m3u");
    f = fopen(path, "r");
    if (!f) {
        PC_Log("no %s: songs play in the original order", path);
        return;
    }
    while (m_nsongs < kMaxSongs && fgets(line, sizeof line, f)) {
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
        m_songs[m_nsongs++] = _strdup(p);
    }
    fclose(f);
    PC_Log("playlist %s: %d songs", path, m_nsongs);
}

bool Playlist::NextSong() {
    int tries, i;
    if (!m_loaded)
        Load();
    for (tries = 0; tries < m_nsongs; tries++) {
        const char *song = m_songs[m_next];
        char path[MAX_PATH];
        const GameMovieDetails *m;
        int entry, id;
        m_next = (m_next + 1) % m_nsongs;
        if (strchr(song, ':') || song[0] == '\\' || song[0] == '/') {
            snprintf(path, sizeof path, "%s", song);
        } else {
            char rel[MAX_PATH];
            snprintf(rel, sizeof rel, "Music\\%s", song);
            g_assets.Path(path, sizeof path, rel);
        }
        if (!(id = g_streamer.File(path))) {
            PC_Log("playlist: cannot open %s, skipped", path);
            continue;
        }
        entry = FindEntry(song);
        if (entry < 0 || Entry(entry)->width != kMusicWidth) {
            for (i = 0; i < kMovieCount && Entry(i)->width != kMusicWidth; i++)
                ;
            if (i == kMovieCount)
                return false;
            m = Entry(i);
            GameFMV_CurrentTrack[0] = (uint8_t)i;
            /* a film's song loops at the end of its audio (StreamEnded) */
            GameFMV_EndFrame[0] = IsSfd(song) ? 0x7fffffff : g_streamer.LastFrame(id);
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
        return true;
    }
    return false;
}

void Playlist::StreamEnded() {
    if (GameFMV_Active[0] && !GameFMV_IsMovie[0])
        GameFMV_RewindSwitch[0] = 1;
}

} // namespace pc

/* ---- the game's interface (pc_stream.h) ---- */

int PC_MusicNextSong(void) { return pc::g_playlist.NextSong(); }

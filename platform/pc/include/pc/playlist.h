/* The soundtrack order (src/pc_music.cpp): Data/Music/Playlist.m3u instead of the retail
 * rotation through the movie table. pc_stream.h has the C interface. */
#pragma once

namespace pc {

class Playlist {
public:
    /* GameFMV_SetStartTrack's "next song": sets the GameFMV_* track globals up for the next
     * playable entry of the playlist. false when there is none: the game takes its own next. */
    bool NextSong();
    /* A film's audio ran out, so the song starts over, as a PS1 song does at its end frame. */
    void StreamEnded();

private:
    static constexpr int kMaxSongs = 256;

    void Load();

    char *m_songs[kMaxSongs] = {};
    int m_nsongs = 0, m_next = 0;
    bool m_loaded = false;
};

extern Playlist g_playlist;

} // namespace pc

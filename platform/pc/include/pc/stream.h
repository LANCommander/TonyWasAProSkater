/* Songs and movies, streamed from their files (src/pc_stream.cpp). pc_stream.h describes the
 * streams and has the C interface the game calls. */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include "pc/platform.h" /* MAX_PATH */
#include "pc/xa.h"

namespace pc {

class SfdFilm;

class Streamer {
public:
    /* The stream for a file: the same id each time for the same file. 0 if it can't be opened. */
    int File(const char *path);
    /* The stream for a movie-table name ("\\SUICIDE.STR;1"): its file under Data/, or the
     * Dreamcast film of it (Assets::FindFilm). 0 if there is none. */
    int Find(const char *discName);
    const char *Path(int id) const;
    /* A Dreamcast film (no STR sectors: its pictures come from pc_movie.cpp). */
    bool IsFilm(int id) const;
    /* The number of an STR's last video frame (0 if none, or a film). */
    int LastFrame(int id) const;

    /* Play a stream from the start (the game's strKickCD: the start of a song, a movie, or a song
     * looping). Any frames not taken yet are dropped. */
    void Play(int id);
    /* Stop reading (the stream is over until the next Play). */
    void Stop();
    /* Pause (the game paused, or the sound menu): no reading, no frames; Resume carries on. */
    void Pause();
    void Resume();

    /* The oldest whole video frame, once the music has played up to it: true and its MDEC
     * bitstream and its 32-byte STR sector header (the frame number is header word 2), until
     * FreeFrame(frame). false if none is ready. */
    bool NextFrame(uint32_t **frame, uint32_t **header);
    void FreeFrame(const void *frame);

    /* Still reading, or frames waiting (the movie player's waits). */
    bool Busy() const;
    /* Seconds of the playing film's audio heard so far (0 before it starts, or if no film
     * plays). */
    double FilmTime() const;

    /* The game's music levels (pc_stream.h's PC_StreamMix), applied to the playing stream. */
    void Mix(const uint8_t levels[4]);

    /* Once per frame (pc_frame.cpp): read what the stream would have read since the last. */
    void Frame();

private:
    static constexpr int kSector = 2352;
    static constexpr int kMaxFrames = 16; /* room for the frames waiting on the music */
    static constexpr int kMaxFiles = 256;
    static constexpr int kFilmChunk = 1024;

    struct FileEntry {
        char path[MAX_PATH];
        bool film;
    };

    /* a video frame being assembled from its sectors, or waiting to be taken */
    enum class FrameState { Free, Filling, Ready, Taken };
    struct VideoFrame {
        FrameState state;
        uint8_t *buf; /* the sector header, then the frame's bitstream */
        uint32_t cap;
        uint32_t frame, seq;
        int chunks, got;
        uint32_t sector; /* the sector that completed it */
        bool timed;      /* due is known (some audio had been read) */
        uint64_t due;    /* the music clock (Sound::MusicConsumed) at which it shows */
    };

    const FileEntry *FileOf(int id) const;
    void ClearFrames();
    VideoFrame *AllocFrame(uint32_t need);
    void VideoChunk(const uint8_t *u);
    void Close();
    void ApplyMix();
    void FilmAudio();
    void TimeFrame(VideoFrame &f) const;
    void AudioClock();

    FileEntry m_files[kMaxFiles] = {};
    int m_nfiles = 0;

    /* the stream */
    int m_id = 0;              /* playing (or paused), 0 for none */
    FILE *m_f = nullptr;       /* an STR */
    SfdFilm *m_sfd = nullptr;  /* a film (owned) */
    bool m_reading = false, m_paused = false;
    uint32_t m_owed = 0;       /* sectors owed, in sixtieths */
    uint32_t m_sector = 0;     /* sectors read since the start */
    uint8_t m_mix[4] = {};     /* the game's levels (CdMix order) */

    /* its video frames */
    VideoFrame m_fr[kMaxFrames] = {};
    uint32_t m_seq = 0;
    int m_filling = -1;

    /* an STR's audio: its last audio sector, and where on the music clock that sector's audio
     * starts (an audio sector plays from when it is read, as on the PS1) */
    bool m_audioSeen = false;
    uint32_t m_audioSector = 0;
    uint64_t m_audioPos = 0;
    int m_audioRate = 0;
    uint64_t m_lastConsumed = 0;
    int m_clockStall = 0;      /* frames the music clock hasn't moved with frames waiting on it */

    /* a film's audio */
    uint64_t m_heardFrom = 0;  /* the mixer's frame count at which its first frame plays */
    int m_filmRate = 0;

    /* read buffers */
    uint8_t m_raw[kSector] = {};
    int16_t m_xaPcm[XaDecoder::kMaxFrames * 2] = {};
    int16_t m_filmPcm[kFilmChunk * 2] = {};
};

extern Streamer g_streamer;

} // namespace pc

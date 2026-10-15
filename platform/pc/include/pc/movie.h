/* Full-screen movies (src/pc_movie.cpp): the STR movies' screen, which the game's own player
 * decodes into, and the Dreamcast films' pictures through pl_mpeg. pc_movie.h has the C
 * interface the game calls. */
#pragma once

#include <stdint.h>

#include "pl_mpeg/pl_mpeg.h" /* declarations only (pc_movie.cpp compiles its implementation) */

namespace pc {

class SfdFilm;

class MoviePlayer {
public:
    /* true if GameFMV_PlayMovie(track) should play movie-table entry ~track: its file is here (an
     * STR or a Dreamcast film) and THPS_MOVIES isn't 0. Otherwise the movie is skipped, as when
     * the player presses a button through one. */
    bool Plays(int track) const;

    /* An STR movie (the game's own player): wait a frame for the stream to deliver the next video
     * frame. false once the stream has stopped with no frame left to come. */
    bool Wait();

    /* An STR movie's screen: black, 320x240 pixels of 24 bits (rgb24) or 15. Decode decodes a
     * frame (runlevel: the game's VLC output) into it at x (halfwords, as in the PS1's screen
     * buffer), y, its columns to xend, h high; Show shows the screen from the next frame on; End
     * goes back to the game's screen. */
    void Begin(bool rgb24);
    void Decode(const void *runlevel, int x, int y, int xend, int h);
    void Show() { ScreenShow(); }
    void End();

    /* Open the film the stream plays (GameFMV_StartSector, pc_stream.h): false if it isn't one. */
    bool Open();
    /* Show the picture that is due and wait for the next frame (which shows it). false once the
     * film has ended. */
    bool Frame();
    void Close(); /* and back to the game's screen */

private:
    static constexpr int kScreenW = 320;
    static constexpr int kScreenH = 240;

    static void OnVideo(void *ctx, const uint8_t *p, int n);
    static void LoadVideo(plm_buffer_t *b, void *user);
    void ScreenShow();
    void ShowPicture(plm_frame_t *f);

    /* the screen: rows of kScreenW * 3 bytes, 24-bit pixels or (m_rgb24 false) 15-bit halfwords */
    uint8_t m_screen[kScreenH][kScreenW * 3] = {};
    bool m_rgb24 = false;
    uint32_t m_argb[kScreenW * kScreenH] = {}; /* the screen as it goes to pc_screen */

    /* a film */
    SfdFilm *m_sfd = nullptr;       /* owned */
    plm_buffer_t *m_buf = nullptr;
    plm_video_t *m_vid = nullptr;
    bool m_gotVideo = false;
    plm_frame_t *m_next = nullptr;  /* decoded, not shown yet */
    double m_clock = 0.0, m_audio = 0.0;
    int m_stall = 0;
    uint8_t *m_rgb = nullptr;       /* a picture in 24 bits, before it goes to the screen */
    int m_rgbCap = 0;
};

extern MoviePlayer g_moviePlayer;

} // namespace pc

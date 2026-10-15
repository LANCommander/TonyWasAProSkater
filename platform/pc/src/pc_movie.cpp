/* Movie pictures from a Dreamcast film (see pc_movie.h): the film's video stream, demultiplexed
 * by pc_sfd.cpp, goes to pl_mpeg's MPEG-1 video decoder (include/pl_mpeg, MIT). Each picture is
 * shown when the film's audio, which its stream is playing (Streamer::FilmTime), reaches its time;
 * pictures that are late are dropped. Without audio (or once it has ended) the pictures keep time
 * on the frame clock.
 *
 * The pictures are shown as the PS1's 24-bit movie screen was, 320x240 at 4:3 (pc_screen.h). A
 * film smaller than that is centred; a larger one is cropped.
 *
 * An STR movie's pictures are decoded (pc_vdec.h) into the same screen, where the PS1 put them in
 * its screen buffer. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/movie.h"
#include "pc/sfd.h"
#include "pc/stream.h"
#include "pc/vdec.h"
#include "pc_assets.h"
#include "pc_frame.h"
#include "pc/game_data.h"
#include "pc_movie.h"
#include "pc/platform.h"
#include "pc_runtime.h"
#include "pc_screen.h"
#include "pc_vdec.h"
#include "pc/assets.h"
#include "pc/screen.h"

/* pl_mpeg's implementation (pc/movie.h has included its declarations) */
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg/pl_mpeg.h"

namespace pc {

MoviePlayer g_moviePlayer;

namespace {

constexpr int kMovieCount = GAME_MOVIE_COUNT;
constexpr int kAudioStall = 30; /* frames without audio progress before the frame clock takes over */

} // namespace

void MoviePlayer::ScreenShow() {
    int x, y;
    for (y = 0; y < kScreenH; y++)
        for (x = 0; x < kScreenW; x++) {
            const uint8_t *p = m_screen[y];
            uint32_t c;
            if (m_rgb24) {
                c = (uint32_t)p[x * 3] << 16 | (uint32_t)p[x * 3 + 1] << 8 | p[x * 3 + 2];
            } else {
                uint32_t h = p[x * 2] | (uint32_t)p[x * 2 + 1] << 8, r = h & 31,
                         g = (h >> 5) & 31, b = (h >> 10) & 31;
                c = (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
            }
            m_argb[y * kScreenW + x] = 0xff000000u | c;
        }
    g_screens.ShowImage(m_argb, kScreenW, kScreenH);
}

void MoviePlayer::Begin(bool rgb24) {
    memset(m_screen, 0, sizeof m_screen);
    m_rgb24 = rgb24;
}

void MoviePlayer::Decode(const void *runlevel, int x, int y, int xend, int h) {
    int colw = m_rgb24 ? 24 : 16, cols = (xend - x + colw - 1) / colw;
    if (x < 0 || y < 0 || (x + cols * colw) * 2 > kScreenW * 3 || y + h > kScreenH)
        return;
    g_videoDecoder.Frame(runlevel, m_rgb24 ? PC_VDEC_RGB24 : PC_VDEC_15STP, cols, h,
                         &m_screen[y][x * 2], sizeof m_screen[0]);
}

void MoviePlayer::End() {
    g_screens.ShowGame();
}

bool MoviePlayer::Plays(int track) const {
    const char *e = getenv("THPS_MOVIES");
    char path[260];
    if ((!e || atoi(e) != 0) && track < 0 && ~track < kMovieCount) {
        const char *name = movieDetails[~track].name;
        if (name && g_assets.FindFilm(name, path, sizeof path))
            return true;
    }
    PC_Log("movie %d skipped", track);
    return false;
}

bool MoviePlayer::Wait() {
    PC_FrameWait(1);
    return g_streamer.Busy();
}

void MoviePlayer::OnVideo(void *ctx, const uint8_t *p, int n) {
    MoviePlayer *self = (MoviePlayer *)ctx;
    plm_buffer_write(self->m_buf, (uint8_t *)p, (size_t)n);
    self->m_gotVideo = true;
}

/* The decoder wants more of the stream: demultiplex until some video comes. */
void MoviePlayer::LoadVideo(plm_buffer_t *b, void *user) {
    MoviePlayer *self = (MoviePlayer *)user;
    self->m_gotVideo = false;
    while (!self->m_gotVideo)
        if (!self->m_sfd->Demux()) {
            plm_buffer_signal_end(b);
            return;
        }
}

bool MoviePlayer::Open() {
    int id = GameFMV_StartSector[0]; /* the stream (pc_stream.h) */
    const char *path = g_streamer.Path(id);
    if (!g_streamer.IsFilm(id) || !(m_sfd = SfdFilm::Open(path).release()))
        return false;
    m_sfd->SetAudio(false);
    m_sfd->SetVideo(OnVideo, this);
    m_buf = plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
    plm_buffer_set_load_callback(m_buf, LoadVideo, this);
    m_vid = plm_video_create_with_buffer(m_buf, 1);
    m_next = nullptr;
    m_clock = m_audio = 0.0;
    m_stall = 0;
    Begin(true);
    ScreenShow();
    PC_Log("movie %s", path);
    return true;
}

void MoviePlayer::ShowPicture(plm_frame_t *f) {
    int w = (int)f->width, h = (int)f->height, x0 = 0, y0 = 0, sx = 0, sy = 0, cw, ch, y;
    if (m_rgbCap < w * h * 3) {
        m_rgbCap = w * h * 3;
        m_rgb = (uint8_t *)realloc(m_rgb, (size_t)m_rgbCap);
    }
    plm_frame_to_rgb(f, m_rgb, w * 3);
    if (w < kScreenW)
        x0 = (kScreenW - w) / 2 & ~1;       /* whole VRAM halfwords */
    else
        sx = (w - kScreenW) / 2;
    if (h < kScreenH)
        y0 = (kScreenH - h) / 2;
    else
        sy = (h - kScreenH) / 2;
    cw = w - sx < kScreenW - x0 ? w - sx : kScreenW - x0;
    ch = h - sy < kScreenH - y0 ? h - sy : kScreenH - y0;
    for (y = 0; y < ch; y++)
        memcpy(&m_screen[y0 + y][x0 * 3], m_rgb + ((sy + y) * w + sx) * 3, (size_t)cw * 3);
    ScreenShow();
}

bool MoviePlayer::Frame() {
    double a, step = 1.0 / plm_video_get_framerate(m_vid);
    if (!m_sfd)
        return false;
    if (!m_next)
        m_next = plm_video_decode(m_vid);
    if (!m_next)
        return false;
    /* late: skip to the last picture that is due */
    while (m_next->time + step <= m_clock) {
        plm_frame_t *f = plm_video_decode(m_vid);
        if (!f)
            break;
        m_next = f;
    }
    if (m_next->time <= m_clock) {
        ShowPicture(m_next);
        m_next = nullptr;
    }
    PC_FrameWait(1);
    a = g_streamer.FilmTime();
    if (a > m_audio) {
        m_audio = m_clock = a;
        m_stall = 0;
    } else if (++m_stall > kAudioStall) {
        /* no audio, or it has ended: frames keep the time */
        m_clock += 1.0 / 60.0;
    }
    return true;
}

void MoviePlayer::Close() {
    if (m_vid)
        plm_video_destroy(m_vid);
    m_vid = nullptr;
    m_buf = nullptr;
    delete m_sfd;
    m_sfd = nullptr;
    End();
}

} // namespace pc

/* ---- the game's interface (pc_movie.h) ---- */

int PC_MoviePlays(int track) { return pc::g_moviePlayer.Plays(track); }
int PC_MovieWait(void) { return pc::g_moviePlayer.Wait(); }
void PC_MovieBegin(int rgb24) { pc::g_moviePlayer.Begin(rgb24 != 0); }
void PC_MovieDecode(const void *runlevel, int x, int y, int xend, int h) {
    pc::g_moviePlayer.Decode(runlevel, x, y, xend, h);
}
void PC_MovieShow(void) { pc::g_moviePlayer.Show(); }
void PC_MovieEnd(void) { pc::g_moviePlayer.End(); }
int PC_MovieOpen(void) { return pc::g_moviePlayer.Open(); }
int PC_MovieFrame(void) { return pc::g_moviePlayer.Frame(); }
void PC_MovieClose(void) { pc::g_moviePlayer.Close(); }

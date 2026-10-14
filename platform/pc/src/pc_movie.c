/* Movie pictures from a Dreamcast film (see pc_movie.h): the film's video stream, demultiplexed
 * by pc_sfd.c, goes to pl_mpeg's MPEG-1 video decoder (include/pl_mpeg, MIT). Each picture is
 * shown when the film's audio, which its stream is playing (PC_StreamFilmTime), reaches its time;
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

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg/pl_mpeg.h"

#include "pc_assets.h"
#include "pc_frame.h"
#include "pc_movie.h"
#include "pc_platform.h"
#include "pc_runtime.h"
#include "pc_screen.h"
#include "pc_sfd.h"
#include "pc_stream.h"
#include "pc_vdec.h"

#define MOVIE_DETAILS PSX_DATA_ADDR(0x800a63e8u)
#define MOVIE_COUNT 26
#define GAMEFMV_START_SECTOR PSX_DATA_ADDR(0x800d165cu)
#define SCREEN_W 320
#define SCREEN_H 240
#define AUDIO_STALL 30      /* frames without audio progress before the frame clock takes over */

/* the screen: rows of SCREEN_W * 3 bytes, 24-bit pixels or (s_rgb24 = 0) 15-bit halfwords */
static uint8_t s_screen[SCREEN_H][SCREEN_W * 3];
static int s_rgb24;

static void ScreenShow(void) {
    static uint32_t argb[SCREEN_W * SCREEN_H];
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++) {
            const uint8_t *p = s_screen[y];
            uint32_t c;
            if (s_rgb24) {
                c = (uint32_t)p[x * 3] << 16 | (uint32_t)p[x * 3 + 1] << 8 | p[x * 3 + 2];
            } else {
                uint32_t h = p[x * 2] | (uint32_t)p[x * 2 + 1] << 8, r = h & 31,
                         g = (h >> 5) & 31, b = (h >> 10) & 31;
                c = (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
            }
            argb[y * SCREEN_W + x] = 0xff000000u | c;
        }
    PC_ScreenShowImage(argb, SCREEN_W, SCREEN_H);
}

void PC_MovieBegin(int rgb24) {
    memset(s_screen, 0, sizeof s_screen);
    s_rgb24 = rgb24;
}

void PC_MovieDecode(const void *runlevel, int x, int y, int xend, int h) {
    int colw = s_rgb24 ? 24 : 16, cols = (xend - x + colw - 1) / colw;
    if (x < 0 || y < 0 || (x + cols * colw) * 2 > SCREEN_W * 3 || y + h > SCREEN_H)
        return;
    PC_VdecFrame(runlevel, s_rgb24 ? PC_VDEC_RGB24 : PC_VDEC_15STP, cols, h, &s_screen[y][x * 2],
                 sizeof s_screen[0]);
}

void PC_MovieShow(void) {
    ScreenShow();
}

void PC_MovieEnd(void) {
    PC_ScreenShowGame();
}

static PcSfd *s_sfd;
static plm_buffer_t *s_buf;
static plm_video_t *s_vid;
static int s_gotVideo;
static plm_frame_t *s_next;      /* decoded, not shown yet */
static double s_clock, s_audio;
static int s_stall;

int PC_MoviePlays(int track) {
    const char *e = getenv("THPS_MOVIES");
    const uint8_t *d;
    uint32_t name;
    char path[260];
    if ((!e || atoi(e) != 0) && track < 0 && ~track < MOVIE_COUNT) {
        d = PSX_PTR(MOVIE_DETAILS + (uint32_t)~track * 32);
        memcpy(&name, d, 4);
        if (name && PC_AssetsFindFilm(PSX_PTR(name), path, sizeof path))
            return 1;
    }
    PC_Log("movie %d skipped", track);
    return 0;
}

int PC_MovieWait(void) {
    PC_FrameWait(1);
    return PC_StreamBusy();
}

static void OnVideo(void *ctx, const uint8_t *p, int n) {
    (void)ctx;
    plm_buffer_write(s_buf, (uint8_t *)p, (size_t)n);
    s_gotVideo = 1;
}

/* The decoder wants more of the stream: demultiplex until some video comes. */
static void LoadVideo(plm_buffer_t *b, void *user) {
    (void)user;
    s_gotVideo = 0;
    while (!s_gotVideo)
        if (!PC_SfdDemux(s_sfd)) {
            plm_buffer_signal_end(b);
            return;
        }
}

int PC_MovieOpen(void) {
    int id = *(int32_t *)PSX_PTR(GAMEFMV_START_SECTOR); /* the stream (pc_stream.h) */
    const char *path = PC_StreamPath(id);
    if (!PC_StreamIsFilm(id) || !(s_sfd = PC_SfdOpen(path)))
        return 0;
    PC_SfdSetAudio(s_sfd, 0);
    PC_SfdSetVideo(s_sfd, OnVideo, NULL);
    s_buf = plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
    plm_buffer_set_load_callback(s_buf, LoadVideo, NULL);
    s_vid = plm_video_create_with_buffer(s_buf, 1);
    s_next = NULL;
    s_clock = s_audio = 0.0;
    s_stall = 0;
    PC_MovieBegin(1);
    ScreenShow();
    PC_Log("movie %s", path);
    return 1;
}

static void Show(plm_frame_t *f) {
    static uint8_t *rgb;
    static int cap;
    int w = (int)f->width, h = (int)f->height, x0 = 0, y0 = 0, sx = 0, sy = 0, cw, ch, y;
    if (cap < w * h * 3) {
        cap = w * h * 3;
        rgb = realloc(rgb, (size_t)cap);
    }
    plm_frame_to_rgb(f, rgb, w * 3);
    if (w < SCREEN_W)
        x0 = (SCREEN_W - w) / 2 & ~1;       /* whole VRAM halfwords */
    else
        sx = (w - SCREEN_W) / 2;
    if (h < SCREEN_H)
        y0 = (SCREEN_H - h) / 2;
    else
        sy = (h - SCREEN_H) / 2;
    cw = w - sx < SCREEN_W - x0 ? w - sx : SCREEN_W - x0;
    ch = h - sy < SCREEN_H - y0 ? h - sy : SCREEN_H - y0;
    for (y = 0; y < ch; y++)
        memcpy(&s_screen[y0 + y][x0 * 3], rgb + ((sy + y) * w + sx) * 3, (size_t)cw * 3);
    ScreenShow();
}

int PC_MovieFrame(void) {
    double a, step = 1.0 / plm_video_get_framerate(s_vid);
    if (!s_sfd)
        return 0;
    if (!s_next)
        s_next = plm_video_decode(s_vid);
    if (!s_next)
        return 0;
    /* late: skip to the last picture that is due */
    while (s_next->time + step <= s_clock) {
        plm_frame_t *f = plm_video_decode(s_vid);
        if (!f)
            break;
        s_next = f;
    }
    if (s_next->time <= s_clock) {
        Show(s_next);
        s_next = NULL;
    }
    PC_FrameWait(1);
    a = PC_StreamFilmTime();
    if (a > s_audio) {
        s_audio = s_clock = a;
        s_stall = 0;
    } else if (++s_stall > AUDIO_STALL) {
        /* no audio, or it has ended: frames keep the time */
        s_clock += 1.0 / 60.0;
    }
    return 1;
}

void PC_MovieClose(void) {
    if (s_vid)
        plm_video_destroy(s_vid);
    s_vid = NULL;
    s_buf = NULL;
    PC_SfdClose(s_sfd);
    s_sfd = NULL;
    PC_MovieEnd();
}

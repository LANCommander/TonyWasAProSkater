/* Movie pictures from a Dreamcast film (see pc_movie.h): the film's video stream, demultiplexed
 * by pc_sfd.c, goes to pl_mpeg's MPEG-1 video decoder (include/pl_mpeg, MIT). Each picture is
 * shown when the film's audio, which the CD stream is playing (PC_CdFilmTime), reaches its time;
 * pictures that are late are dropped. Without audio (or once it has ended) the pictures keep time
 * on the vblank clock.
 *
 * The pictures go where the PS1's MDEC put them, as 24-bit pixels at VRAM (0, 256), which the
 * screen shows in 24-bit mode (GameFMV_PlayMovie sets it up). A film smaller than 320x240 is
 * centred; a larger one is cropped. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg/pl_mpeg.h"

#include "pc_assets.h"
#include "pc_hle.h"
#include "pc_movie.h"
#include "pc_runtime.h"
#include "pc_sfd.h"

#define MOVIE_DETAILS 0x800a63e8u
#define MOVIE_COUNT 26
#define GAMEFMV_START_SECTOR 0x800d165cu
#define SCREEN_W 320
#define SCREEN_H 240
#define SCREEN_Y 256
#define AUDIO_STALL 30      /* vblanks without audio progress before the vblank clock takes over */

typedef struct {
    short x, y, w, h;
} PcRect;

int LoadImage(PcRect *rect, const void *p);   /* hle_libgpu.c */
int VSync(int mode);                          /* hle_libetc.c */

static PcSfd *s_sfd;
static plm_buffer_t *s_buf;
static plm_video_t *s_vid;
static int s_gotVideo;
static uint32_t s_lba;
static plm_frame_t *s_next;      /* decoded, not shown yet */
static uint8_t *s_rgb;           /* the screen's 24-bit pixels */
static double s_clock, s_audio;
static int s_stall;

int PC_MovieAvailable(int track) {
    const uint8_t *d;
    uint32_t name;
    char path[260];
    size_t n;
    if (track >= 0 || ~track >= MOVIE_COUNT)
        return 0;
    d = PSX_PTR(MOVIE_DETAILS + (uint32_t)~track * 32);
    memcpy(&name, d, 4);
    if (!name || !PC_AssetsFindFilm(PSX_PTR(name), path, sizeof path))
        return 0;
    n = strlen(path);
    return n > 4 && !_stricmp(path + n - 4, ".sfd");
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
    uint32_t lba = *(uint32_t *)PSX_PTR(GAMEFMV_START_SECTOR);
    const char *path = PC_CdMountPath(lba);
    size_t n = path ? strlen(path) : 0;
    PcRect r = {0, SCREEN_Y, SCREEN_W * 3 / 2, SCREEN_H};
    if (n < 4 || _stricmp(path + n - 4, ".sfd") || !(s_sfd = PC_SfdOpen(path)))
        return 0;
    PC_SfdSetAudio(s_sfd, 0);
    PC_SfdSetVideo(s_sfd, OnVideo, NULL);
    s_buf = plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
    plm_buffer_set_load_callback(s_buf, LoadVideo, NULL);
    s_vid = plm_video_create_with_buffer(s_buf, 1);
    s_lba = lba;
    s_next = NULL;
    s_clock = s_audio = 0.0;
    s_stall = 0;
    s_rgb = calloc(1, SCREEN_W * SCREEN_H * 3);
    LoadImage(&r, s_rgb);
    PC_Log("movie %s", path);
    return 1;
}

static void Show(plm_frame_t *f) {
    static uint8_t *rgb;
    static int cap;
    int w = (int)f->width, h = (int)f->height, x0 = 0, y0 = 0, sx = 0, sy = 0, cw, ch, y;
    PcRect r = {0, SCREEN_Y, SCREEN_W * 3 / 2, SCREEN_H};
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
        memcpy(s_rgb + ((y0 + y) * SCREEN_W + x0) * 3, rgb + ((sy + y) * w + sx) * 3,
               (size_t)cw * 3);
    LoadImage(&r, s_rgb);
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
    VSync(0);
    a = PC_CdFilmTime(s_lba);
    if (a > s_audio) {
        s_audio = s_clock = a;
        s_stall = 0;
    } else if (++s_stall > AUDIO_STALL) {
        /* no audio, or it has ended: vblanks keep the time */
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
    free(s_rgb);
    s_rgb = NULL;
}

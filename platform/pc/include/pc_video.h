/* The game window (src/pc_video.c): windowed, borderless fullscreen at the desktop's resolution,
 * or exclusive fullscreen in one of the display's modes. With OpenGL the game draws at the
 * window's resolution (pc_gpu_gl.h), so a 1024x768 mode is drawn at 1024x768. Kept in the
 * [Video] section of Settings.ini; Options > Graphics (decomp/src/front/controlleroptions.cpp,
 * PC branch) and Alt+Enter change it. */
#ifndef PC_VIDEO_H
#define PC_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

enum { PC_VIDEO_WINDOWED, PC_VIDEO_BORDERLESS, PC_VIDEO_FULLSCREEN, PC_VIDEO_MODES };

typedef struct {
    int mode;              /* PC_VIDEO_* */
    int win_w, win_h;      /* windowed: the window's drawing area, in pixels */
    int full_w, full_h;    /* fullscreen: the display mode */
    int widescreen;        /* gameplay fills screens wider than 4:3 (pc_gpu_gl.h) */
} PcVideoSettings;

void PC_VideoGetSettings(PcVideoSettings *s);
/* Switch the window to s now (Settings.ini is written by PC_VideoSave). */
void PC_VideoApply(const PcVideoSettings *s);
int PC_VideoSave(void);

/* The resolutions a mode offers, smallest first: the display's modes (windowed: those that fit
 * on the desktop, plus the current window size), or just the desktop's for borderless. */
int PC_VideoResolutionCount(int mode);
void PC_VideoResolution(int mode, int i, int *w, int *h);

/* An SDL_Event from the event pump: Alt+Enter, window resizes. */
void PC_VideoEvent(const void *sdl_event);

#ifdef __cplusplus
}
#endif

#endif

/* The game window (src/pc_video.c): windowed, borderless fullscreen at the desktop's resolution,
 * or exclusive fullscreen in one of the display's modes. With OpenGL the game draws at the
 * window's resolution (pc_gpu_gl.h), so a 1024x768 mode is drawn at 1024x768. The draw distance
 * and fog settings live here too (PC_DrawFarPlane below). Kept in the
 * [Video] section of Settings.ini; Options > Graphics (decomp/src/front/controlleroptions.cpp,
 * PC branch) and Alt+Enter change it. */
#ifndef PC_VIDEO_H
#define PC_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

enum { PC_VIDEO_WINDOWED, PC_VIDEO_BORDERLESS, PC_VIDEO_FULLSCREEN, PC_VIDEO_MODES };

/* distance fog: as the level sets it, only over the far half of its range, or none */
enum { PC_FOG_NORMAL, PC_FOG_LIGHT, PC_FOG_OFF, PC_FOG_MODES };

typedef struct {
    int mode;              /* PC_VIDEO_* */
    int win_w, win_h;      /* windowed: the window's drawing area, in pixels */
    int full_w, full_h;    /* fullscreen: the display mode */
    int widescreen;        /* gameplay fills screens wider than 4:3 (pc_gpu_gl.h) */
    int draw_distance;     /* percent of the level's far plane (and its fog with it), or
                              PC_DISTANCE_FULL */
    int object_distance;   /* percent of the models' level-of-detail and fade-out distances */
    int fog;               /* PC_FOG_* */
} PcVideoSettings;

/* the range Settings.ini allows for draw_distance and object_distance */
#define PC_DISTANCE_MIN 50
#define PC_DISTANCE_MAX 400
/* draw_distance: the entire level (beyond the 16-bit view space; native 3D renderer only) */
#define PC_DISTANCE_FULL 10000

/* Create the window and the renderer (scale = integer window scale of a 320x240 display), and
 * close them. */
void PC_VideoInit(int scale);
void PC_VideoShutdown(void);
/* Nonzero when THPS_HEADLESS=1 suppressed the window. */
int PC_VideoIsHeadless(void);
/* Show the display (or the picture shown instead, pc_screen.h) in the window. Once per frame. */
void PC_VideoPresent(void);
/* Debugging: the screens' and textures' space (1024x512 at the PS1's resolution) or the display
 * area to a .bmp. */
int PC_VideoDumpSpace(const char *path);
int PC_VideoDumpDisplay(const char *path);

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

/* Draw distance, for the game's renderer (decomp/src/m3d, PC_PORT). The level's fog runs from
 * DpqMin to DpqMax, and DpqMax is also the far plane (SViewport.Yon), so the far plane and the
 * fog scale together; the far plane stops at 0x7fff, the most the 16-bit view depths hold.
 * Models switch to their next level of detail, or vanish after the last one, at their own
 * distances (M3d_Render), which the object distance scales. Everything is as retail at 100% and
 * normal fog.
 *
 * The entire level (PC_DrawFull): the far plane stays at 0x7fff for the PS1 transforms, and
 * M3d_Render hands items beyond reach of the 16-bit view space to the native renderer
 * (R3d_FarBegin, pc_render3d.h); level geometry is drawn even where the level's scripts hide it
 * (Downhill Jam shows each section only as the skater nears it) or beyond its last level of
 * detail. The fog then ends at the far side of the level (PC_DrawSetLevelSize). */
int PC_DrawFarPlane(int dpqmax);
/* the fog's start (scratchpad 0x284, so at most 0x7fff) and shift (0x294) for the level's
 * DpqMin/DpqMax/DpqShift */
int PC_DrawFogMin(int dpqmin, int dpqmax);
int PC_DrawFogShift(int dpqmin, int dpqmax, int dpqshift);
/* the fog's start without the 16-bit limit (INT_MAX: no fog), for the native renderer */
int PC_DrawFogStart(int dpqmin, int dpqmax);
/* a model's level-of-detail distance */
int PC_DrawObjectDistance(int d);
/* 1: draw the entire level */
int PC_DrawFull(void);
/* the level's size (the diagonal of what it covers), where the fog ends when drawing it all */
void PC_DrawSetLevelSize(int size);
/* the distance beyond which bodies are suspended (Ob_AI): as far as they can be seen */
unsigned PC_DrawSuspendDistance(unsigned d);

#ifdef __cplusplus
}
#endif

#endif

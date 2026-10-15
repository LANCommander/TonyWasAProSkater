/* The game window (src/pc_video.cpp; the game's interface is pc_video.h): windowed, borderless or
 * exclusive fullscreen, presenting the display through the OpenGL renderer (pc_gpu_gl.h), the
 * [Video] settings, the draw distance and fog they set, and BMP dumps for debugging. */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include "pc_video.h"

struct SDL_Window;

namespace pc {

class Video {
  public:
    /* Create the window and the renderer (scale = integer window scale of a 320x240 display), and
     * close them. */
    void Init(int scale);
    void Shutdown();
    /* THPS_HEADLESS=1 suppressed the window. */
    bool IsHeadless() const { return m_headless; }
    /* Show the display (or the picture shown instead, pc_screen.h) in the window. Once per frame. */
    void Present();
    /* An SDL_Event from the event pump: Alt+Enter, window resizes. */
    void Event(const void *sdlEvent);

    /* Debugging: the screens' and textures' space (1024x512 at the PS1's resolution) or the
     * display area to a .bmp. */
    int DumpSpace(const char *path);
    int DumpDisplay(const char *path);

    /* ---- settings ---- */
    const PcVideoSettings &Settings() const { return m_set; }
    /* Switch the window to s now (Settings.ini is written by Save). */
    void Apply(const PcVideoSettings &s);
    int Save();
    /* Settings.ini's [Video] section (pc_settings.h) */
    void SettingsDefaults();
    int SettingsLine(const char *section, const char *key, char *value);
    void WriteSettings(FILE *f);

    /* The resolutions a mode offers, smallest first. */
    int ResolutionCount(int mode);
    void Resolution(int mode, int i, int *w, int *h);

    /* ---- draw distance (pc_video.h: PC_DrawFarPlane and the rest) ---- */
    int FarPlane(int dpqmax);
    int FogMin(int dpqmin, int dpqmax);
    int FogShift(int dpqmin, int dpqmax, int dpqshift);
    int FogStart(int dpqmin, int dpqmax);
    int ObjectDistance(int d);
    bool Full();
    void SetLevelSize(int size) { m_levelSize = size; }
    unsigned SuspendDistance(unsigned d);

  private:
    void DrawEnv();
    double FogEnd(int dpqmax);
    int ScaledFar(int dpqmax);
    bool FogRetail();
    double FogRange(int dpqmin, int dpqmax);
    int DisplayIndex();
    void ApplyWindow();
    void AddRes(int w, int h);
    bool InitGl(int scale);

    SDL_Window *m_window = nullptr;
    void *m_glctx = nullptr;   /* SDL_GLContext */
    bool m_headless = false;
    bool m_videoInited = false;

    PcVideoSettings m_set = {};
    int m_lastFull = PC_VIDEO_BORDERLESS;   /* where Alt+Enter goes from a window */

    int m_levelSize = 0;       /* SetLevelSize */
    bool m_envDone = false;    /* DrawEnv read the environment */

    struct Res {
        int w, h;
    };
    Res m_res[256] = {};
    int m_nres = 0, m_resMode = -1;

    int m_gpuDumpFrom = -2, m_gpuDumpTo = -2;   /* THPS_GPUDUMP's frames */
};

extern Video g_video;

}   // namespace pc

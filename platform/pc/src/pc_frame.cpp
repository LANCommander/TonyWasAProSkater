/* The frame clock (pc/frame_clock.h). Frames run at 60 Hz off the wall clock, from Wait and from
 * Poll, which the port calls wherever the game waits on something a frame changes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/frame_clock.h"
#include "pc/platform.h"
#include "pc_frame.h"
#include "pc_draw.h"
#include "pc_video.h"
#include "pc_render3d.h"
#include "pc_runtime.h"
#include "pc_stream.h"
#include "pc/draw.h"
#include "pc/input.h"
#include "pc/render3d.h"
#include "pc/stream.h"
#include "pc/video.h"

extern volatile uint32_t pc_heartbeat;

namespace pc {

FrameClock g_frameClock;

int64_t FrameClock::Now() const {
    if (m_deterministic)
        return m_virtualNow;
    return os::ClockNow();
}

void FrameClock::Init() {
    m_deterministic = getenv("THPS_DETERMINISTIC") && atoi(getenv("THPS_DETERMINISTIC"));
    if (m_deterministic)
        m_freq = 1000000;
    else
        m_freq = os::ClockFreq();
    m_next = Now();
    m_unthrottled = getenv("THPS_FAST") && atoi(getenv("THPS_FAST"));
}

/* One frame: input, streaming, the game's frame handler, then show the display area (the game
 * flips buffers from its handler and never waits for a frame while playing). A frame that is late
 * with the next one also due isn't shown when vsync is on: each present then waits for the
 * display's refresh, and the frames that catch up on the clock mustn't. */
void FrameClock::RunFrame(bool present) {
    m_frames++;
    g_streamer.Frame();
    g_otDrawer.RunDone();
    if (m_handler && !m_inHandler) {
        m_inHandler = true;
        m_handler();
        m_inHandler = false;
    }
    if (present || !g_video.VSyncActive())
        g_video.Present();
    pc_heartbeat = pc_heartbeat + 1;
    g_input.PumpEvents();
    DebugHooks();
    if (g_input.QuitRequested())
        os::Exit(0);
}

int FrameClock::Poll() {
    int64_t now;
    int n = 0;
    if (!m_freq)
        Init();
    if (m_deterministic)
        m_virtualNow += 1000;
    now = Now();
    while (now >= m_next && n < 4) {
        m_next += m_freq / kHz;
        RunFrame(now < m_next || n == 3);
        n++;
    }
    if (now - m_next > m_freq / 4)
        m_next = now; /* fell far behind (debugger, loading): don't replay a burst */
    return n;
}

/* Automated runs: THPS_SHOT="f1,f2,..." dumps the display (and VRAM) at those frames to
 * THPS_SHOTDIR (default "."); THPS_FRAMES=n exits after n frames; THPS_POKE="name[+offset]:value,..."
 * stores those words every frame into globals of the game's data (Cheat_LevelSelect:1,
 * GCareer+0x82c:4 = GCareer.LastLevel; PC_DataByName). */
void FrameClock::DebugHooks() {
    char path[512];
    if (m_maxFrames < 0) {
        m_maxFrames = getenv("THPS_FRAMES") ? atoi(getenv("THPS_FRAMES")) : 0;
        m_shots = getenv("THPS_SHOT");
        m_pokes = getenv("THPS_POKE");
    }
    for (const char *p = m_pokes; p && *p;) {
        char name[128], *e;
        size_t n = strcspn(p, "+:,");
        uint32_t off = 0, v = 0;
        char *g;
        snprintf(name, sizeof name, "%.*s", (int)(n < sizeof name ? n : sizeof name - 1), p);
        e = (char *)p + n;
        if (*e == '+')
            off = strtoul(e + 1, &e, 0);
        if (*e == ':')
            v = strtoul(e + 1, &e, 0);
        g = (char *)PC_DataByName(name);
        if (g && !(off & 3))
            *(uint32_t *)(g + off) = v;
        p = strchr(e, ',') ? strchr(e, ',') + 1 : "";
    }
    m_hookFrames++;
    if (m_shots) {
        const char *p = m_shots;
        while (*p) {
            if (atoi(p) == m_hookFrames) {
                const char *dir = getenv("THPS_SHOTDIR") ? getenv("THPS_SHOTDIR") : ".";
                snprintf(path, sizeof path, "%s/frame%05d.bmp", dir, m_hookFrames);
                g_video.DumpDisplay(path);
                snprintf(path, sizeof path, "%s/vram%05d.bmp", dir, m_hookFrames);
                g_video.DumpSpace(path);
                if (getenv("THPS_R3D_DUMP")) {
                    snprintf(path, sizeof path, "%s/faces%05d.txt", dir, m_hookFrames);
                    g_renderer3d.DumpLast(path);
                }
                PC_Log("frame %d: dumped display", m_hookFrames);
            }
            while (*p && *p != ',')
                p++;
            if (*p == ',')
                p++;
        }
    }
    if (m_maxFrames && m_hookFrames >= m_maxFrames) {
        PC_Log("THPS_FRAMES reached (%d), exiting", m_hookFrames);
        os::Exit(0);
    }
}

/* Block until the next frame has run. */
void FrameClock::WaitFrame() {
    uint32_t start = m_frames;
    if (!m_freq)
        Init();
    if (m_deterministic) {
        m_virtualNow = m_next;
        Poll();
    } else if (m_unthrottled) {
        m_next = Now();
        RunFrame();
    } else {
        while (m_frames == start) {
            int64_t now;
            now = Now();
            if (now < m_next) {
                int64_t ms = (m_next - now) * 1000 / m_freq;
                if (ms > 1)
                    os::SleepMs((unsigned)(ms - 1));
            }
            Poll();
        }
    }
}

void FrameClock::Wait(int n) {
    while (n-- > 0)
        WaitFrame();
}

void FrameClock::SetHandler(void (*fn)(void)) {
    m_handler = fn;
    if (getenv("THPS_TRACE_CULL"))
        fprintf(stderr, "frame handler %p set at f%u\n", (void *)fn, pc_heartbeat);
}

} // namespace pc

/* ---- the game's interface (pc_frame.h) ---- */

extern "C" {
void PC_FrameWait(int n) { pc::g_frameClock.Wait(n); }
int PC_FramePoll(void) { return pc::g_frameClock.Poll(); }
uint32_t PC_FrameCount(void) { return pc::g_frameClock.Count(); }
void PC_SetFrameHandler(void (*fn)(void)) { pc::g_frameClock.SetHandler(fn); }
}

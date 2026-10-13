/* The frame clock (pc_frame.h). Frames run at 60 Hz off the wall clock, from PC_FrameWait and from
 * PC_FramePoll, which the port calls wherever the game waits on something a frame changes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_frame.h"
#include "pc_gpu.h"
#include "pc_platform.h"
#include "pc_render3d.h"
#include "pc_runtime.h"
#include "pc_stream.h"

#define FRAME_HZ 60

extern volatile uint32_t pc_heartbeat;
void PC_CountReport(uint32_t frame);
static void (*s_frameHandler)(void);
static uint32_t s_frames;
static int64_t s_freq, s_next;
static int s_inHandler;
static int s_unthrottled;
/* THPS_DETERMINISTIC=1: a virtual clock instead of the wall clock, so automated runs (scripted
 * input, frame dumps) behave the same every time. Each poll advances it 1 ms. */
static int s_det;
static int64_t s_virtualNow;

static int64_t Now(void) {
    if (s_det)
        return s_virtualNow;
    return PC_ClockNow();
}

uint32_t PC_FrameCount(void) { return s_frames; }

static void ClockInit(void) {
    s_det = getenv("THPS_DETERMINISTIC") && atoi(getenv("THPS_DETERMINISTIC"));
    if (s_det)
        s_freq = 1000000;
    else
        s_freq = PC_ClockFreq();
    s_next = Now();
    s_unthrottled = getenv("THPS_FAST") && atoi(getenv("THPS_FAST"));
}

static void DebugHooks(void);

/* One frame: input, streaming, the game's frame handler, then show the display area (the game
 * flips buffers from its handler and never waits for a frame while playing). */
static void RunFrame(void) {
    s_frames++;
    PC_StreamFrame();
    PC_GpuRunDueCallback();
    if (s_frameHandler && !s_inHandler) {
        s_inHandler = 1;
        s_frameHandler();
        s_inHandler = 0;
    }
    PC_GpuPresent();
    PC_CountReport(pc_heartbeat);
    pc_heartbeat++;
    PC_PumpEvents();
    DebugHooks();
    if (PC_QuitRequested())
        PC_ExitProcess(0);
}

int PC_FramePoll(void) {
    int64_t now;
    int n = 0;
    if (!s_freq)
        ClockInit();
    if (s_det)
        s_virtualNow += 1000;
    now = Now();
    while (now >= s_next && n < 4) {
        s_next += s_freq / FRAME_HZ;
        RunFrame();
        n++;
    }
    if (now - s_next > s_freq / 4)
        s_next = now; /* fell far behind (debugger, loading): don't replay a burst */
    return n;
}

/* Automated runs: THPS_SHOT="f1,f2,..." dumps the display (and VRAM) at those frames to
 * THPS_SHOTDIR (default "."); THPS_FRAMES=n exits after n frames; THPS_POKE="addr:value,..."
 * stores those words every frame (0x800d1708:1 = Cheat_LevelSelect; a decompiled global by its
 * retail address, PC_Data). */
static void DebugHooks(void) {
    static int frames, maxFrames = -1;
    static const char *shots, *pokes;
    char path[512];
    if (maxFrames < 0) {
        maxFrames = getenv("THPS_FRAMES") ? atoi(getenv("THPS_FRAMES")) : 0;
        shots = getenv("THPS_SHOT");
        pokes = getenv("THPS_POKE");
    }
    for (const char *p = pokes; p && *p;) {
        char *e;
        uint32_t a = strtoul(p, &e, 0), v = *e == ':' ? strtoul(e + 1, &e, 0) : 0;
        if (a >= PSX_RAM_BASE && a < PSX_RAM_BASE + PSX_RAM_MAPPED && !(a & 3))
            *(uint32_t *)PC_Data(a) = v;
        p = strchr(e, ',') ? strchr(e, ',') + 1 : "";
    }
    frames++;
    if (shots) {
        const char *p = shots;
        while (*p) {
            if (atoi(p) == frames) {
                const char *dir = getenv("THPS_SHOTDIR") ? getenv("THPS_SHOTDIR") : ".";
                snprintf(path, sizeof path, "%s/frame%05d.bmp", dir, frames);
                PC_GpuDumpDisplay(path);
                snprintf(path, sizeof path, "%s/vram%05d.bmp", dir, frames);
                PC_GpuDumpVram(path);
                if (getenv("THPS_R3D_DUMP")) {
                    snprintf(path, sizeof path, "%s/faces%05d.txt", dir, frames);
                    R3d_DumpLast(path);
                }
                PC_Log("frame %d: dumped display", frames);
            }
            while (*p && *p != ',')
                p++;
            if (*p == ',')
                p++;
        }
    }
    if (maxFrames && frames >= maxFrames) {
        PC_Log("THPS_FRAMES reached (%d), exiting", frames);
        PC_ExitProcess(0);
    }
}

/* Block until the next frame has run. */
static void WaitFrame(void) {
    uint32_t start = s_frames;
    if (!s_freq)
        ClockInit();
    if (s_det) {
        s_virtualNow = s_next;
        PC_FramePoll();
    } else if (s_unthrottled) {
        s_next = Now();
        RunFrame();
    } else {
        while (s_frames == start) {
            int64_t now;
            now = Now();
            if (now < s_next) {
                int64_t ms = (s_next - now) * 1000 / s_freq;
                if (ms > 1)
                    PC_SleepMs((unsigned)(ms - 1));
            }
            PC_FramePoll();
        }
    }
}

void PC_FrameWait(int n) {
    while (n-- > 0)
        WaitFrame();
}

void PC_SetFrameHandler(void (*fn)(void)) {
    s_frameHandler = fn;
    if (getenv("THPS_TRACE_CULL"))
        fprintf(stderr, "frame handler %p set at f%u\n", (void *)fn, pc_heartbeat);
}

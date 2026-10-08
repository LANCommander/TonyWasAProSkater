/* libetc: VSync, interrupt callbacks. Time advances on a 60 Hz clock; vblank "interrupts" are
 * run from VSync and from the HLE calls games busy-wait on (see PC_TickVBlank). */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "pc_gpu.h"
#include "pc_hle.h"
#include "pc_runtime.h"

#define NTSC_HZ 60

extern volatile uint32_t pc_heartbeat;
static uint32_t s_vsyncCallback;
static uint32_t s_vblanks;
static LARGE_INTEGER s_freq, s_next;
static int s_inTick;
static int s_unthrottled;

static void (*s_vsyncHooks[4])(void);

uint32_t PC_VBlankCount(void) { return s_vblanks; }

static void ClockInit(void) {
    QueryPerformanceFrequency(&s_freq);
    QueryPerformanceCounter(&s_next);
    s_unthrottled = getenv("THPS_FAST") && atoi(getenv("THPS_FAST"));
}

static void RunVBlank(void) {
    s_vblanks++;
    PC_InputPoll();
    if (s_vsyncCallback && !s_inTick) {
        s_inTick = 1;
        ((void (*)(void))PSX_PTR(s_vsyncCallback))();
        s_inTick = 0;
    }
}

int PC_TickVBlank(void) {
    LARGE_INTEGER now;
    int n = 0;
    if (!s_freq.QuadPart)
        ClockInit();
    QueryPerformanceCounter(&now);
    while (now.QuadPart >= s_next.QuadPart && n < 4) {
        s_next.QuadPart += s_freq.QuadPart / NTSC_HZ;
        RunVBlank();
        n++;
    }
    if (now.QuadPart - s_next.QuadPart > s_freq.QuadPart / 4)
        s_next = now; /* fell far behind (debugger, loading): don't replay a burst */
    return n;
}

/* Automated runs: THPS_SHOT="f1,f2,..." dumps the display (and VRAM) at those VSync(0) counts to
 * THPS_SHOTDIR (default "."); THPS_FRAMES=n exits after n presented frames. */
static void DebugHooks(void) {
    static int frames, maxFrames = -1;
    static const char *shots;
    char path[512];
    if (maxFrames < 0) {
        maxFrames = getenv("THPS_FRAMES") ? atoi(getenv("THPS_FRAMES")) : 0;
        shots = getenv("THPS_SHOT");
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
        ExitProcess(0);
    }
}

/* Block until the next vblank, then present. */
static void WaitVBlank(void) {
    uint32_t start = s_vblanks;
    if (!s_freq.QuadPart)
        ClockInit();
    if (s_unthrottled) {
        QueryPerformanceCounter(&s_next);
        RunVBlank();
    } else {
        while (s_vblanks == start) {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (now.QuadPart < s_next.QuadPart) {
                LONGLONG ms = (s_next.QuadPart - now.QuadPart) * 1000 / s_freq.QuadPart;
                if (ms > 1)
                    Sleep((DWORD)(ms - 1));
            }
            PC_TickVBlank();
        }
    }
    PC_GpuPresent();
    pc_heartbeat++;
    PC_PumpEvents();
    DebugHooks();
    if (PC_QuitRequested())
        ExitProcess(0);
}

int VSync(int mode) {
    if (mode < 0)
        return (int)s_vblanks;
    if (mode == 1) {
        PC_TickVBlank();
        return 0; /* horizontal lines since the last vblank */
    }
    if (mode == 0)
        mode = 1;
    while (mode-- > 0)
        WaitVBlank();
    return 0;
}

int VSyncCallback(uint32_t func) {
    s_vsyncCallback = func;
    return 0;
}

int ResetCallback(void) {
    s_vsyncCallback = 0;
    return 0;
}

uint32_t InterruptCallback(int irq, uint32_t func) {
    (void)irq;
    (void)func;
    return 0;
}

uint32_t DMACallback(int dma, uint32_t func) {
    (void)dma;
    (void)func;
    return 0;
}

int SetIntrMask(int mask) { return mask; }
int StopCallback(void) { return 0; }
int RestartCallback(void) { return 0; }
int CheckCallback(void) { return 0; }
void PadStop(void) {}

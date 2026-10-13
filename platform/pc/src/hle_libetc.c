/* libetc: VSync, interrupt callbacks. Time advances on a 60 Hz clock; vblank "interrupts" are
 * run from VSync and from the HLE calls games busy-wait on (see PC_TickVBlank). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_gpu.h"
#include "pc_hle.h"
#include "pc_platform.h"
#include "pc_render3d.h"
#include "pc_runtime.h"

#define NTSC_HZ 60

extern volatile uint32_t pc_heartbeat;
void PC_CountReport(uint32_t frame);
static uint32_t s_vsyncCallback;
static uint32_t s_vblanks;
static int64_t s_freq, s_next;
static int s_inTick;
static int s_unthrottled;
/* THPS_DETERMINISTIC=1: a virtual clock instead of the wall clock, so automated runs (scripted
 * input, frame dumps) behave the same every time. Each poll of hardware state advances it 1 ms. */
static int s_det;
static int64_t s_virtualNow;

static int64_t Now(void) {
    if (s_det)
        return s_virtualNow;
    return PC_ClockNow();
}

uint32_t PC_VBlankCount(void) { return s_vblanks; }

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

/* One vertical blank: input, the game's VSync callback, then show the display area (games that
 * flip buffers from the callback never call VSync(0) while playing). */
static void RunVBlank(void) {
    s_vblanks++;
    PC_InputPoll();
    PC_CdStreamVBlank();
    PC_GpuRunDueCallback();
    if (s_vsyncCallback && !s_inTick) {
        s_inTick = 1;
        ((void (*)(void))PSX_PTR(s_vsyncCallback))();
        s_inTick = 0;
    }
    PC_GpuPresent();
    PC_CountReport(pc_heartbeat);
    pc_heartbeat++;
    PC_PumpEvents();
    DebugHooks();
    if (PC_QuitRequested())
        PC_ExitProcess(0);
}

int PC_TickVBlank(void) {
    int64_t now;
    int n = 0;
    if (!s_freq)
        ClockInit();
    if (s_det)
        s_virtualNow += 1000;
    now = Now();
    while (now >= s_next && n < 4) {
        s_next += s_freq / NTSC_HZ;
        RunVBlank();
        n++;
    }
    if (now - s_next > s_freq / 4)
        s_next = now; /* fell far behind (debugger, loading): don't replay a burst */
    return n;
}

/* Automated runs: THPS_SHOT="f1,f2,..." dumps the display (and VRAM) at those VSync(0) counts to
 * THPS_SHOTDIR (default "."); THPS_FRAMES=n exits after n presented frames; THPS_POKE=
 * "addr:value,..." stores those words every vblank (0x800d1708:1 = Cheat_LevelSelect; a
 * decompiled global by its retail address, PC_Data). */
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

/* Block until the next vblank, then present. */
static void WaitVBlank(void) {
    uint32_t start = s_vblanks;
    if (!s_freq)
        ClockInit();
    if (s_det) {
        s_virtualNow = s_next;
        PC_TickVBlank();
    } else if (s_unthrottled) {
        s_next = Now();
        RunVBlank();
    } else {
        while (s_vblanks == start) {
            int64_t now;
            now = Now();
            if (now < s_next) {
                int64_t ms = (s_next - now) * 1000 / s_freq;
                if (ms > 1)
                    PC_SleepMs((unsigned)(ms - 1));
            }
            PC_TickVBlank();
        }
    }
}

int VSync(int mode) {
    if (mode < 0) {
        /* the vblank counter: games spin on it (the career auto-load waits a second after
         * loading), so a vblank that is due happens here */
        PC_TickVBlank();
        return (int)s_vblanks;
    }
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
    if (getenv("THPS_TRACE_CULL"))
        fprintf(stderr, "VSyncCallback(%08x) registered at f%u\n", func, pc_heartbeat);
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

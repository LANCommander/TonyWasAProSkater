/* Decompiled game functions replaced on PC (hleov_<addr> beats the decompiled C).
 * Keep this list short: each entry is a feature the port doesn't support yet. */
#include <stdlib.h>

#include "pc_frame.h"
#include "pc_hle.h"
#include "pc_movie.h"
#include "pc_runtime.h"

void PC_Backtrace(const char *why);

/* GameFMV_PlayMovie(int): the PS1's STR movies play through the game's own player (the MDEC is
 * pc_mdec.c), the Dreamcast's films through pc_movie.c. A movie whose file isn't there is
 * skipped, which is what happens when the player presses a button through one; THPS_MOVIES=0
 * skips every movie. */
void hleov_80022c1c(int track) {
    extern void pcfn_80022c1c(int);
    const char *e = getenv("THPS_MOVIES");
    if ((!e || atoi(e) != 0) && PC_MovieAvailable(track)) {
        pcfn_80022c1c(track);
        return;
    }
    PC_Log("movie %d skipped", track);
}

/* THPS_TRACE_PSX=1: log PSX model processing (debugging the spooler). */
static int TracePsx(void) {
    static int on = -1;
    if (on < 0)
        on = getenv("THPS_TRACE_PSX") && atoi(getenv("THPS_TRACE_PSX"));
    return on;
}

void hleov_8006f290(int region) { /* ProcessNewPSX */
    extern void pcfn_8006f290(int);
    if (TracePsx()) {
        uint32_t *psx = *(uint32_t **)PSX_PTR(PSX_DATA_ADDR(0x800bf7a8) + region * 64 + 0x14);
        PC_Log("ProcessNewPSX(%d) pPSX=%08x name=%.9s", region, (unsigned)(uintptr_t)psx,
               (char *)PSX_PTR(PSX_DATA_ADDR(0x800bf7a8) + region * 64));
    }
    pcfn_8006f290(region);
}

void hleov_8006ed2c(int region) {
    extern void pcfn_8006ed2c(int);
    if (TracePsx())
        PC_Log("PSX backup %d", region);
    pcfn_8006ed2c(region);
}

void hleov_8006edb8(int region) {
    extern void pcfn_8006edb8(int);
    if (TracePsx())
        PC_Log("PSX restore %d", region);
    pcfn_8006edb8(region);
}

/* Bang(r, g, b): the game's fatal-error handler (fills the screen with a colour and spins).
 * Report it instead of hanging silently: which heap ran out, and its free list. */
void hleov_8004ce28(int r, int g, int b) {
    uint32_t heap;
    PC_Log("Bang(%d, %d, %d): the game hit a fatal error (out of memory if from Mem_New*)", r, g, b);
    for (heap = 0; heap < 2; heap++) {
        uint32_t blk = *(uint32_t *)PSX_PTR(PSX_DATA_ADDR(0x800d1b64) + heap * 4), total = 0, n = 0, big = 0;
        while (blk && n < 100000) {
            uint32_t size = *(uint32_t *)PSX_PTR(blk + 4) >> 4;
            total += size;
            if (size > big)
                big = size;
            blk = *(uint32_t *)PSX_PTR(blk);
            n++;
        }
        PC_Log("  heap %u: %u free blocks, %u bytes free, largest %u; used %d", heap, n, total, big,
               *(int *)PSX_PTR(PSX_DATA_ADDR(0x800d1b5c) + heap * 4));
    }
    PC_Backtrace("Bang");
    PC_Fatal("Bang(%d, %d, %d)", r, g, b);
}

/* Pause(n): spins until the frame handler (MyVSync) has bumped Vblanks n times. On the PS1 the interrupt
 * runs during the spin; here vblanks only happen when the port polls, so poll while waiting. */
void hleov_800478f4(int n) {
    volatile uint32_t *vblanks = PSX_PTR(PSX_DATA_ADDR(0x800d1a20));
    uint32_t t = *vblanks + (uint32_t)n;
    while (*vblanks < t)
        PC_FramePoll();
}

/* THPS_TRACE_RENDER=1: per-frame counts for the environment renderer (debugging). */
extern volatile uint32_t pc_heartbeat;
void PC_RenderTrace(const char *what, uint32_t ret) {
    static uint32_t frame = ~0u, calls[2], culled;
    static int on = -1;
    if (on < 0)
        on = getenv("THPS_TRACE_RENDER") && atoi(getenv("THPS_TRACE_RENDER"));
    if (!on)
        return;
    if (frame != pc_heartbeat) {
        if (calls[0] || calls[1])
            PC_Log("render f%u: %u transform+outcode (%u culled), %u ProcessPolys", frame, calls[0],
                   culled, calls[1]);
        frame = pc_heartbeat;
        calls[0] = calls[1] = culled = 0;
    }
    if (what[0] == 'T') {
        calls[0]++;
        if (ret & 0xbf)
            culled++;
    } else {
        calls[1]++;
    }
}

typedef uint32_t (*Fn12)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                         uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
extern const PcFunc pc_recomp_funcs[], pc_recomp_game_funcs[];
/* The implementation an override wraps: the function's C (decompiled, rewritten from assembly or
 * HLE'd), else (build.py --mips) its translated retail code. */
Fn12 PC_Impl(uint32_t addr) {
    const PcFunc *t;
    for (t = pc_game_funcs; t->addr; t++)
        if (t->addr == addr)
            return (Fn12)t->fn;
    for (t = pc_recomp_funcs; t->addr; t++)
        if (t->addr == addr)
            return (Fn12)t->fn;
    for (t = pc_recomp_game_funcs; t->addr; t++)
        if (t->addr == addr)
            return (Fn12)t->fn;
    PC_Fatal("no implementation of %08x", addr);
    return NULL;
}

/* The vertex transforms and ProcessPolys (0x80085d28, 0x80086f40 ...) are wrapped by the native
 * 3D renderer, pc_render3d.c. */

/* M3dAsm_BoundingSpherePreprocessing(list, flags): THPS_TRACE_CULL=1 logs how many items it culls. */
uint32_t hleov_800858ac(uint32_t list, uint32_t flags) {
    static Fn12 f;
    static int on = -1;
    uint32_t r, p, n = 0, culled = 0, before = 0;
    if (!f)
        f = PC_Impl(0x800858ac);
    if (on < 0)
        on = getenv("THPS_TRACE_CULL") && atoi(getenv("THPS_TRACE_CULL"));
    if (on)
        for (p = list; p && n < 10000; p = *(uint32_t *)PSX_PTR(p + 28), n++)
            before += (*(uint16_t *)PSX_PTR(p) & 0x8000) != 0;
    r = f(list, flags, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (on && n) {
        for (p = list, n = 0; p && n < 10000; p = *(uint32_t *)PSX_PTR(p + 28), n++)
            culled += (*(uint16_t *)PSX_PTR(p) & 0x8000) != 0;
        /* Per-item identity tracking (not just the aggregate count): log items whose culled bit
         * FLIPS vs the last time this exact list ran, to catch a subset flickering in/out that
         * nets to a small aggregate delta. */
        if (n > 200) {
            static uint32_t prev_addr[512];
            static uint8_t prev_culled[512];
            static int prev_n;
            uint32_t i = 0;
            for (p = list; p && i < 512 && i < n; p = *(uint32_t *)PSX_PTR(p + 28), i++) {
                uint8_t now_c = (*(uint16_t *)PSX_PTR(p) & 0x8000) != 0;
                if (prev_n == (int)n && prev_addr[i] == p && prev_culled[i] != now_c)
                    PC_Log("  flip f%u idx=%u item=%08x %s pos %d %d %d", pc_heartbeat, i, p,
                           now_c ? "NOW_CULLED" : "NOW_VISIBLE", *(int32_t *)PSX_PTR(p + 4) >> 12,
                           *(int32_t *)PSX_PTR(p + 8) >> 12, *(int32_t *)PSX_PTR(p + 12) >> 12);
                prev_addr[i] = p;
                prev_culled[i] = now_c;
            }
            prev_n = (int)n;
        }
        uint32_t cam = *(uint32_t *)PSX_PTR(PSX_DATA_ADDR(0x800d217c));
        PC_Log("cull f%u list %08x: %u items, %u culled (%u before), flags %x", pc_heartbeat, list, n,
               culled, before, flags);
        if (cam && list)
            PC_Log("   camera %08x pos %d %d %d; item0 flags %04x pos %d %d %d", cam,
                   *(int32_t *)PSX_PTR(cam + 4), *(int32_t *)PSX_PTR(cam + 8),
                   *(int32_t *)PSX_PTR(cam + 12), *(uint16_t *)PSX_PTR(list),
                   *(int32_t *)PSX_PTR(list + 4) >> 12, *(int32_t *)PSX_PTR(list + 8) >> 12,
                   *(int32_t *)PSX_PTR(list + 12) >> 12);
        {
            uint32_t sk = *(uint32_t *)PSX_PTR(PSX_DATA_ADDR(0x800d19a4));
            if (sk)
                PC_Log("   skater %08x pos %d %d %d", sk, *(int32_t *)PSX_PTR(sk + 4) >> 12,
                       *(int32_t *)PSX_PTR(sk + 8) >> 12, *(int32_t *)PSX_PTR(sk + 12) >> 12);
        }
    }
    return r;
}

/* CCamera::LoadIntoMikeCamera(this): THPS_TRACE_CULL=1 also logs the camera object. */
void hleov_80078b04(uint32_t self) {
    static Fn12 f;
    static uint32_t last;
    if (!f)
        f = PC_Impl(0x80078b04);
    if (getenv("THPS_TRACE_CULL") && pc_heartbeat != last && (pc_heartbeat % 50) == 1) {
        uint32_t tri = *(uint32_t *)PSX_PTR(self + 0x420);
        last = pc_heartbeat;
        PC_Log("camera obj %08x mPos %d %d %d tripod %08x state %d tripodpos %d %d %d focus %d %d %d",
               self, *(int32_t *)PSX_PTR(self + 4) >> 12, *(int32_t *)PSX_PTR(self + 8) >> 12,
               *(int32_t *)PSX_PTR(self + 12) >> 12, tri,
               tri ? *(int32_t *)PSX_PTR(tri + 0x734) : -1,
               *(int32_t *)PSX_PTR(self + 0x43c) >> 12, *(int32_t *)PSX_PTR(self + 0x440) >> 12,
               *(int32_t *)PSX_PTR(self + 0x444) >> 12, *(int32_t *)PSX_PTR(self + 0x46c) >> 12,
               *(int32_t *)PSX_PTR(self + 0x470) >> 12, *(int32_t *)PSX_PTR(self + 0x474) >> 12);
    }
    f(self, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/* Debugging help for the game's code: its fatal-error handler (Bang) and the renderer's per-frame
 * counts. */
#include <stdlib.h>

#include "pc_runtime.h"

void PC_Backtrace(const char *why);

void PC_Bang(int r, int g, int b) {
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

/* Debugging help for the game's code: its fatal-error handler (Bang) and the renderer's per-frame
 * counts. */
#include <stdlib.h>

#include "pc_runtime.h"

void PC_Backtrace(const char *why);

void PC_Bang(int r, int g, int b) {
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

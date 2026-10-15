/* Debugging help for the game's code: its fatal-error handler (Bang) and the renderer's per-frame
 * counts. */
#include <stdlib.h>

#include "pc/runtime.h"
#include "pc_runtime.h"

extern volatile uint32_t pc_heartbeat;

namespace pc {

RenderTrace g_renderTrace;

/* THPS_TRACE_RENDER=1: per-frame counts for the environment renderer (debugging). */
void RenderTrace::Count(const char *what, uint32_t ret) {
    if (m_on < 0)
        m_on = getenv("THPS_TRACE_RENDER") && atoi(getenv("THPS_TRACE_RENDER"));
    if (!m_on)
        return;
    if (m_frame != pc_heartbeat) {
        if (m_calls[0] || m_calls[1])
            PC_Log("render f%u: %u transform+outcode (%u culled), %u ProcessPolys", m_frame,
                   m_calls[0], m_culled, m_calls[1]);
        m_frame = pc_heartbeat;
        m_calls[0] = m_calls[1] = m_culled = 0;
    }
    if (what[0] == 'T') {
        m_calls[0]++;
        if (ret & 0xbf)
            m_culled++;
    } else {
        m_calls[1]++;
    }
}

} // namespace pc

/* ---- the game's interface (pc_runtime.h) ---- */

void PC_Bang(int r, int g, int b) {
    pc::g_diagnostics.Backtrace("Bang");
    PC_Fatal("Bang(%d, %d, %d)", r, g, b);
}

/* (C++ linkage, as pc_render3d.cpp declares it) */

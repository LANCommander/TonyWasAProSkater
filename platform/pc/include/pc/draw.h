/* The game's drawing as a class (pc_draw.cpp; the game's C interface is pc_draw.h).
 *
 * OtDrawer walks the game's ordering tables and primitive packets (psx-spx "GPU" command words)
 * and draws them through OpenGL (pc_gpu_gl.h). It keeps what the PS1's GPU kept between commands:
 * the drawing mode and texture page (E1), texture window (E2), drawing area (E3/E4), offset (E5)
 * and mask bits (E6), plus the port's own markers (2D entries, flat and glow and depth packets)
 * and the "drawing finished" callback. */
#pragma once

#include <stdint.h>

#include "pc/gl_gpu.h"

struct P_TAG;

namespace pc {

class OtDrawer {
public:
    /* Draw an ordering table (or any chain of packets) from its first entry. The done function
     * (SetDone) runs with the next frame, as drawing on the PS1 finished a while later. */
    void DrawOT(const void *ot);
    /* An ordering-table entry after which a chain is the HUD, panels and text (the game's OT and
     * OT2D entries), which the precise (unrounded) 3D vertices don't apply to. */
    void Mark2D(const void *entry);
    /* Draw one packet now. */
    void DrawPacket(const void *p);
    /* Clear an ordering table of n entries linked from the last to the first (entry 0 ends it). */
    static void ClearOTR(void *ot, int n);
    /* The drawing environment (the game's Psy-Q DRAWENV, whose dr_env gets its commands). */
    void SetEnv(void *drawenv);
    /* What runs once an ordering table is drawn (NULL: nothing), and running it when it is due. */
    void SetDone(void (*fn)(void)) { m_doneCb = fn; }
    void RunDone();
    /* The drawing state back to its power-on values. */
    void Reset();
    /* The current drawing offset and drawing area (inclusive), for the native 3D renderer;
     * returns GlGpu::WideClip's surface number. */
    int State(int *ofs_x, int *ofs_y, int clip[4]) const;

private:
    struct Prim;
    struct Vtx;

    static int TriTooBig(const Vtx *a, const Vtx *b, const Vtx *c);
    static void ToGlVertex(GlVertex *o, const Vtx *v);
    void PrimFromState(Prim *p, int textured, int raw, int semi, uint16_t clut) const;
    void VtxFromWords(Vtx *v, uint32_t xy, uint32_t col) const;
    void ToGlPrim(GlPrim *o, const Prim *p) const;
    void GlPrecise(GlVertex *gv, const uint32_t *xy, int n) const;
    void LineOut(const Prim *p, const Vtx *a, const Vtx *b) const;
    void CmdGlow(const uint32_t *w);
    void CmdPolygon(const uint32_t *w);
    int CmdLine(const uint32_t *w, int n);
    void CmdRect(const uint32_t *w);
    void SetDrawMode(uint32_t v);
    void SetTexWindow(uint32_t v);
    void Execute(const uint32_t *w);
    void DrawWords(const uint32_t *w, int n);
    bool Is2dEntry(const P_TAG *p) const;
    void DrawChain(const P_TAG *first);
    void DrawOne(const P_TAG *ot);

    /* The GPU's drawing state, as its E1-E6 commands set it. */
    struct GpuState {
        uint32_t drawmode = 0;   /* E1 bits 0-13 (texpage, abr, depth, dither, dfe, tex off, flips) */
        uint32_t e2 = 0;         /* the texture window, as set */
        uint8_t tw_and_x = 0, tw_and_y = 0, tw_or_x = 0, tw_or_y = 0;
        int clip_l = 0, clip_t = 0, clip_r = 0, clip_b = 0;
        int ofs_x = 0, ofs_y = 0;
        int mask_set = 0, mask_check = 0;
    };
    GpuState m_gs;

    /* The view depths of a polygon's vertices given by PC_DRAW_DEPTH up to the flat end (0: none). */
    float m_flatZ[4] = {};
    /* Set while DrawChain is past the ordering table's 2D entries (Is2dEntry). */
    int m_chain2d = 0;
    /* Set between PC_DRAW_FLAT_BEGIN (or PC_DRAW_GLOW) and PC_DRAW_FLAT_END. */
    int m_flat = 0;
    /* Set after a glow drawn from its PC_DRAW_GLOW: its polygons up to the flat end are not drawn. */
    int m_glowDrawn = 0;

    /* The ordering-table entries drawn last, with the HUD, panels and text: THPS1's display
     * buffers (SDoubleBuffer) end their ordering table with OT and OT2D (Mark2D). */
    const void *m_2dEntries[8] = {};

    /* The "drawing has finished" callback and whether it is due (RunDone). */
    void (*m_doneCb)(void) = nullptr;
    int m_doneDue = 0;

    /* Ordering tables given while one is drawn (from the done callback): the outer call draws them. */
    static constexpr int MAX_PENDING = 32;
    int m_inDraw = 0, m_npending = 0;
    const P_TAG *m_pending[MAX_PENDING] = {};
};

extern OtDrawer g_otDrawer;

} // namespace pc

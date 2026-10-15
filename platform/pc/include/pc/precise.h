/* The PC renderers' view of the game's projections: precise vertices and the float capture
 * (src/pc_precise.cpp; the C API is pc_precise.h). */
#pragma once

#include <stdint.h>

#include "pc_geom.h"

namespace pc {

/* Precise projections, for the OpenGL renderer. The game's projection rounds every vertex to a
 * whole pixel (its screen word) and the GPU gets nothing else, which makes geometry wobble and
 * textures warp. When enabled, every projection also records the unrounded screen position and
 * the view depth, keyed by the 32-bit screen word the game copies into its GPU packets; the GPU
 * looks the word up again when it draws. Entries expire a few vblanks after they were recorded
 * (a frame's packets can be drawn after the next frame has started projecting).
 *
 * Between CaptureBegin and CaptureEnd, every projected vertex is also appended to a buffer as its
 * unrounded view-space position: the native 3D renderer (pc/render3d.h) wraps the retail vertex
 * transforms with this. Both are fed by the geometry maths' projection observer (pc_geom.h). */
class PreciseVertices {
public:
    void Enable(bool on);
    /* Depth scale of the projections recorded from now on (1 by default): THPS1's dewibbled
     * transforms work at 16 times the scale, so their true view depth is z / 16. */
    void ZScale(float s);
    /* A polygon's vertices (1 if it found all of them); where different vertices rounded to the
     * same pixel it takes the one nearest the polygon's depth. */
    int Polygon(const uint32_t *sxy, int n, float *x, float *y, float *z);

    /* Every projected vertex from now on to buf (up to max), in vertex units before the shift. */
    void CaptureBegin(float (*buf)[3], int max);
    /* Stops the capture; the number of vertices captured. */
    int CaptureEnd();

    /* The projection observer (installed while either is on). */
    void Observe(const GeomState *g, const int64_t acc[3], int shift, uint32_t sxy, uint32_t flag,
                 int clamped);

private:
    static constexpr int kSlots = 1 << 16;
    static constexpr int kProbe = 8;
    static constexpr int kCands = 4;
    static constexpr uint32_t kMaxAge = 6; /* vblanks; a 30 fps frame is projected and then drawn
                                            * within about 4 */

    /* Every distinct projection that rounded to one screen word in one vblank (usually one). */
    struct Entry {
        uint32_t key, gen; /* gen: pc_heartbeat + 1 when recorded, 0: never used */
        int n;
        float x[kCands], y[kCands], z[kCands];
    };

    void Record(const GeomState *g, const int64_t *acc, int shift, uint32_t sxy, uint32_t flag,
                int clamped);
    const Entry *Find(uint32_t key) const;
    int Candidates(uint32_t sxy, float *x, float *y, float *z) const;
    void UpdateObserver();

    /* the table (4 MB of zeros): a static member so that it stays out of the instance's
     * initialised data */
    static Entry s_table[kSlots];

    bool m_on = false;
    double m_zscale = 1.0;
    float (*m_capture)[3] = nullptr;
    int m_captureN = 0, m_captureMax = 0;
};

extern PreciseVertices g_precise;

} // namespace pc

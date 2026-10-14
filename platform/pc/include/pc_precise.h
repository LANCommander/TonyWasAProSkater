/* The PC renderers' view of the game's projections (src/pc_precise.c). */
#ifndef PC_PRECISE_H
#define PC_PRECISE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Precise projections, for the OpenGL renderer. The game's projection rounds every vertex to a
 * whole pixel (its screen word) and the GPU gets nothing else, which makes geometry wobble and
 * textures warp. When enabled, every projection also records the unrounded screen position and
 * the view depth, keyed by the 32-bit screen word the game copies into its GPU packets; the GPU
 * looks the word up again when it draws. PC_PrecisePolygon finds a polygon's vertices (1 if it
 * found all of them); where different vertices rounded to the same pixel it takes the one nearest
 * the polygon's depth.
 * Entries expire a few vblanks after they were recorded (a frame's packets can be drawn after
 * the next frame has started projecting). */
void PC_PreciseEnable(int on);
/* Depth scale of the projections recorded from now on (1 by default): THPS1's dewibbled
 * transforms work at 16 times the scale, so their true view depth is z / 16. */
void PC_PreciseZScale(float s);
int PC_PrecisePolygon(const uint32_t *sxy, int n, float *x, float *y, float *z);

/* Between these, every projected vertex is also appended to buf (up to max) as its unrounded
 * view-space position (before the shift, in vertex units). The native 3D renderer
 * (pc_render3d.c) wraps the retail vertex transforms with this. End returns the count. */
void PC_CaptureBegin(float (*buf)[3], int max);
int PC_CaptureEnd(void);

#ifdef __cplusplus
}
#endif

#endif

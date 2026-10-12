/* Software GTE (PS1 COP2), register level. Everything that touches the GTE goes through these five
 * calls: the inline_c.h macros, code translated from MIPS by mips2c (mtc2/mfc2/ctc2/cfc2/lwc2/swc2/
 * cop2), and the libgte functions. Semantics follow psx-spx "Geometry Transformation Engine". */
#ifndef PC_GTE_H
#define PC_GTE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Data registers 0-31 (mfc2/mtc2, lwc2/swc2): read/write side effects included (SXYP pushes the
 * FIFO, IRGB/ORGB convert, LZCS sets LZCR, 16-bit registers sign/zero extend on read). */
uint32_t PC_GteGetData(int reg);
void PC_GteSetData(int reg, uint32_t value);

/* Control registers 0-31 (cfc2/ctc2), with the hardware's sign-extension of 16-bit fields. */
uint32_t PC_GteGetCtrl(int reg);
void PC_GteSetCtrl(int reg, uint32_t value);

/* Execute a GTE command: the low 25 bits of the cop2 instruction (sf, mx, v, cv, lm, opcode). */
void PC_GteCommand(uint32_t op);

/* Reset all registers to zero (power-on state). */
void PC_GteReset(void);

/* The whole register state as an opaque blob (differential tests, snapshots). */
int PC_GteStateSize(void);
void PC_GteSaveState(void *buf);
void PC_GteLoadState(const void *buf);

/* Precise projections, for the OpenGL renderer. The GTE rounds every projected vertex to a whole
 * pixel (SXY) and the GPU gets nothing else, which makes geometry wobble and textures warp. When
 * enabled, RTPS/RTPT also record the unrounded screen position and the view depth, keyed by the
 * 32-bit SXY word the game copies into its GPU packets; the GPU looks the word up again when it
 * draws. PC_GtePrecisePolygon finds a polygon's vertices (1 if it found all of them); where
 * different vertices rounded to the same pixel it takes the one nearest the polygon's depth.
 * Entries expire a few vblanks after they were recorded (a frame's packets can be drawn after
 * the next frame has started projecting). */
void PC_GtePreciseEnable(int on);
/* Depth scale of the projections recorded from now on (1 by default): THPS1's dewibbled
 * transforms work at 16 times the scale, so their true view depth is z / 16. */
void PC_GtePreciseZScale(float s);
int PC_GtePrecisePolygon(const uint32_t *sxy, int n, float *x, float *y, float *z);

/* Between these, every RTPS/RTPT vertex is also appended to buf (up to max) as its unrounded
 * view-space position (MAC1-3 before the shift, in vertex units). The native 3D renderer
 * (pc_render3d.c) wraps the retail vertex transforms with this. End returns the count. */
void PC_GteCaptureBegin(float (*buf)[3], int max);
int PC_GteCaptureEnd(void);

#ifdef __cplusplus
}
#endif

#endif

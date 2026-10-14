/* The game's drawing (src/pc_draw.c).
 *
 * The game builds its frames as the PS1 had it: primitive packets (POLY_FT4, SPRT, TILE, LINE_*,
 * DR_*: GPU command words after a tag) linked into ordering tables by 24-bit addresses in PS1 RAM.
 * Those are its display list; this draws them through OpenGL (pc_gpu_gl.h) onto the screens
 * (pc_screen.h), with the textures of the texture store (pc_texstore.h). The native 3D renderer
 * (pc_render3d.h) draws its layers where they are in the ordering table. */
#ifndef PC_DRAW_H
#define PC_DRAW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Draw an ordering table (or any chain of packets) from its first entry. The done function
 * (PC_DrawSetDone) runs with the next frame, as drawing on the PS1 finished a while later. */
void PC_DrawOT(const void *ot);
/* Draw one packet now. */
void PC_DrawPacket(const void *p);
/* Clear an ordering table of n entries linked from the last to the first (entry 0 ends it). */
void PC_DrawClearOTR(void *ot, int n);
/* The drawing environment (the game's Psy-Q DRAWENV, whose dr_env gets its commands): drawing
 * area, offset, texture page and window, and the background fill when it has one. */
void PC_DrawSetEnv(void *drawenv);
/* What runs once an ordering table is drawn (NULL: nothing), and running it when it is due:
 * once a frame, before the game's frame handler (pc_frame.c). */
void PC_DrawSetDone(void (*fn)(void));
void PC_DrawRunDone(void);
/* The drawing state back to its power-on values. */
void PC_DrawReset(void);

/* The current drawing offset and drawing area (inclusive), for the native 3D renderer; widened
 * for widescreen when it covers a gameplay screen, then returns PC_GlWideClip's surface number. */
int PC_DrawState(int *ofs_x, int *ofs_y, int clip[4]);

#ifdef __cplusplus
}
#endif

#endif

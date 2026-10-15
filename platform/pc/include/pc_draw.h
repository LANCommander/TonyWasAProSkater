/* The game's drawing (src/pc_draw.c).
 *
 * The game builds its frames as the PS1 had it: primitive packets (POLY_FT4, SPRT, TILE, LINE_*,
 * DR_*: GPU command words after a tag, Psy-Q's libgpu.h P_TAG) linked into ordering tables.
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
/* An ordering-table entry after which a chain is the HUD, panels and text, which the precise
 * (unrounded) 3D vertices don't apply to: the game's OT and OT2D entries. */
void PC_DrawMark2D(const void *entry);
/* Command words (GPU NOPs) around packets the game made in screen space from one projection, such
 * as the glows: the polygons between them are drawn as given. The precise lookup goes by value,
 * so their computed corners can land on unrelated projections and get those depths. */
#define PC_DRAW_FLAT_BEGIN 0x00f1a700u
#define PC_DRAW_FLAT_END 0x00f1a701u
/* A flat begin that also describes a light's glow (game/src/bit/display.cpp), which is drawn
 * smoothly from it in place of the game's polygons up to the end (THPS_GLOWS=retail: the
 * polygons). Words: this, centre (y << 16 | x, 16-bit), centre colour, rim colour, fringe colour
 * (0x00BBGGRR), angle of section 0 minus one step | step << 16 (4096ths of a turn), fringe width,
 * then the 8 sections' radii two to a word; radii vertical, in 16ths of a pixel (horizontal ones
 * are 400/256 of that). */
#define PC_DRAW_GLOW 0x00f1a702u
#define PC_DRAW_GLOW_WORDS 11
/* A flat begin with view depths for the packets up to the end: effects made in screen space
 * around projected points (sparks, splashes, steam and water jets). Words: this, then the integer
 * view z of a polygon's vertices 0-3 (rectangles take the first). Inside the native 3D layer they
 * are depth-tested at those, so geometry in front hides them however the ordering table sorted
 * the two. */
#define PC_DRAW_DEPTH 0x00f1a703u
#define PC_DRAW_DEPTH_WORDS 5
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

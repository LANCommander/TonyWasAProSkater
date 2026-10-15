/* The screens (src/pc_screen.cpp): what the game draws on and what the window shows.
 *
 * The game's screens are areas in the PS1's 1024x512 frame-buffer space, where its drawing
 * environments and display put them (two 512x240 buffers in play, one 640x480 interlaced in the
 * front end); their pixels are in the renderer's render targets (pc/gl_gpu.h). The CPU's own
 * writes and reads of them, 15-bit pixels with the mask bit, go through here. */
#ifndef PC_SCREEN_H
#define PC_SCREEN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fill a screen area with one colour, rgb = 0x00BBGGRR (8 bits each, as ClearImage). */
void PC_ScreenFill(int x, int y, int w, int h, uint32_t rgb);
/* Pixels (w x h, rows of w) over a screen area. */
void PC_ScreenBlit(int x, int y, int w, int h, const void *src);
/* An image (sw x sh pixels) filling the screen area x, y, w, h: drawn there filtered, so a
 * display surface keeps its resolution (reads see it scaled to the area). The Dreamcast's
 * higher-resolution images. */
void PC_ScreenBlitHiRes(int x, int y, int w, int h, const void *src, int sw, int sh);
/* A screen area's pixels as drawn, at the PS1's resolution (w x h into dst). */
void PC_ScreenRead(int x, int y, int w, int h, void *dst);

/* The display (the game's Psy-Q DISPENV): the screen area shown, and its size as a TV showed it. */
void PC_ScreenShow(const void *dispenv);
/* Show the display, or black. */
void PC_ScreenEnable(int on);
/* The display at power-on: off. */
void PC_ScreenReset(void);

#ifdef __cplusplus
}
#endif

#endif

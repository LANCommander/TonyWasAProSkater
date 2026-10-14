/* The PS1's STR video pictures (src/pc_vdec.c): full-screen movies, and the songs' 96x64 video on
 * the in-level TVs and in the front end.
 *
 * The game does the variable-length stage itself (DecDCTvlc3, decomp/src/shatter/decdctvlc3.c):
 * it turns each frame's bitstream into run-level halfwords, a header word (its low 16 bits: the
 * size in words) and then the halfwords. This decodes those into a picture: macroblocks of 16x16
 * pixels in columns, top to bottom, then left to right, as the PS1's MDEC delivered them. */
#ifndef PC_VDEC_H
#define PC_VDEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pixel formats: 24-bit (R, G, B bytes), or the 15-bit texture format with the STP bit set. */
#define PC_VDEC_RGB24 0
#define PC_VDEC_15STP 1

/* Decode a frame (runlevel: the game's VLC output) `cols` macroblock columns wide and h pixels
 * high into out, `pitch` bytes per row. */
void PC_VdecFrame(const void *runlevel, int fmt, int cols, int h, void *out, int pitch);

/* A song's video into the texture store (pc_texstore.h): the picture the game's columns covered,
 * from texture x to xend (halfwords: columns of 16 pixels, 24 halfwords each when rgb24) at y,
 * h high. */
void PC_VdecToTexture(const void *runlevel, int rgb24, int x, int y, int xend, int h);

#ifdef __cplusplus
}
#endif

#endif

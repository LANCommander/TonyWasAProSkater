/* The texture store (src/pc_texstore.cpp): the game's textures and CLUTs.
 *
 * The game places its textures and colour tables itself (the Pack allocator, Pal's CLUT slots) at
 * PS1 VRAM coordinates, and its primitives refer to them by those (texture page, CLUT, u, v). The
 * store keeps them at those coordinates: 16-bit texels in the PS1's texture formats, sampled by
 * the renderer. Screens are not in it. */
#ifndef PC_TEXSTORE_H
#define PC_TEXSTORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Texels (w x h halfwords, rows of w) to x, y. */
void PC_TexUpload(int x, int y, int w, int h, const void *src);
/* Texels at x, y (w x h) into dst. */
void PC_TexRead(int x, int y, int w, int h, void *dst);
/* Fill with one colour, rgb = 0x00BBGGRR (8 bits each, as ClearImage). */
void PC_TexFill(int x, int y, int w, int h, uint32_t rgb);

#ifdef __cplusplus
}
#endif

#endif

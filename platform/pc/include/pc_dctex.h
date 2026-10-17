/* Dreamcast models and textures (src/pc_dctex.cpp).
 *
 * The Dreamcast version (Treyarch's port) keeps the PS1 game's .psx model/level files, as
 * version 6 of the format: the same layout, but each textured face carries 16-bit texture
 * coordinates (u0..u3 then v0..v3 in words 5-8, 512 = one repeat of the texture, which may tile)
 * and the textures are 16-bit PowerVR images (up to 256x256, twiddled, VQ-compressed or plain,
 * with mipmaps) instead of 4/8-bit images with palettes. Their checksums are new, so they only
 * go with their own models.
 *
 * Such a file is loaded without its texture images (FileIO, pc_fileio.cpp): those are kept here,
 * by checksum, and drawn by OpenGL. The game's texture entry (Texture) for one gets a texture
 * page with colour depth 3, which the PS1 game never uses, and its CLUT field holds the handle;
 * see Spool_LoadTexturesToVRAM and M3dInit_ParsePSX. On the faces of a version 6 model, word 4
 * names the face's Texture entry: PC_DC_FACE_TEX | its index here (PC_DcFaceTexIndex), as a face
 * word can't hold a native pointer (PS1 faces have u0v0 | clut << 16 there, never with bit 31
 * set). */
#ifndef PC_DCTEX_H
#define PC_DCTEX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PC_HD_TPAGE 0x0180u   /* texture page halfword: colour depth 3 */
#define PC_HD_IS_TPAGE(tp) (((tp) & 0x0180u) == 0x0180u)
/* texture coordinates of an HD texture in primitives: 256 = one repeat */

/* Handle of a registered texture (0 = none). */
uint16_t PC_HdTexFind(uint32_t checksum);
/* A texture entry starts or stops using it; at 0 users its OpenGL texture is freed. */
void PC_HdTexRef(uint16_t handle, int delta);

/* The Dreamcast's soft shadow image (bodyshadow.pvr, or one like it with the PS1's data): its
 * handle, for the Dreamcast's shadows (pc_video.h PC_DrawDcShadows). */
uint16_t PC_DcShadowBlob(void);

/* A Dreamcast face's word 4: PC_DC_FACE_TEX | the index of its Texture entry. */
#define PC_DC_FACE_TEX 0x80000000u
uint32_t PC_DcFaceTexIndex(void *texture);
/* The Texture entry of a face word made by PC_DcFaceTexIndex. */
void *PC_DcFaceTex(uint32_t word);

#ifdef __cplusplus
}
#endif

#endif

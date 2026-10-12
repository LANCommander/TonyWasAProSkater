/* The Dreamcast version's sound banks (.kat) as the PS1 game's VABs (src/pc_dcsound.c). */
#ifndef PC_DCSOUND_H
#define PC_DCSOUND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A VAB for the game to load in place of `vab` ("ware.vab"), made from the .kat of the same name
 * in Data/: its samples are registered with the SPU as PCM (pc_spu.h) and the VAB's samples are
 * their stand-ins. Returns a malloc'd image and its size, or NULL if there is no such .kat. */
uint8_t *PC_DcSoundVab(const char *vab, int *size);

#ifdef __cplusplus
}
#endif

#endif

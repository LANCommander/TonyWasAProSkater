/* The Dreamcast version's sound banks (.kat) as the PS1 game's VABs (src/pc_dcsound.c). */
#ifndef PC_DCSOUND_H
#define PC_DCSOUND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A VAB for the game to load in place of `vab` ("ware.vab"), made from the .kat of the same name
 * in Data/: its samples are registered as PCM (pc_sound.h) and the VAB's samples are
 * their stand-ins. Returns a malloc'd image and its size, or NULL if there is no such .kat. */
uint8_t *PC_DcSoundVab(const char *vab, int *size);

/* Point the game's sound effects at the .kat sounds the Dreamcast version plays for them (some
 * moved, so the PS1 table would play the wrong ones), at its volumes. Once, before the game runs. */
void PC_DcSoundTable(void);

#ifdef __cplusplus
}
#endif

#endif

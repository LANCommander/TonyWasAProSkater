/* The Psy-Q libraries the port still reimplements at their retail addresses (src/hle_*.c): GPU,
 * GTE, CD, SPU, MDEC. Each is on its way to a native subsystem the game calls directly, as the C
 * library, BIOS, memory card, controllers and VSync already have (pc_frame.h, pc_input.h,
 * pc_saves.h).
 *
 * Naming: an HLE entry point is either the Psy-Q function's own name (DrawOTag) or `hle_<name>`
 * where that would clash with the host C library. The build maps both to the retail address of
 * the Psy-Q function (build/psyq_names.tsv). `hle_<8 hex digits>` names an address directly, and
 * `hleov_<8 hex digits>` overrides even a decompiled game function. */
#ifndef PC_HLE_H
#define PC_HLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CD image. Sector reads see the disc image plus the mounted files. */
int PC_CdOpen(void);
/* The PS1 disc image's path, if there is one (no error otherwise): 1 and the path. */
int PC_CdFindImage(char *path, int n);
/* No PS1 disc image is used (the data came from the Dreamcast disc): disc reads fail. */
void PC_CdSetNoDisc(int on);
int PC_CdReadSector(uint32_t lba, void *dst2048);       /* mode 2 form 1 user data */
int PC_CdReadRawSector(uint32_t lba, void *dst2352);    /* whole raw sector */
/* The disc image only: n raw sectors from lba, and ISO 9660 lookup (size in bytes). */
int PC_CdReadDiscSectors(uint32_t lba, uint32_t n, void *dst);
int PC_CdFindDiscFile(const char *name, uint32_t *lba, uint32_t *size);
/* Make a file of raw 2352-byte sectors (an extracted STR) readable as if it were on the disc,
 * at an LBA past the end of the image. Mounting the same file again gives the same LBA. A
 * Dreamcast film (.sfd) mounts as well: streaming it plays its audio (it has no sectors). */
int PC_CdMountFile(const char *path, uint32_t *lba, uint32_t *sectors);
/* The file mounted at lba, or NULL. */
const char *PC_CdMountPath(uint32_t lba);
/* Once per frame: advance a CD stream (CdRead2/CdlReadS) by the sectors read since the last. */
void PC_CdStreamVBlank(void);
/* Seconds of the film mounted at lba that have been heard since its stream started from the
 * top (0 before the mixer gets to it, or if it isn't the film streaming). */
double PC_CdFilmTime(uint32_t lba);
/* The drive is streaming, or STR frames are waiting in the ring (pc_movie.c's waits). */
int PC_CdStreamBusy(void);

/* The soundtrack (pc_music.c): a Dreamcast song's audio ran out (hle_libcd.c), so the song
 * starts over, as a PS1 song does at its end frame. */
void PC_MusicStreamEnded(void);

#ifdef __cplusplus
}
#endif

#endif

/* High-level reimplementations of the Psy-Q libraries and the PS1 BIOS (src/hle_*.c).
 *
 * Naming: an HLE entry point is either the Psy-Q function's own name (DrawOTag) or, where that
 * would clash with the host C library, `hle_<name>` (hle_sprintf). The build maps both to the
 * retail address of the Psy-Q function (build/psyq_names.tsv). `hle_<8 hex digits>` names an
 * address directly, and `hleov_<8 hex digits>` overrides even a decompiled game function. */
#ifndef PC_HLE_H
#define PC_HLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void PC_HleInit(void);

/* BIOS heap (InitHeap/malloc/free) in PS1 RAM. */
void PC_HeapInit(uint32_t addr, uint32_t size);
uint32_t PC_Malloc(uint32_t size);
void PC_Free(uint32_t addr);

/* BIOS events. Classes/specs as in the BIOS (e.g. 0xf4000001 HwCARD, 0x0004 EvSpIOE). */
void PC_DeliverEvent(uint32_t cls, uint32_t spec);

/* A vertical blank is due: run the VSync callbacks and poll input. Called by VSync() and by the
 * HLE functions games spin on (CdReadSync, DrawSync, TestEvent...), so code that busy-waits for
 * interrupt-driven state still sees time pass. Returns the number of vblanks that elapsed. */
int PC_TickVBlank(void);
/* Total vblanks since boot. */
uint32_t PC_VBlankCount(void);

/* Memory card files (hle_libcard.c): the BIOS file API on "bu00:", also reached through the
 * B0 table. dir points at a libapi struct DIRENTRY. */
int hle_open(const char *path, int mode);
int hle_lseek(int fd, int offset, int whence);
int hle_read(int fd, void *buf, int n);
int hle_write(int fd, const void *buf, int n);
int hle_close(int fd);
int hle_erase(const char *path);
int hle_format(const char *path);
void *hle_firstfile(const char *path, void *dir);
void *hle_nextfile(void *dir);

/* Input (pc_input.c, hle_libpad.c): PC_InputPoll fills the libpad receive buffers, once per vblank. */
void PC_InputInit(void);
void PC_InputPoll(void);

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
 * at an LBA past the end of the image. Mounting the same file again gives the same LBA. */
int PC_CdMountFile(const char *path, uint32_t *lba, uint32_t *sectors);
/* Once per vblank: advance a CD stream (CdRead2/CdlReadS) by the sectors read since the last. */
void PC_CdStreamVBlank(void);

#ifdef __cplusplus
}
#endif

#endif

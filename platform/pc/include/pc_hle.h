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

/* Input: fill a libpad receive buffer (34 bytes) for a port. */
void PC_InputInit(void);
void PC_InputPoll(void);
void PC_InputFillPadBuffer(int port, uint8_t *buf);

/* CD image. */
int PC_CdOpen(void);
int PC_CdReadSector(uint32_t lba, void *dst2048);       /* mode 2 form 1 user data */
int PC_CdReadRawSector(uint32_t lba, void *dst2352);    /* whole raw sector */

#ifdef __cplusplus
}
#endif

#endif

/* The Psy-Q library functions the game calls that <libgte.h> and <libgpu.h> don't declare: the C
   library, LIBETC, LIBCD, LIBPRESS, LIBPAD, LIBSPU and LIBSND. The port has only libgte.h and
   libgpu.h of the SDK, so these are declared here, with the SDK's prototypes (C linkage: they're the
   SDK's C functions). printf/sprintf come from Psy-Q's STDIO.H under egcs; everything else is
   declared here: Psy-Q's STRINGS.H has no prototypes, and the port must not take the host's
   <stdio.h> (MinGW's turns sprintf into a call to __mingw_sprintf, not the game's 0x8008b900). */
#ifndef GAME_PSYQ_H
#define GAME_PSYQ_H

/* CPPPSX doesn't define __cplusplus, so the SDK headers' own extern "C" guards are inactive under
   egcs: wrap them, as the TUs do */
extern "C" {
	#include <sys/types.h>
	#ifndef PC_PORT
	#include <stdio.h> /* printf, sprintf */
	#endif
}

struct CdlATV;       /* LIBCD.H: CD audio mix (val0..val3) */
struct SpuVoiceAttr; /* LIBSPU.H */

extern "C" {
	#ifdef PC_PORT
	int printf(const char *fmt, ...);                /* 0x8008c190 */
	int sprintf(char *buffer, const char *fmt, ...); /* 0x8008b900 */
	#endif
	char *strcpy(char *dst, const char *src);                  /* 0x8008e904 */
	unsigned int strlen(const char *s);                        /* 0x8008f6ec */
	int strcmp(const char *s1, const char *s2);                /* 0x8008f6fc */
	void *memcpy(void *dst, const void *src, unsigned int n);  /* 0x8009752c */
	int VSync(int mode);                                       /* 0x8008eebc */
	int CdRead2(long mode);                                    /* 0x8008f0ec */
	void StSetMask(u_long mask, u_long start, u_long end);     /* 0x8008f19c */
	int CdControl(u_char com, u_char *param, u_char *result);  /* 0x8008f264 */
	int CdControlB(u_char com, u_char *param, u_char *result); /* 0x8008f4d4 */
	int CdMix(CdlATV *vol);                                    /* 0x8008f620 */
	void DecDCTin(u_long *buf, int mode);                      /* 0x800902c0 */
	void DecDCTout(u_long *buf, int size);                     /* 0x8009033c */
	int DecDCToutCallback(void (*func)());                     /* 0x8009035c */
	void StUnSetRing(void);                                    /* 0x8009074c */
	unsigned PadEnableCom(unsigned mode);                      /* 0x800920b4 */
	void SpuNSetVoiceAttr(int vNum, SpuVoiceAttr *arg);        /* 0x80093e54 */
	unsigned short SsPitchFromNote(short vabId, short program, unsigned char note,
	                               unsigned char fine); /* 0x80094e44 */
}

#endif

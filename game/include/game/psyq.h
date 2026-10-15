/* The Psy-Q library functions the game calls that <libgte.h> and <libgpu.h> don't declare: the C
   library, LIBETC, LIBCD, LIBPRESS, LIBPAD, LIBSPU and LIBSND. The port has only libgte.h and
   libgpu.h of the SDK, so these are declared here, with the SDK's prototypes (C linkage: they're the
   SDK's C functions). printf/sprintf come from Psy-Q's STDIO.H under egcs; everything else is
   declared here: Psy-Q's STRINGS.H has no prototypes. The PC port doesn't take the host's
   <stdio.h> either, but links these C library calls to the host's own (platform/pc/tools/build.py
   NATIVE_OK; printf becomes the port's trace log, NATIVE_RENAME). */
#ifndef GAME_PSYQ_H
#define GAME_PSYQ_H
#include "game/trig.h"

/* CPPPSX doesn't define __cplusplus, so the SDK headers' own extern "C" guards are inactive under
   egcs: wrap them, as the TUs do */
extern "C" {
	#include <sys/types.h>
}

struct CdlATV;       /* LIBCD.H: CD audio mix (val0..val3) */
struct SpuVoiceAttr; /* LIBSPU.H */

extern "C" {
	int printf(const char *fmt, ...);                /* 0x8008c190 */
	int sprintf(char *buffer, const char *fmt, ...); /* 0x8008b900 */
	char *strcpy(char *dst, const char *src);                  /* 0x8008e904 */
	__SIZE_TYPE__ strlen(const char *s);                       /* 0x8008f6ec */
	int strcmp(const char *s1, const char *s2);                /* 0x8008f6fc */
	void *memcpy(void *dst, const void *src, __SIZE_TYPE__ n); /* 0x8009752c */
	void *memset(void *dst, int c, __SIZE_TYPE__ n);
	void *malloc(__SIZE_TYPE__ n); /* (the port's: the host's) */
	void free(void *p);
	int VSync(int mode);                                       /* 0x8008eebc */
	int CdRead2(int mode);                                    /* 0x8008f0ec */
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

/* globals */
extern SSinCos rcossin_tbl[];
#endif

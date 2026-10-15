/* Psy-Q <sys/types.h> for the PC build: the toolchain's own header plus the BSD / System V
 * short names the PS1 code expects. All widths match the PS1: u_long and ulong are 32 bits (long
 * is on Win32, Win64 and 32-bit Linux; an LP64 system's are unsigned int, and its C library's own
 * u_long/ulong are renamed out of the way). */
#ifndef PSYQ_SYS_TYPES_H
#define PSYQ_SYS_TYPES_H

#if __SIZEOF_LONG__ == 8
#define u_long psyq_libc_u_long
#define ulong psyq_libc_ulong
#include_next <sys/types.h>
#undef u_long
#undef ulong
typedef unsigned int u_long;
typedef unsigned int ulong;
#else
#include_next <sys/types.h>
typedef unsigned long u_long;
typedef unsigned long ulong;
#endif

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned short ushort;
typedef unsigned int uint;

#endif

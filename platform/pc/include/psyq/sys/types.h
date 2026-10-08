/* Psy-Q <sys/types.h> for the PC build: the toolchain's own header plus the BSD / System V
 * short names the PS1 code expects. All widths match the PS1 (long is 32 bits on Win32). */
#ifndef PSYQ_SYS_TYPES_H
#define PSYQ_SYS_TYPES_H

#include_next <sys/types.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#endif

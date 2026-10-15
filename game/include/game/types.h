/* Basic typedefs the game's C++ uses (Neversoft's GLOBALS.H names). Identical to the ones Psy-Q's
   <sys/types.h> declares for ushort/uint/ulong, so including both is fine (C++ allows an identical
   re-typedef). */
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
#if __SIZEOF_LONG__ == 8
typedef unsigned int ulong; /* (32 bits, as on the PS1; an LP64 system's long is 64) */
#else
typedef unsigned long ulong;
#endif

#endif

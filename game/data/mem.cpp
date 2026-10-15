/* Decompiled data: the retail executable's initialised globals that src/mem/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"

struct SBlockHeader;

int DAT_800d1b38[2] = {
	100, 0x1000032,
};

char *DAT_800d1b40;
char *DAT_800d1b44;

/* THPS2 demo: int */
int LowMemory;

/* THPS2 demo: uint */
unsigned int UniqueIndentifier;

/* THPS2 demo: SBlockHeader*[2] */
SBlockHeader *FirstFreeBlock[2];

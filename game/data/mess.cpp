/* Decompiled data: the retail executable's initialised globals that src/mess/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/font.h"

struct Message;
struct SWobble;

/* FontInfo[10] (THPS2 demo: FontInfo[10]) */
FontInfo Font_table[10];

/* THPS2 demo: int */
int Num_fonts[1];

/* THPS2 demo: int */
int Current_font[1];

SWobble *DAT_800d15e8;

/* THPS2 demo: Message* */
Message *pMessages;

/* THPS2 demo: uchar */
unsigned char TextJustification[4];

/* THPS2 demo: uint */
unsigned int FlagsBGR[1] = {
	0x2c808080,
};

/* THPS2 demo: ushort */
unsigned short Scale[1];

/* THPS2 demo: ushort */
unsigned short Sort[3] = {
	0, 64,
};

/* THPS2 demo: ushort */
unsigned short Clut[1];

/* THPS2 demo: uchar */
unsigned char NewFont[2];

/* THPS2 demo: int */
int Pack_NoRight[1];

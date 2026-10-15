/* Decompiled data: the retail executable's initialised globals that src/mess/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/font.h"

struct Message;
struct SWobble;

/* FontInfo[10] (THPS2 demo: FontInfo[10]) */
PSX_DATA(0x800a6110) FontInfo Font_table[10];

/* THPS2 demo: int */
PSX_DATA(0x800d15e0) int Num_fonts[1];

/* THPS2 demo: int */
PSX_DATA(0x800d15e4) int Current_font[1];

PSX_DATA(0x800d15e8) SWobble *DAT_800d15e8;

/* THPS2 demo: Message* */
PSX_DATA(0x800d15ec) Message *pMessages;

/* THPS2 demo: uchar */
PSX_DATA(0x800d15f0) unsigned char TextJustification[4];

/* THPS2 demo: uint */
PSX_DATA(0x800d15f4) unsigned int FlagsBGR[1] = {
	0x2c808080,
};

/* THPS2 demo: ushort */
PSX_DATA(0x800d15f8) unsigned short Scale[1];

/* THPS2 demo: ushort */
PSX_DATA(0x800d15fa) unsigned short Sort[3] = {
	0, 64,
};

/* THPS2 demo: ushort */
PSX_DATA(0x800d1600) unsigned short Clut[1];

/* THPS2 demo: uchar */
PSX_DATA(0x800d1602) unsigned char NewFont[2];

/* THPS2 demo: int */
PSX_DATA(0x800d1a70) int Pack_NoRight[1];

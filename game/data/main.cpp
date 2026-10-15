/* Decompiled data: the retail executable's initialised globals that src/main/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"

struct CBody;
struct CBruce;
struct CPowerUp;

extern char FUN_80096c70[];
extern char FUN_80097060[];
extern char FUN_80097100[];

int DAT_8009f5cc[12] = {
	0x20746f6e, 0x2e2e2e2e, 0x444e4520, 0x45444f43, 0x4557545f, 0x554a4b41, 0xa504d, 0x20686f0a,
	0x74696873, 0x21212121,
};

int DAT_800d0010[1];

void *DAT_800d0160[5] = {
	FUN_80096c70, FUN_80097060, 0, FUN_80097100, 0 /* (Psy-Q library data) */,
};

int DAT_800d01d8[3];

int DAT_800d05f0[13] = {
	0xfb28, 0xfc18, 0xf6f8, 0xf204, 0xea44, 0xe128, 0xcff8, 0xcff8, 0xf880, 0, 0, 0x117350,
	0x450000,
};

int DAT_800d0734[134] = {
	0x9c00c0f0, 0x4ec05280, 0x31b03e4, 0x2af03a4, 0x2660372, 0x25d031c, 0x18e025c, 0x135022f,
	0xb701d2, 0xb5018f, 0x8000b4, 0x26004c, 0x80008000, 0, 0x7f00b1, 0x4fa870f0, 0x4510bce0,
	0xb4c0bef0, 0x4ec05280, 0x76b0904, 0x65f0824, 0x61607a2, 0x5ed076c, 0x42e05ec, 0x305050f,
	0x2b70462, 0x265042f, 0x1b20264, 0x800100, 0x80008000, 0, 0xa900e3, 0x4fa86f60, 0x4510bce0,
	0xa680bef0, 0x52c05680, 0xb580dfb, 0xa3c0d09, 0x9730bd9, 0x8da0b59, 0x5e908d9, 0x4b007ec,
	0x3d206ef, 0x31d05ea, 0x238031c, 0xaa0154, 0x80008000, 0, 0x13901a5, 0x50006000, 0xb8004c00,
	0xc000bc00, 0x5c006000, 0x11bb15ba, 0x10bd14c2, 0xdc111bc, 0xdc311c0, 0x9c10dc0, 0x7c10bc4,
	0x6cd0a00, 0x5c109c2, 0x41a05c0, 0x13a0274, 0x80008000, 0, 0x231033d, 0x50007e00, 0xb000b400,
	0xb0004c00, 0x54006000, 0x1a311ed6, 0x183b1d14, 0x16b21bc2, 0x15ef1a32, 0x105515ee, 0xf2d1334,
	0xc5d11f6, 0xae11056, 0x7a20ae0, 0x2320464, 0x80008000, 0, 0x10001, 0x7fff7fff, 0, 0x81000000,
	0, 0xfff1fff, 0x51005, 0, 0x51005, 0, 0, 0, 0, 0x10021004, 0x20004, 0x80008000, 0, 0x10001,
	0x7fff7fff, 0, 0, 0, 0xfff1fff, 0x51005, 0, 0x51005, 0, 0, 0, 0, 0x10021004, 0x20004,
	0x80008000, 0, 0x130017, 0x4fa870f0, 0x4510bce0, 0x8500bef0, 0x54c05f80, 0x2af0371, 0x1df02e5,
	0x1d702b0, 0x26a0358, 0x11e01d6, 0xb1012d, 0x59011f, 0xe301a0, 0x400058, 0x140028, 0x80008000,
};

int D_800d10d8[1] = {
	1,
};

/* THPS2 demo: CBody* */
CBody *PowerUpList;

/* THPS2 demo: int */
int GAutoPlayer1a[1];

/* THPS2 demo: int */
int GAutoPlayer2a[1];

/* THPS2 demo: int */
int GAutoLevela[1];

/* THPS2 demo: int */
int GAutoGamea[1];

/* THPS2 demo: int */
int Mick_MapMode[1];

/* THPS2 demo: int */
int Mick_MapDoit[1];

/* EEndCode (THPS2 demo: EEndCode) */
int EndCode[1];

/* THPS2 demo: int */
int EveryOther[1];

/* THPS2 demo: int */
int Restart[1];

/* THPS2 demo: int */
int Mick_Darkness[1];

/* THPS2 demo: int */
int GSpin[1];

/* THPS2 demo: int */
int GSpinSpeed[1];

/* THPS2 demo: int[2] */
int GSkaterRegion[1];

int DAT_800d1798[1];

/* THPS2 demo: char */
unsigned char GVideoFlags[4];

CBruce *DAT_800d17cc;

/* THPS2 demo: CPowerUp* */
CPowerUp *GPowS;

/* THPS2 demo: CPowerUp* */
CPowerUp *GPowK;

/* THPS2 demo: CPowerUp* */
CPowerUp *GPowA;

/* THPS2 demo: CPowerUp* */
CPowerUp *GPowT;

/* THPS2 demo: CPowerUp* */
CPowerUp *GPowE;

/* THPS2 demo: CPowerUp* */
CPowerUp *GPowTape;

/* THPS2 demo: int */
int GClear1[1];

/* THPS2 demo: int */
int GClear2[17];

/* THPS2 demo: int */
int CloseCam[1];

/* THPS2 demo: int */
int PrimitiveCount[1];

int DAT_800d183c[1];

/* THPS2 demo: int */
int VblankUpdateBar[1];

/* THPS2 demo: short */
short M3d_OTPushback[1];

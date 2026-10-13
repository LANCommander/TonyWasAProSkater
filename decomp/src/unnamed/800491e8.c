/* Db_Init__Fv (0x800491e8) flags=-fstructure-noalias */
#include "names.h"

typedef struct {
	short x, y, w, h;
} R;

typedef struct {
	char p0[0x78];
} Z;

typedef struct {
	char p0[0x50];
	int arr[10];
} E;

typedef struct {
	char p0[0x70];
	int f70, f74, f78, f7c;
	char p80[0x84 - 0x80];
	char p84[0x8090 - 0x84];
	int f8090;
	char p8094[0x80a8 - 0x8094];
	char *f80a8;
	char p80ac[0x811c - 0x80ac];
	int f811c, f8120, f8124, f8128;
	char p812c[0x1013c - 0x812c];
	int f1013c;
	char p10140[0x10154 - 0x10140];
	char *f10154;
} S;

extern S DoubleBuffer;
extern char DAT_800b09c8[], DAT_800b0a24[], DAT_800b0a4c[];
extern E Levels[];
extern int GNumberOfPlayers, GLevel;

int Db_SkyColor;

extern R DAT_800d1a40[];

char *pDoubleBuffer, *pOtherBuffer;

extern void FUN_8008c3d0(int);
extern int DrawSync(int);
extern char *Mem_NewX(int, int, int);
extern void FUN_80092e70(void *, int, int, int, int);
extern void SetDefDispEnv(void *, int, int, int, int);
extern void FUN_80048d00(void);
extern void Db_UpdateSky(void);
extern void FUN_8008c94c(void *);
extern void PutDispEnv(void *);
extern int Db_OTSize(void);
extern void FUN_8008c7d4(void *, int);
extern int FUN_8008c67c(void *, int, int, int);
extern void M3dInit_SetResolution(int, int);

void Db_Init(void) {
	R r;

	FUN_8008c3d0(0);
	DrawSync(0);

	Db_SkyColor = 0;

	if (DoubleBuffer.f80a8 == 0) {
		DoubleBuffer.f80a8 =
		    Mem_NewX((Levels[GLevel].arr[GNumberOfPlayers - 1] + 0x17c00) * 2, 1, 1);
		DoubleBuffer.f10154 =
		    DoubleBuffer.f80a8 + (Levels[GLevel].arr[GNumberOfPlayers - 1] + 0x17c00);
	}

	DoubleBuffer.f8090 = 0;
	DoubleBuffer.f1013c = 0x100;
	FUN_80092e70(&DoubleBuffer, 0, 0, 512, 240);
	FUN_80092e70(DAT_800b09c8, 0, 256, 512, 240);
	SetDefDispEnv((char *)&DoubleBuffer + 92, 0, 256, 512, 240);
	SetDefDispEnv(DAT_800b0a24, 0, 0, 512, 240);
	FUN_80048d00();
	Db_UpdateSky();
	FUN_8008c94c(&DoubleBuffer);
	PutDispEnv((char *)&DoubleBuffer + 92);

	DoubleBuffer.f70 = 0;
	DoubleBuffer.f811c = 0;
	DoubleBuffer.f78 = 0;
	DoubleBuffer.f8124 = 0;
	DoubleBuffer.f7c = 0;
	DoubleBuffer.f8128 = 0;
	DoubleBuffer.f74 = 0;
	DoubleBuffer.f8120 = 0;
	pDoubleBuffer = DAT_800b09c8;
	pOtherBuffer = (char *)&DoubleBuffer;
	FUN_8008c7d4((char *)&DoubleBuffer + 132, Db_OTSize());
	FUN_8008c7d4(DAT_800b0a4c, Db_OTSize());

	r = DAT_800d1a40[0];
	FUN_8008c67c(&r, 0, 0, 0);
	DrawSync(0);

	r.y = 511;
	r.h = 1;
	FUN_8008c67c(&r, 0, 0, 0);
	DrawSync(0);
	FUN_8008c3d0(1);
	M3dInit_SetResolution(512, 240);
}

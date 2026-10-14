/* HiRes_Init__Fi (0x8006b01c) flags=-fstrict-aliasing */
#include "names.h"
#ifdef PC_PORT
#include "pc_draw.h"
#include "pc_screen.h"
#endif

typedef struct {
	char p0[0x17];
	char b17, b18, b19, b1a, b1b;
	char p1[0x80a8 - 0x1c];
	int f80a8;
	char p2[0x80c3 - 0x80ac];
	char b80c3, b80c4;
	char p3[0x10154 - 0x80c5];
	int f10154;
} G;

typedef struct {
	char p0[0x19];
	char b19, b1a, b1b;
} H;

extern G DoubleBuffer;
extern H DAT_800b09c8;
extern char DAT_800b0a24[];
extern G *pDoubleBuffer;
extern int TimeScale, PolyBufferEnd, pPoly;
extern short M3d_OTPushback, M3d_OTPushback2;

void FUN_8008c3d0(int);
int Mem_NewX(int, int, int);
void FUN_80092e70(void *, int, int, int, int);
void SetDefDispEnv(void *, int, int, int, int);
void FUN_80048d00(void);
int FUN_8008c94c(void *);
void PutDispEnv(void *);
void HiRes_BlankScreen(void);
void M3dInit_SetResolution(int, int);

void HiRes_Init(int a) {
	int v;
	char *s2;

#ifdef PC_PORT
	PC_ScreenEnable(0);
#else
	FUN_8008c3d0(0);
#endif

	v = Mem_NewX(0x14000, 1, 1);
	DoubleBuffer.f80a8 = v;
	DoubleBuffer.f10154 = v + 0xa000;
	FUN_80092e70(&DoubleBuffer, 0, 0, 640, 480);

	s2 = (char *)&DoubleBuffer + 92;
	SetDefDispEnv(s2, 0, 0, 640, 480);
	FUN_80092e70(&DAT_800b09c8, 0, 0, 640, 480);
	SetDefDispEnv(DAT_800b0a24, 0, 0, 640, 480);
	FUN_80048d00();

	DoubleBuffer.b17 = 0;
	DoubleBuffer.b80c3 = 0;
	DoubleBuffer.b18 = 0;
	DoubleBuffer.b80c4 = 0;
	DoubleBuffer.b19 = 255;
	DoubleBuffer.b1a = 0;
	DoubleBuffer.b1b = 0;
	DAT_800b09c8.b19 = 0;
	DAT_800b09c8.b1a = 0;
	DAT_800b09c8.b1b = 255;
#ifdef PC_PORT
	PC_DrawSetEnv(&DoubleBuffer);
	PC_ScreenShow((void *)(s2));
#else
	FUN_8008c94c(&DoubleBuffer);
	PutDispEnv(s2);
#endif

	if (a)
		HiRes_BlankScreen();

	DoubleBuffer.b17 = 0;
	DoubleBuffer.b18 = 0;
#ifdef PC_PORT
	PC_DrawSetEnv(&DoubleBuffer);
#else
	FUN_8008c94c(&DoubleBuffer);
#endif

	pDoubleBuffer = &DoubleBuffer;
	PolyBufferEnd = -1;
	pPoly = DoubleBuffer.f80a8;
	M3dInit_SetResolution(640, 480);

	TimeScale = 128;
	M3d_OTPushback = 0;
	M3d_OTPushback2 = 0;
}

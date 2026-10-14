/* HiRes_Init__Fi (0x8006b01c) flags=-fstrict-aliasing */
#include "names.h"
#include "game/dbuffer.h"
#include "pc_draw.h"
#include "pc_screen.h"

extern unsigned char *pPoly, *PolyBufferEnd;
extern int TimeScale;
extern short M3d_OTPushback, M3d_OTPushback2;

void FUN_8008c3d0(int);
unsigned char *Mem_NewX(int, int, int);
void FUN_80092e70(void *, int, int, int, int);
void FUN_80048d00(void);
int FUN_8008c94c(void *);
void HiRes_BlankScreen(void);
void M3dInit_SetResolution(int, int);

void HiRes_Init(int a) {
	unsigned char *v;

	PC_ScreenEnable(0);

	v = Mem_NewX(DB_PACKET_MEMORY(0xa000) * 2, 1, 1);
	DoubleBuffer[0].Polys = v;
	DoubleBuffer[1].Polys = v + DB_PACKET_MEMORY(0xa000);
	FUN_80092e70(&DoubleBuffer[0].Draw, 0, 0, 640, 480);

	SetDefDispEnv(&DoubleBuffer[0].Disp, 0, 0, 640, 480);
	FUN_80092e70(&DoubleBuffer[1].Draw, 0, 0, 640, 480);
	SetDefDispEnv(&DoubleBuffer[1].Disp, 0, 0, 640, 480);
	FUN_80048d00();

	DoubleBuffer[0].Draw.dfe = 0;
	DoubleBuffer[1].Draw.dfe = 0;
	DoubleBuffer[0].Draw.isbg = 0;
	DoubleBuffer[1].Draw.isbg = 0;
	DoubleBuffer[0].Draw.r0 = 255;
	DoubleBuffer[0].Draw.g0 = 0;
	DoubleBuffer[0].Draw.b0 = 0;
	DoubleBuffer[1].Draw.r0 = 0;
	DoubleBuffer[1].Draw.g0 = 0;
	DoubleBuffer[1].Draw.b0 = 255;
	PC_DrawSetEnv(&DoubleBuffer[0].Draw);
	PC_ScreenShow(&DoubleBuffer[0].Disp);
	PC_DrawMark2D(&DoubleBuffer[0].OT2D);
	PC_DrawMark2D(&DoubleBuffer[0].OT);
	PC_DrawMark2D(&DoubleBuffer[1].OT2D);
	PC_DrawMark2D(&DoubleBuffer[1].OT);

	if (a)
		HiRes_BlankScreen();

	DoubleBuffer[0].Draw.dfe = 0;
	DoubleBuffer[0].Draw.isbg = 0;
	PC_DrawSetEnv(&DoubleBuffer[0].Draw);

	pDoubleBuffer = &DoubleBuffer[0];
	PolyBufferEnd = (unsigned char *)-1;
	pPoly = DoubleBuffer[0].Polys;
	M3dInit_SetResolution(640, 480);

	TimeScale = 128;
	M3d_OTPushback = 0;
	M3d_OTPushback2 = 0;
}

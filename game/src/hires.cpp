/* HIRES.CPP: the 640x480 front-end display: double-buffered ordering tables, blanking, the
   preview-model camera (declarations in game/hires.h). */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include "pc_screen.h"
#include "pc_draw.h"
#include "game/hires.h"
#include "game/dbuffer.h"
#include "game/career.h"
#include "game/camera.h"
#include "game/m3d.h"
#include "game/m3dasm.h"
#include "game/bit.h"
#include "game/db.h"
#include "game/front.h"
#include "game/frontend.h"
#include "game/globals.h"
#include "game/m3dinit.h"
#include "game/main.h"
#include "game/mem.h"
#include "game/physics.h"
#include "game/sfx.h"
#include "game/utils.h"

extern RECT DAT_800d1e20[];
void FUN_8008c3d0(int);
void FUN_80092e70(void *, int, int, int, int);
int FUN_8008c94c(void *);
extern SViewport DAT_800bf614;
/* set by front-end screens that redraw part of the background themselves (platform/pc) */
extern void (*pc_hires_before_draw)(void);

/* HiRes_BlankScreen__Fv (0x8006af64) */
void HiRes_BlankScreen(void) {
	RECT r;

	PC_ScreenEnable(0);

	r = DAT_800d1e20[0];
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);

	r.y = 511;
	r.h = 1;
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);

	DoubleBuffer[0].Draw.dfe = 0;
	DoubleBuffer[0].Draw.isbg = 0;
	PC_DrawSetEnv(&DoubleBuffer[0].Draw);
	PC_ScreenEnable(1);
}

/* HiRes_Init__Fi (0x8006b01c) */
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

/* HiRes_SetupCamera__Fv (0x8006b1c8) */
SViewport *HiRes_SetupCamera(void) {
	DAT_800bf614.TanHalf = 3413;
	M3dMaths_SetIdentityRotation(&MikeCamera[0].Transform);

	MikeCamera[0].Position.vx = 0;
	MikeCamera[0].Position.vy = 0;
	MikeCamera[0].Position.vz = -200;
	TransMatrix(&MikeCamera[0].Transform, &MikeCamera[0].Position);
	M3dInit_SetFoggingParams(10, 6000, 2048);
	return &DAT_800bf614;
}

/* HiRes_ClearOTag__Fv (0x8006b240) */
void HiRes_ClearOTag(void) {
	PC_DrawClearOTR(&pDoubleBuffer->OT2D, 8194);
}

/* HiRes_DrawOTag__Fv (0x8006b26c) */
void HiRes_DrawOTag(void) {
	SDoubleBuffer *a;

	FlickerSync();
	/* after the wait for the next field, so the redraw and this frame's primitives are shown together */
	if (pc_hires_before_draw)
		pc_hires_before_draw();
	PC_DrawOT(&pDoubleBuffer->OT3D[0x1000]);

	a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = &DoubleBuffer[1];

	pDoubleBuffer = a;
	pPoly = a->Polys;
	PollHost();
}

/* HiRes_DrawOTagNoSync__Fv (0x8006b2f4) */
void HiRes_DrawOTagNoSync(void) {
	SDoubleBuffer *a;

	PC_DrawOT(&pDoubleBuffer->OT3D[0x1000]);

	a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = &DoubleBuffer[1];

	pDoubleBuffer = a;
	pPoly = a->Polys;
	PollHost();
}

/* HiRes_DrawOTagAll__Fv (0x8006b374) */
void HiRes_DrawOTagAll(void) {
	SDoubleBuffer *a;

	PC_DrawOT(&pDoubleBuffer->OT3D[0x1fff]);

	a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = &DoubleBuffer[1];

	pDoubleBuffer = a;
	pPoly = a->Polys;
	PollHost();
}

/* FUN_8006b3f8 (0x8006b3f8) */
void FUN_8006b3f8(void) {
	volatile int pad[2];

	Front_Init();
	ResetMainDemoTimer();
	SFX_Init("shell.vab");
	Front_Main();
}

/* kickIt__Fii (0x8006b434) */
void kickIt(int a, int b) {
	char *n;

	GLevel = a;
	n = Levels[a].pTRGName;
	SelectedSkater = b;
	Utils_CopyString(n, StartupGameState.State.TrigFileName);

	StartupGameState.State.RestartName[0] = 0;
	LaunchGame();
}

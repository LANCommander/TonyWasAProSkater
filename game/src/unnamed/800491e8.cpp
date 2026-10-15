/* Db_Init__Fv (0x800491e8) flags=-fstructure-noalias */
#include "game/dbuffer.h"
#include "game/career.h"
#include "game/mem.h"
#include "pc_screen.h"
#include "pc_draw.h"

typedef struct {
	short x, y, w, h;
} R;

extern SLevel Levels[];
extern int GNumberOfPlayers, GLevel;

int Db_SkyColor;

extern R DAT_800d1a40[];

SDoubleBuffer *pOtherBuffer;

extern "C" {
void FUN_8008c3d0(int);
void FUN_80092e70(void *, int, int, int, int);
void FUN_80048d00(void);
void FUN_8008c94c(void *);
void FUN_8008c7d4(void *, int);
int FUN_8008c67c(void *, int, int, int);
}

void Db_UpdateSky();
int Db_OTSize();
void M3dInit_SetResolution(int, int);

void Db_Init() {
	R r;

	PC_ScreenEnable(0);
	DrawSync(0);

	Db_SkyColor = 0;

	if (DoubleBuffer[0].Polys == 0) {
		int size = DB_PACKET_MEMORY(Levels[GLevel].ExtraPolyBuffer[GNumberOfPlayers - 1] + 0x17c00);

		DoubleBuffer[0].Polys = (unsigned char *)Mem_NewX(size * 2, 1, 1);
		DoubleBuffer[1].Polys = DoubleBuffer[0].Polys + size;
	}

	DoubleBuffer[0].Top = 0;
	DoubleBuffer[1].Top = 0x100;
	FUN_80092e70(&DoubleBuffer[0].Draw, 0, 0, 512, 240);
	FUN_80092e70(&DoubleBuffer[1].Draw, 0, 256, 512, 240);
	SetDefDispEnv(&DoubleBuffer[0].Disp, 0, 256, 512, 240);
	SetDefDispEnv(&DoubleBuffer[1].Disp, 0, 0, 512, 240);
	FUN_80048d00();
	Db_UpdateSky();
	PC_DrawSetEnv(&DoubleBuffer[0].Draw);
	PC_ScreenShow(&DoubleBuffer[0].Disp);

	DoubleBuffer[0].Visible = 0;
	DoubleBuffer[1].Visible = 0;
	DoubleBuffer[0].Rendering = 0;
	DoubleBuffer[1].Rendering = 0;
	DoubleBuffer[0].FlipNow = 0;
	DoubleBuffer[1].FlipNow = 0;
	DoubleBuffer[0].RenderNow = 0;
	DoubleBuffer[1].RenderNow = 0;
	pDoubleBuffer = &DoubleBuffer[1];
	pOtherBuffer = &DoubleBuffer[0];
	PC_DrawClearOTR(&DoubleBuffer[0].OT2D, Db_OTSize());
	PC_DrawClearOTR(&DoubleBuffer[1].OT2D, Db_OTSize());
	PC_DrawMark2D(&DoubleBuffer[0].OT2D);
	PC_DrawMark2D(&DoubleBuffer[0].OT);
	PC_DrawMark2D(&DoubleBuffer[1].OT2D);
	PC_DrawMark2D(&DoubleBuffer[1].OT);

	r = DAT_800d1a40[0];
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);

	r.y = 511;
	r.h = 1;
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);
	PC_ScreenEnable(1);
	M3dInit_SetResolution(512, 240);
}

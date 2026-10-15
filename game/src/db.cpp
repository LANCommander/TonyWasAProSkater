/* DB.CPP: the display double buffers: ordering-table size, the sky colour, the draw and
   vsync callbacks, rendering a frame and setting up. Declarations in game/db.h and
   game/dbuffer.h. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/db.h"
#include "game/dbuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include "pc_draw.h"
#include "game/m3d.h"
#include "pc_screen.h"
#include "game/m3dasm.h"
#include "game/career.h"
#include "game/mem.h"
#include "game/globals.h"
#include "game/m3dinit.h"
#include "game/shellutils.h"
#include "game/utils.h"

SDoubleBuffer DoubleBuffer[2];
extern SDoubleBuffer DoubleBuffer[2];
extern SDoubleBuffer *pVB;
void Pause__Fi(int);
void Db_CallBack(void);
extern SDoubleBuffer *pOtherBuffer;
void FUN_8008c370(void (*f)(void));
typedef struct {
	short x, y, w, h;
} R;
extern R DAT_800d1a40[];
extern "C" {
void FUN_8008c3d0(int);
void FUN_80092e70(void *, int, int, int, int);
void FUN_8008c94c(void *);
void FUN_8008c7d4(void *, int);
int FUN_8008c67c(void *, int, int, int);
}
extern int DAT_800d1a38;

/* Db_OTSize__Fv (0x80048c74) */
int Db_OTSize(void) {
	return (GNumberOfPlayers << 12) | 2;
}

/* Db_UpdateSky__Fv (0x80048c8c) */
/* The sky colour (-1: none) as both buffers' background fill. */
void Db_UpdateSky(void) {
	unsigned int c = Db_SkyColor;

	if (c == -1) {
		DoubleBuffer[0].Draw.isbg = 0;
		DoubleBuffer[1].Draw.isbg = 0;
	} else {
		DoubleBuffer[0].Draw.isbg = 1;
		DoubleBuffer[0].Draw.r0 = c >> 16;
		DoubleBuffer[0].Draw.g0 = c >> 8;
		DoubleBuffer[0].Draw.b0 = Db_SkyColor;
		DoubleBuffer[1].Draw.isbg = 1;
		DoubleBuffer[1].Draw.r0 = c >> 16;
		DoubleBuffer[1].Draw.g0 = c >> 8;
		DoubleBuffer[1].Draw.b0 = Db_SkyColor;
	}
}

/* FUN_80048d00 (0x80048d00) */
/* Both display buffers back at the default screen position. */
void FUN_80048d00(void) {
	DoubleBuffer[1].Disp.screen.x = 0;
	DoubleBuffer[0].Disp.screen.x = 0;
	DoubleBuffer[1].Disp.screen.y = 0;
	DoubleBuffer[0].Disp.screen.y = 0;
}

/* Db_DrawPending__Fv (0x80048d24) */
/* Draws a buffer whose frame is ready once neither buffer is being drawn: its drawing
   environment, then its ordering table from the last entry. */
void Db_DrawPending(void) {
	int i;

	for (i = 0; i < 2; i++) {
		if (DoubleBuffer[i].RenderNow) {
			if (!DoubleBuffer[i].Rendering && !DoubleBuffer[1 - i].Rendering &&
			    !DoubleBuffer[i].Visible) {
				DoubleBuffer[i].RenderNow = 0;
				DoubleBuffer[i].Rendering = 1;
				PC_DrawSetEnv(&DoubleBuffer[i].Draw);
				PC_DrawOT(&(&DoubleBuffer[i].OT2D)[Db_OTSize() - 1]);
				if (getenv("THPS_TRACE_CULL")) {
					extern volatile unsigned pc_heartbeat;
					fprintf(stderr, "d24_fired f%u buf=%d\n", pc_heartbeat, i);
				}
			}
			else if (getenv("THPS_TRACE_CULL")) {
				extern volatile unsigned pc_heartbeat;
				fprintf(stderr, "d24_blocked f%u buf=%d Rendering=%d,%d Visible=%d\n", pc_heartbeat,
				        i, DoubleBuffer[i].Rendering, DoubleBuffer[1 - i].Rendering,
				        DoubleBuffer[i].Visible);
			}
		}
	}
}

/* Db_CallBack__Fv (0x80048e40) */
/* Demo twin; locals from its MAIN.SYM. Draw-sync callback for the two display
   buffers: once the GPU is idle, any buffer waiting to be drawn is marked done. */
/* the display buffers (game/dbuffer.h) */
void Db_CallBack() {
	if (!DoubleBuffer[0].Rendering && !DoubleBuffer[1].Rendering)
		return;

	if (!DrawSync(1)) {
		int i;

		for (i = 0; i < 2; i++) {
			if (DoubleBuffer[i].Rendering) {
				DoubleBuffer[i].Rendering = 0;
				DoubleBuffer[i].FlipNow = 1;
			}
		}
	}
}

/* Db_Vblank__Fv (0x80048ed8) */
/* Demo twin Db_Vblank; THPS1 moved the demo's second loop into Db_DrawPending.
   Flips any buffer that finished drawing. The unused frame delta (a dead read of the volatile
   vblank counter, demo line 209) is what keeps &LastVblanks out of the loop's giv set. */
void Db_Vblank() {
	int i;

	for (i = 0; i < 2; i++) {
		if (DoubleBuffer[i].FlipNow) {
			int frames = Vblanks - DoubleBuffer[i].LastVblanks;

			PC_ScreenShow((void *)(&DoubleBuffer[1 - i].Disp));

			DoubleBuffer[i].FlipNow = 0;
			DoubleBuffer[i].Visible = 1;
			DoubleBuffer[1 - i].Visible = 0;
			DoubleBuffer[i].LastVblanks = Vblanks;
		}
	}

	Db_DrawPending();
}

/* Db_WaitRender__Fv (0x80048fec) */
/* Waits until the buffer about to be built has been drawn, then empties its ordering table and
   packet memory. */
void Db_WaitRender(void) {
	pVB = pDoubleBuffer;

	while (pVB->RenderNow)
		Pause__Fi(1);

	if (pDoubleBuffer->Rendering) {
		DrawSync(0);
		Db_CallBack();
	}

	PC_DrawClearOTR(&pDoubleBuffer->OT2D, Db_OTSize());
	pPoly = pDoubleBuffer->Polys;
}

/* Db_Render__Fv (0x80049094) */
/* Demo twin Db_Render: waits for the other buffer's render, sets the draw area to
   this buffer's 512x240 window, links the clip prim into the OT, kicks off the draw and swaps the
   two buffer pointers. THPS1's SDoubleBuffer embeds the OT instead of pointing at it. */
void Db_Render() {
	if (pOtherBuffer->RenderNow) {
		if (getenv("THPS_TRACE_CULL")) {
			extern volatile unsigned pc_heartbeat;
			fprintf(stderr, "db_render_stall f%u other=%p RenderNow=%d Rendering=%d\n",
			        pc_heartbeat, (void *)pOtherBuffer, pOtherBuffer->RenderNow,
			        pOtherBuffer->Rendering);
		}
		Pause(1);
	}

	setRECT(&pDoubleBuffer->Window, 0, pDoubleBuffer->Top, 512, 240);
	SetDrawArea(&pDoubleBuffer->Clip, &pDoubleBuffer->Window);

	addPrim(&pDoubleBuffer->OT, &pDoubleBuffer->Clip);
	CompactOTagR(&pDoubleBuffer->OT2D, Db_OTSize());

	pDoubleBuffer->RenderNow = 1;
	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "db_render f%u buf=%p\n", pc_heartbeat, (void *)pDoubleBuffer);
	}
	SDoubleBuffer *q = DoubleBuffer;
	SDoubleBuffer *p = q;

	if (pDoubleBuffer == q)
		p++;

	pDoubleBuffer = p;

	if (p == q)
		q++;

	pOtherBuffer = q;
}

/* FUN_800491a0 (0x800491a0) */
void FUN_800491a0(void) {
	PC_DrawSetDone((void (*)(void))(Db_CallBack));
}

/* Db_Reset__Fv (0x800491c4) */
/* Forgets both buffers' packet memory. */
void Db_Reset(void) {
	DoubleBuffer[0].Polys = 0;
	DoubleBuffer[1].Polys = 0;
}

/* Db_Init__Fv (0x800491e8) */
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

/* FUN_80049474 (0x80049474) */
/* Swaps to the other buffer, with its ordering table cleared and its packet memory empty. */
void FUN_80049474(void) {
	SDoubleBuffer *a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = &DoubleBuffer[1];

	pDoubleBuffer = a;
	PC_DrawClearOTR(&pDoubleBuffer->OT2D, Db_OTSize());
	pPoly = pDoubleBuffer->Polys;
}

/* FUN_800494dc (0x800494dc) */
/* Draws the buffer being built now: its environment and display, the background picture
   (pBMR) when there is one, then its ordering table. */
void FUN_800494dc(void) {
	RECT r;

	PC_DrawSetEnv(&pDoubleBuffer->Draw);
	PC_ScreenShow(&pDoubleBuffer->Disp);

	if (DAT_800d1a38 && pBMR) {
		r.x = 0;
		r.y = (unsigned short)pDoubleBuffer->Top;
		r.w = 512;
		r.h = 240;
		PC_ScreenBlit(r.x, r.y, r.w, r.h, pBMR);
		DrawSync(0);
	} else {
		DAT_800d1a38 = 0;
	}

	CompactOTagR(&pDoubleBuffer->OT2D, Db_OTSize());

	if (DAT_800d1a38)
		DrawSync(0);

	PC_DrawOT(&(&pDoubleBuffer->OT2D)[Db_OTSize() - 1]);
}

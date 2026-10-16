/* CREDITS.CPP: the credits / music-list scroller, with its video inset and rotating skater
   preview (THPS1 code, no demo counterpart; declarations in game/credits.h). */

#include "game/credits.h"
#include "game/padbuttons.h"
#include "pc_frame.h"
#include "game/dbuffer.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/m3d.h"
#include "game/camera.h"
#include "game/mem.h"
#include "game/psyq.h"
#include "game/bit.h"
#include "game/db.h"
#include "game/fileio.h"
#include "game/front.h"
#include "game/frontend.h"
#include "game/gamefmv.h"
#include "game/globals.h"
#include "game/hires.h"
#include "game/main.h"
#include "game/mess.h"
#include "game/pack.h"
#include "game/pad.h"
#include "game/sfx.h"
#include "game/shellutils.h"
#include "game/spool.h"

extern int DAT_800d1b98[1];
void FUN_8005027c(int, int, int);
void Mess_LoadFont__FPciii(char *, int, int, int);
void FUN_800502bc(int a);
extern "C" void FUN_800507e4(int a, int b);
typedef struct P {
	P_TAG tag;
	unsigned char r, g, b, code;
	short x0, y0;
	unsigned char u0, v0;
	short clut;
	short x1, y1;
	unsigned char u1, v1;
	short tpage;
	short x2, y2;
	unsigned char u2, v2;
	short pad2;
	short x3, y3;
	unsigned char u3, v3;
	short pad3;
} P;
extern int DAT_800d2274, DAT_800d2278, DAT_800d227c, DAT_800d226c, DAT_800d2270;
extern P *DAT_800d2280;
extern int DAT_800be3d0[];
extern SLight DAT_800be3f8;
extern int DAT_800d1ba8;
extern int SKATER_Y_POS;
void M3dMaths_SetIdentityRotation(MATRIX *m);
extern int DAT_800d2bf0[];
extern short DAT_800d2294[3];
extern int DAT_800d2284, DAT_800d2288, DAT_800d228c, DAT_800d1ba8;
extern CBruce *DAT_800d229c;
extern SViewport *DAT_800d22a0;
extern "C" CBruce *FUN_800506d0(int);
extern int Z_OVER_SY, SKATER_Y_POS;

/* Credits_Roll__Fii (0x8004fe28) */
void Credits_Roll(int a0, int a1) {
	char *s0;
	char *a2;

	char (*out)[30];
	unsigned char *p;
	int c;
	int y, s7, i, s6, s3, s4, s2, th, bot, top, off, tog, s0i;

	if (a1)
		HiRes_BlankScreen();
	else
		Front_ClearScreen();

	Pack_DefragmentFully();

	if (a0 == 0)
		s0i = FileIO_Open("music.txt");

	if (a0 == 1)
		s0i = FileIO_Open("credits.txt");

	s0 = (char *)Mem_NewX(s0i, 1, 1);
	FileIO_Load(s0);

	s7 = 0;
	FileIO_Sync();

	out = (char (*)[30])Mem_NewX(12000, 1, 1);
	p = (unsigned char *)s0;

	while (1) {
		c = *p;

		if ((unsigned char)c == 35)
			break;

		a2 = out[s7];
		i = 0;

		if ((unsigned char)c != 13) {
			for (;;) {
				if (c != 35) {
					a2[i] = c;
					i++;
					p++;

					if (i < 29) {
						c = *p;

						if (c != 13)
							continue;
					}
				}

				break;
			}
		}

		a2[i] = 0;

		if (*p == 13)
			p += 2;

		s7++;

		if (s7 < 400)
			continue;

		break;
	}

	Mem_Delete(s0);
	Mess_LoadFont__FPciii("credits.fnt", -1, -1, -1);
	Mess_SetTextJustify(0);
	Mess_SetScale(255);
	Mess_SetRGB(150, 100, 25, 0);

	s6 = 0;
	s3 = Mess_TextHeightAboveBaseline((char *)out) + 256;
	th = Mess_TextHeight((char *)out) + 8;
	bot = -th;
	top = s3;
	off = a0 * 4;
	s4 = *(int *)((char *)DAT_800d1b98 + off);
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	Db_Init();

	tog = 0;
	FUN_8005027c(a0, 0, a1);

	while (1) {
		s2 = s6;
		Front_UpdateMusicLoop();
		FUN_80049474();
		ResetOT();
		SetClippingBox(0, tog << 8, 512, 240);
		DrawColoredBox(0, 0, 512, 240, 0, 0, 0);

		tog = 1 - tog;

		if (!(s6 < s7))
			break;

		y = s3;

		while (y < top) {
			Mess_DrawText(256, y, out[s2], 1, 0, 256, 0);

			s2++;
			y += th;

			if (!(s2 < s7))
				break;
		}

		s4--;

		if (s4 == 0) {
			s3--;
			s4 = *(int *)((char *)DAT_800d1b98 + off);
		}

		if (s3 < bot) {
			s6++;
			s3 += th;
		}

		PC_FrameWait(1);
		FUN_8005027c(a0, 1, a1);
		PC_FrameWait(1);
		FUN_800494dc();
		Pad_Update();
		PollHost();

		if (Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered ||
		    Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered || Pad[0].Camera.Triggered ||
		    Pad[1].Camera.Triggered) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			SFX_PlayX(SFX_SHELL_SELECT, 8192, 0);
			break;
		}
	}

	FUN_8005027c(a0, 2, a1);
	Mem_Delete((char *)out);
	Mess_UnloadFont();
}

/* FUN_8005027c (0x8005027c) */
void FUN_8005027c(int a, int b, int c) {
	if (a == 0)
		FUN_800502bc(b);
	else if (a == 1)
		FUN_800507e4(b, c);
}

/* FUN_800502bc (0x800502bc) */
/* the inset's POLY_FT4 */
void FUN_800502bc(int a) {
	switch (a) {
		case 0:
			DAT_800d2274 = 0;
			DAT_800d2278 = 0;
			DAT_800d227c = 0;
			DAT_800d2280 = (P *)Mem_NewX(sizeof(P), 1, 1);
			setlen(DAT_800d2280, 9);
			DAT_800d2280->code = 44;
			DAT_800d2280->r = 128;
			DAT_800d2280->g = 128;
			DAT_800d2280->b = 128;
			DAT_800d2280->u0 = 0;
			DAT_800d2280->v0 = 0;
			DAT_800d2280->u1 = 96;
			DAT_800d2280->v1 = 0;
			DAT_800d2280->u2 = 0;
			DAT_800d2280->v2 = 64;
			DAT_800d2280->u3 = 96;
			DAT_800d2280->v3 = 64;
			DAT_800d2280->code &= 253;
			DAT_800d2280->code &= 254;
			DAT_800d2280->tpage = 264;
			break;
		case 1:
			if (DAT_800d2274 == 0) {
				if (DAT_800d2278 == 0) {
					GameFMV_SetStartTrack(DAT_800be3d0[DAT_800d227c]);

					GameFMV_StartX = 512;
					GameFMV_EndX = 608;
					GameFMV_StartY = 0;
					GameFMV_StartFMV(1);

					/* a start position that varies: the frame count */
					DAT_800d226c = (int)(PC_FrameCount() % 320);
					DAT_800d2270 = (int)(PC_FrameCount() % 112);
				}

				DAT_800d2280->r = DAT_800d2278;
				DAT_800d2280->g = DAT_800d2278;
				DAT_800d2280->b = DAT_800d2278;
				DAT_800d2278 += 2;

				if (DAT_800d2278 >= 128) {
					DAT_800d2274 = a;
					DAT_800d2278 = 500;
				}
			} else if (DAT_800d2274 == a) {
				DAT_800d2278--;

				if (DAT_800d2278 == 0) {
					DAT_800d2274 = 2;
					DAT_800d2278 = 128;
				}
			} else if (DAT_800d2274 == 2) {
				DAT_800d2280->r = DAT_800d2278;
				DAT_800d2280->g = DAT_800d2278;
				DAT_800d2280->b = DAT_800d2278;
				DAT_800d2278 -= 2;

				if (DAT_800d2278 <= 0) {
					DAT_800d2274 = 0;
					DAT_800d2278 = 0;
					DAT_800d227c++;

					if (DAT_800d227c >= 10)
						DAT_800d227c = 0;

					GameFMV_StopFMV();
				}
			}

			GameFMV_SyncFMV(1);

			if (GameFMV_RewindSwitch) {
				DAT_800d2274 = 0;
				DAT_800d2278 = 0;
			}

			{
				P *q = DAT_800d2280;
				unsigned short x = DAT_800d226c, y = DAT_800d2270;
				P_TAG *ot = &pDoubleBuffer->OT3D[OT_entry];
				q->x0 = x;
				q->y0 = y;
				q->x1 = x + 192;
				q->y1 = y;
				q->x2 = x;
				q->y2 = y + 128;
				q->x3 = x + 192;
				q->y3 = y + 128;
				addPrim(ot, q);
			}

			break;
		case 2:
			GameFMV_StopFMV();
			Mem_Delete(DAT_800d2280);
			break;
	}
}

/* FUN_800506d0 (0x800506d0) */
/* : makes the preview skater idx (its front-end model, idling, lit by
   DAT_800be3f8). */
/* identity-matrix init, one arg */
extern "C" CBruce *FUN_800506d0(int idx) {
	CBruce *p = new CBruce;

	p->mSkaterIndex = idx;
	p->InitItem(Stats[idx].FrontName);

	p->mAnimSpeed = 0x10000;
	p->CycleAnim(0, 1);

	p->mRGB = 0x404040;
	p->mPos.vx = DAT_800d1ba8;
	p->mPos.vy = SKATER_Y_POS;
	p->mPos.vz = 0x30000;

	M3dMaths_SetIdentityRotation(&p->mRotMatrix);

	if (Stats[idx].pBio[4][0] == 82 || Stats[idx].pBio[4][0] == 114)
		p->mExtraFlags |= 2;

	p->mpLight = &DAT_800be3f8;
	return p;
}

/* FUN_800507e4 (0x800507e4) */
/* : the rotating skater preview. 0 starts spooling the first skater's model, 1 (each
   frame) swaps in the next skater every 500 frames and draws the current one, 2 tears down. */
extern "C" void FUN_800507e4(int a, int b) {
	SViewport *q;

	switch (a) {
		case 0:
			DAT_800d2284 = -1;
			DAT_800d2288 = 0;
			DAT_800d228c = 500;
			DAT_800d2bf0[0] = Spool_PSX(Stats[0].FrontName, 2, 1);
			Spool_Sync();

			DAT_800d2294[2] = 0;
			DAT_800d2294[1] = 0;
			DAT_800d2294[0] = 0;
			q = HiRes_SetupCamera();
			DAT_800d22a0 = q;
			q->xR = 512;
			q->yB = 240;
			DAT_800d229c = 0;
			break;
		case 1:
			FileIO_Continue();
			Spool_Continue();

			if (DAT_800d228c == 500 && FUN_80070830() == 0) {
				if (DAT_800d2284 >= 0) {
					if (DAT_800d229c)
						delete DAT_800d229c;

					ClearRegion(DAT_800d2bf0[DAT_800d2284], 1, 1);
					Spool_RestorePSX(DAT_800d2bf0[DAT_800d2284]);
					ProcessNewPSX(DAT_800d2bf0[DAT_800d2284]);
					Spool_ClearPSX(Stats[DAT_800d2284].FrontName);
				}

				Spool_BackupPSX(DAT_800d2bf0[DAT_800d2288]);
				ProcessNewPSX(DAT_800d2bf0[DAT_800d2288]);

				DAT_800d229c = FUN_800506d0(DAT_800d2288);
				DAT_800d229c->mPos.vx = DAT_800d1ba8;
				GHeadPart = -1;
				DAT_800d2284 = DAT_800d2288;

				if (++DAT_800d2288 >= 10)
					DAT_800d2288 = 0;

				DAT_800d2bf0[DAT_800d2288] = Spool_PSX(Stats[DAT_800d2288].FrontName, 2, 1);
			}

			DAT_800d228c--;

			if (DAT_800d228c <= 0)
				DAT_800d228c = 500;

			RotMatrixYXZ((SVECTOR *)DAT_800d2294, &DAT_800d229c->mRotMatrix);

			{
				CBruce *q2 = DAT_800d229c;
				unsigned short sa = q2->mRotMatrix.m[0][1], sb = q2->mRotMatrix.m[1][1],
				               sc = q2->mRotMatrix.m[2][1];

				q2->mRotMatrix.m[0][1] = -sa;
				q2->mRotMatrix.m[1][1] = -sb;
				q2->mRotMatrix.m[2][1] = -sc;
				M3d_BuildTransform(q2);
			}

			DAT_800d229c->UpdateFrame();

			if (b)
				DAT_800d229c->UpdateFrame();

			M3d_RenderSetup(MikeCamera, DAT_800d22a0, pDoubleBuffer->OT3D);
			M3d_Render(MechList);
			M3d_RenderCleanup();

			DAT_800d2294[1] += 10;
			break;
		case 2:
			if (DAT_800d229c)
				delete DAT_800d229c;

			ClearRegion(DAT_800d2bf0[DAT_800d2284], 1, 1);
			Spool_RestorePSX(DAT_800d2bf0[DAT_800d2284]);
			ProcessNewPSX(DAT_800d2bf0[DAT_800d2284]);
			Spool_ClearPSX(Stats[DAT_800d2284].FrontName);
			Spool_Sync();
			ProcessNewPSX(DAT_800d2bf0[DAT_800d2288]);
			Spool_ClearPSX(Stats[DAT_800d2288].FrontName);
			break;
	}
}

/* A C name for a declaration (as game/data's PSX_NAME): `__asm`, not `__asm__`, which portsrc's
   inline-asm translation would take for MIPS code. */
#define CREDITS_STR_(x) #x
#define CREDITS_STR(x) CREDITS_STR_(x)
#define CREDITS_PSX_NAME(name) __asm(CREDITS_STR(__USER_LABEL_PREFIX__) name)
/* the static constructor of SCROLL_COUNT: named as retail's (dots and all), as game/data's
   GameCtorList names it (PSX_NAME there), so the build places it by that name */
extern "C" void GLOBAL_I_SCROLL_COUNT() CREDITS_PSX_NAME("_GLOBAL_.I.SCROLL_COUNT");

/* _GLOBAL_.I.SCROLL_COUNT (0x80050c34) */
void GLOBAL_I_SCROLL_COUNT(void) {
	SKATER_Y_POS = Z_OVER_SY * 330;
}

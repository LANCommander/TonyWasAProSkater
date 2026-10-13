/* Display__Fv (0x80024fa4) */
#include "names.h"
#include "game/padbuttons.h"
#ifdef PC_PORT
#include <stdio.h>
#include <stdlib.h>
#endif
typedef struct {
	char pad[0x80a8];
	unsigned f80a8;
} B;

extern char Pad[];

typedef struct {
	char pad[14];
	short f0e;
} Z;

extern Z DAT_800a6790;
extern char DAT_800a67a4[];
extern char DAT_800a67b8[];
extern char DAT_800a67cc[];
extern char DAT_800a67e0[];
extern char DAT_800a67f4[];
extern char DAT_800a6808[];
extern char DAT_800a681c[];
extern char DAT_800a6830[];
extern int Status;
extern int GamePaused;
extern B *pDoubleBuffer;
extern unsigned pPoly;
extern int CameraList;
extern int camDist;
extern int camPitch;
extern int camYOffset;

int GGame, DAT_800d16dc, GameFrozen, Cheat_ScreenShots, PolyBufferBytesUsed, Mick_Darkness,
    ViewportMode, DAT_800d1748, GNumberOfPlayers, CloseCam;

void BlackOut(int, int, int, int);
void Front_Display(void);
void Mess_SetTextJustify(int);
void Mess_DrawText(int, int, int, int, int, int, int);
void Flash_Display(void);
void Mick_Darken(void);
void Display_Viewport(char *, int);
void Ob_AI(int *, int);
void Font_Display(void);
void FUN_8006dc78(void);
short FUN_8007b0fc(int);
void Mess_Display(void);
void Mess_SetRGB(int, int, int, int);
void Mess_SetScale(int);

void Display(void) {
	int f, a, b, c;

	if (DAT_800d16dc) {
		Mess_SetRGB(255, 0, 0, 0);
		Mess_SetScale(256);
		Mess_SetTextJustify(0);
		Mess_DrawText(256, 60, DAT_800d16dc, 0, 0, 256, 0);
	}

	DAT_800a6790.f0e = FUN_8007b0fc(CameraList);

	if (!(GameFrozen && Cheat_ScreenShots == 0)) {
		Ob_AI(&CameraList, GameFrozen);

		if (Cheat_ScreenShots)
			goto L90;
	}

	if (Pad[PAD_CAMERA + PAD_PRESSED]) {
		if (DAT_800d1748) {
			DAT_800d1748 = 0;

			if (GNumberOfPlayers == 2 && GGame != 7) {
				if (++ViewportMode >= 4)
					ViewportMode = 0;
			} else
				CloseCam ^= 1;
		}
	} else
		DAT_800d1748 = 1;

	if (Pad[PAD_SIZE + PAD_CAMERA + PAD_TRIGGERED]) {
		Pad[PAD_SIZE + PAD_CAMERA + PAD_TRIGGERED] = 0;

		if (++ViewportMode >= 4)
			ViewportMode = 0;
	}
L90:
	if (GNumberOfPlayers == 2 && ViewportMode == 0 && GGame != 7)
		ViewportMode = 1;

	switch (ViewportMode) {
		case 0:
			f = GameFrozen;

			if (f == 0) {
				if (CloseCam) {
					c = 386;
					a = 30;
					camDist = c;
					b = -160;
				} else {
					c = 290;
					a = 150;
					camDist = c;
					b = -108;
				}

				camPitch = a;
				camYOffset = b;
			}

			Display_Viewport((char *)&DAT_800a6790, 0);
			break;
		case 1:
			f = GameFrozen;

			if (f == 0) {
				camDist = 286;
				camPitch = 118;
				camYOffset = -140;
			}

			BlackOut(254, 0, 4, 240);
			Display_Viewport(DAT_800a67a4, 0);
			Display_Viewport(DAT_800a67b8, 1);
			break;
		case 2:
			f = GameFrozen;

			if (f == 0) {
				camDist = 450;
				camPitch = 60;
				camYOffset = -120;
			}

			BlackOut(0, 118, 512, 4);
			Display_Viewport(DAT_800a67cc, 0);
			Display_Viewport(DAT_800a67e0, 1);
			break;
		case 3:
			f = GameFrozen;

			if (f == 0) {
				camDist = 642;
				camPitch = 32;
				camYOffset = -172;
			}

			BlackOut(256, 0, 256, 120);
			BlackOut(0, 120, 256, 120);
			Display_Viewport(DAT_800a67f4, 0);
			Display_Viewport(DAT_800a6808, 1);
			break;
		case 4:
			f = GameFrozen;

			if (f == 0) {
				camDist = 610;
				camPitch = 0;
				camYOffset = -200;
			}

			BlackOut(304, 0, 208, 88);
			BlackOut(0, 152, 208, 88);
			Display_Viewport(DAT_800a681c, 0);
			Display_Viewport(DAT_800a6830, 1);
			break;
	}

	Font_Display();
	FUN_8006dc78();
	Front_Display();
	Mess_Display();

	if (GamePaused && Status != 33)
		Mick_Darken();
	else
		Mick_Darkness = 0;

	Flash_Display();

	PolyBufferBytesUsed = (pPoly & 0x7fffffff) - (pDoubleBuffer->f80a8 & 0x7fffffff);

#ifdef PC_PORT
	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		extern unsigned long PolyBufferEnd;
		fprintf(stderr, "polybuf f%u used=%d cap=%ld remaining=%ld\n", pc_heartbeat,
		        PolyBufferBytesUsed,
		        (long)(PolyBufferEnd & 0x7fffffff) - (long)(pDoubleBuffer->f80a8 & 0x7fffffff),
		        (long)(PolyBufferEnd & 0x7fffffff) - (long)(pPoly & 0x7fffffff));
	}
#endif
}

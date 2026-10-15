/* FLASH.CPP: full-screen colour flashes and fades (Flash_Screen, Flash_Fade) (declarations in
   game/flash.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/flash.h"
#include "game/m3d.h"
#include "game/bit.h"
#include "game/bruce.h"
#include "game/globals.h"

extern int FlashCountdown;
extern int FadeCountdown;
extern int Fading;
extern unsigned char CurrentImportance;
extern int FlashSort;
/* the flash colour, 16.16 fixed point: the integer parts are drawn */
extern int CurrentR, CurrentG, CurrentB;
/* the per-frame colour step */
extern int dR, dG, dB;

/* Flash_Reset__Fv (0x8001f310) */
void Flash_Reset(void) {
	FlashCountdown = 0;
	FadeCountdown = 0;
	Fading = 0;
	CurrentImportance = 0;
}

/* Flash_FadeFinished__Fv (0x8001f328) */
int Flash_FadeFinished(void) {
	return FadeCountdown == 0;
}

/* Flash_Screen__FUcUcUciUci (0x8001f334) */
void Flash_Screen(unsigned char r, unsigned char g, unsigned char b, int n, unsigned char c,
                  int d) {
	if (c >= CurrentImportance && n != 0) {
		unsigned R = r << 16, G = g << 16, B = b << 16;
		unsigned nn = n;

		CurrentR = R;
		CurrentG = G;
		CurrentB = B;
		FlashCountdown = n;
		CurrentImportance = c;
		FlashSort = d;
		dR = R / nn;
		dG = G / nn;
		dB = B / nn;
	}
}

/* Flash_Fade__Fii (0x8001f3b8) */
void Flash_Fade(int a, int b) {
	Fading = 1;
	FadeCountdown = a;
	CurrentImportance = 255;
	CurrentB = 0;
	CurrentG = 0;
	CurrentR = 0;
	FlashSort = b;
	dR = 0xff0000 / a;
}

/* Flash_Update__Fv (0x8001f3f4) */
void Flash_Update(void) {
	if (Fading) {
		if (FadeCountdown) {
			FadeCountdown--;

			if (FadeCountdown == 0) {
				CurrentR = CurrentG = CurrentB = 0xff0000;
			} else {
				CurrentR += dR;
				CurrentG += dR;
				CurrentB += dR;
			}
		}
	} else if (FlashCountdown) {
		FlashCountdown--;

		if (FlashCountdown == 0) {
			CurrentImportance = 0;
		} else {
			CurrentR -= dR;
			CurrentG -= dG;
			CurrentB -= dB;
		}
	}
}

/* Flash_Display__Fv (0x8001f4c4) */
/* Demo twin; locals from its MAIN.SYM. While a screen flash is active, adds a
   full-screen semi-transparent flat quad in the flash colour (additive, or subtractive for the
   second mode) to the current ordering table. */
void Flash_Display() {
	const int fading = Fading; /* read once (retail reloads nothing after the stores below) */

	if (FlashCountdown || fading) {
		P_TAG *OTPos = &pDoubleBuffer->OT2D + FlashSort;
		POLY_F4 *aPoly = (POLY_F4 *)pPoly;
		uchar *newppoly = pPoly + sizeof(POLY_F4) + sizeof(DR_TPAGE);
		DR_TPAGE *ptpage = (DR_TPAGE *)(aPoly + 1);

		if (newppoly <= PolyBufferEnd) {
			pPoly = newppoly;
			setPolyF4(aPoly);
			setSemiTrans(aPoly, 1);
			setRGB0(aPoly, (ushort)(CurrentR >> 16), (ushort)(CurrentG >> 16), (ushort)(CurrentB >> 16));

			aPoly->x0 = 0;
			aPoly->x1 = 512;
			aPoly->y0 = 0;
			aPoly->y1 = 0;
			aPoly->x2 = 0;
			aPoly->x3 = 512;
			aPoly->y2 = 240;
			aPoly->y3 = 240;
			addPrim(OTPos, aPoly);

			setlen(ptpage, 1);

			if (fading)
				ptpage->code[0] = 0xe1000640;
			else
				ptpage->code[0] = 0xe1000620;

			addPrim(OTPos, ptpage);
		}
	}
}

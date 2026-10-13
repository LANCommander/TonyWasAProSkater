/* Flash_Display__Fv (0x8001f4c4) flags=-fstructure-noalias */
/* C++ (FLASH.CPP). Demo twin; locals from its MAIN.SYM. While a screen flash is active, adds a
   full-screen semi-transparent flat quad in the flash colour (additive, or subtractive for the
   second mode) to the current ordering table. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

int FlashCountdown;

extern const int Fading;

int FlashSort;

extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;
extern uchar *PolyBufferEnd;

volatile ushort CurrentR_2;
volatile ushort CurrentG_2;
volatile ushort CurrentB_2;

void Flash_Display() {
	if (FlashCountdown || Fading) {
		u_long *OTPos = &pDoubleBuffer->OT2D + FlashSort;
		POLY_F4 *aPoly = (POLY_F4 *)pPoly;
		uchar *newppoly = pPoly + sizeof(POLY_F4) + sizeof(DR_TPAGE);
		DR_TPAGE *ptpage = (DR_TPAGE *)(aPoly + 1);

		if (newppoly <= PolyBufferEnd) {
			pPoly = newppoly;
			setPolyF4(aPoly);
			setSemiTrans(aPoly, 1);
			setRGB0(aPoly, CurrentR_2, CurrentG_2, CurrentB_2);

			aPoly->x0 = 0;
			aPoly->x1 = 512;
			aPoly->y0 = 0;
			aPoly->y1 = 0;
			aPoly->x2 = 0;
			aPoly->x3 = 512;
			aPoly->y2 = 240;
			aPoly->y3 = 240;
			addPrim(OTPos, aPoly);

			((u_long *)ptpage)[0] = 0x01000000;

			if (Fading)
				((u_long *)ptpage)[1] = 0xe1000640;
			else
				((u_long *)ptpage)[1] = 0xe1000620;

			addPrim(OTPos, ptpage);
		}
	}
}

extern const int Fading = 0;

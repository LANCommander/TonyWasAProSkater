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

extern int FlashCountdown;

extern const int Fading;

extern int FlashSort;

extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;
extern uchar *PolyBufferEnd;

/* the flash colour, 16.16 fixed point: the integer parts are drawn */
extern int CurrentR, CurrentG, CurrentB;

void Flash_Display() {
	if (FlashCountdown || Fading) {
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

			if (Fading)
				ptpage->code[0] = 0xe1000640;
			else
				ptpage->code[0] = 0xe1000620;

			addPrim(OTPos, ptpage);
		}
	}
}

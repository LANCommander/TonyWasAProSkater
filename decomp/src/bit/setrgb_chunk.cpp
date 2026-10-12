/* SetRGB__9CChunkBitUcUcUc (0x800289c8) */
/* C++ (BIT.CPP). Sets the chunk's colour: corner 0 gets it exactly, the other three a random
   shade of it. */
#include "game/bit.h"

int Redbook_Rnd(int);

void CChunkBit::SetRGB(uchar r, uchar g, uchar b) {
	int k;

	mB0G0R0 = (b << 16) | (g << 8) | r;
	k = Redbook_Rnd(4096);
	mB1G1R1 = (((b * k) >> 12) << 16) | (((g * k) >> 12) << 8) | ((r * k) >> 12);
	k = Redbook_Rnd(4096);
	mB2G2R2 = (((b * k) >> 12) << 16) | (((g * k) >> 12) << 8) | ((r * k) >> 12);
	k = Redbook_Rnd(4096);
	mB3G3R3 = (((b * k) >> 12) << 16) | (((g * k) >> 12) << 8) | ((r * k) >> 12);
}

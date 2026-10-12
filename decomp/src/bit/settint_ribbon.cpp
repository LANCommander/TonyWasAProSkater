/* SetTint__7CRibbonUcUcUc (0x8002a0e8) */
/* C++ (BIT.CPP). Tints every segment of the ribbon. */
#include "game/bit.h"

void CRibbon::SetTint(uchar r, uchar g, uchar b) {
	for (int i = 0; i < mNumBits; i++)
		mBits[i]->SetTint(r, g, b);
}

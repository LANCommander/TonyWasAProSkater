/* SetScale__7CRibboni (0x8002a178) */
/* C++ (BIT.CPP). Sets the scale of every segment of the ribbon. */
#include "game/bit.h"

void CRibbon::SetScale(int scale) {
	for (int i = 0; i < mNumBits; i++)
		mBits[i]->SetScale(scale);
}

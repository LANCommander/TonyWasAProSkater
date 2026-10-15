/* TrickOb_Update__Fv (0x8004f374) */
#include "game/m3d.h"
#include "game/trickob.h"

extern CTrickOb *TrickObList;

extern const int GGame;
extern uint Xblanks;

void TrickOb_Update(void) {
	CTrickOb *pTrickOb;

	for (pTrickOb = TrickObList; pTrickOb; pTrickOb = pTrickOb->mpNext) {
		if (pTrickOb->mState) {
			Item *pItem = pTrickOb->mpItem;

			if (pItem) {
				if (GGame == 8) {
					int Red;
					uint t = Xblanks << 2;

					if (pTrickOb->mScore <= 2000)
						Red = t % ((((pTrickOb->mScore << 6) / 2000) + 1) * 2);
					else
						Red = t % 130;

					if (pTrickOb->mScore <= 2000) {
						if ((((pTrickOb->mScore << 6) / 2000) + 1) < Red)
							goto flip;
					} else {
						if (Red >= 66) {
						flip:
							if (pTrickOb->mScore <= 2000)
								Red = ((((pTrickOb->mScore << 6) / 2000) + 1) * 2) - Red;
							else
								Red = 130 - Red;
						}
					}

					if (pTrickOb->mPlayer == 1)
						pItem->RGB = (pItem->RGB & 0xff00ffff) | ((Red + 64) << 16);
					else
						pItem->RGB = (pItem->RGB & 0xffffff00) | (Red + 64);
				} else {
					int Red = (Xblanks >> 1) & 63;

					if (Red > 32)
						Red = 64 - Red;

					pItem->RGB = (pItem->RGB & 0xffffff00) | (Red + 32);
				}
			}
		}
	}
}

/* TrickOb_DarkenNode__Fi (0x8004f24c) */
#include "game/m3d.h"
#include "game/trickob.h"

extern int GGame;

extern CTrickOb *TrickObList;
extern uchar TrickPlayer;
extern ushort TrickScore;

Item *Spool_FindEnviroItem(uint);

void TrickOb_DarkenNode(int id) {
	CTrickOb *n;

	if (GGame == 8) {
		for (n = TrickObList; n; n = n->mpNext) {
			if (n->mNode == id) {
				Item *it;

				if (n->mState && GGame != 8)
					return;

				it = Spool_FindEnviroItem(n->mChecksum);
				n->mScore = TrickScore;
				n->mpItem = it;
				n->mPlayer = TrickPlayer;
				it->RGB = 0x202020;
				it->Flags |= 0x400;
				n->mState = 1;
				return;
			}
		}
	}
}

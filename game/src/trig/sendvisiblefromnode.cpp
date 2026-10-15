/* SendVisibleFromNode__Fii (0x80045838) */
#include "game/m3d.h"
#include "game/ob.h"
#include "game/bit.h"

class CBruce;

extern CBruce *pCurrentPlayer;
extern int GNumberOfPlayers;
extern CBruce *MechList[];
extern CItem *PowerUpList;
extern CBit *NonRenderedBitList;

extern short **OffsetList;

ushort *Trig_GetLinksPointer(int);
Item *Spool_FindEnviroItem(uint);

void SendVisibleFromNode(int node, int visible) {
	int SetFlags;
	int ClearFlags;
	ushort *pNodes;
	ushort NumLinks;

	if (!pCurrentPlayer || GNumberOfPlayers == 1)
		SetFlags = 65;
	else if (pCurrentPlayer == MechList[0])
		SetFlags = 64;
	else
		SetFlags = 1;

	if (visible) {
		ClearFlags = ~SetFlags;
		SetFlags = 0;
	} else {
		ClearFlags = -1;
	}

	pNodes = Trig_GetLinksPointer(node);
	NumLinks = *pNodes++;

	{
		int i;

		for (i = 0; i < NumLinks; i++) {
			ushort VisNode = *pNodes++;
			short *pVisNode = OffsetList[VisNode];

			switch (*pVisNode) {
				case 5: {
					CItem *pUp;

					for (pUp = PowerUpList; pUp; pUp = pUp->mNextItem) {
						if (((CBody *)pUp)->mNode == VisNode) {
							pUp->mFlags = (pUp->mFlags & ClearFlags) | SetFlags;
							break;
						}
					}

					break;
				}
				case 501: {
					CBit *pBit;

					for (pBit = NonRenderedBitList; pBit; pBit = pBit->mNext) {
						if (pBit->mType == 2 && pBit->mLifetime == VisNode) {
							if (visible)
								((CLight *)pBit)->mOn = 1;
							else if (GNumberOfPlayers == 1)
								((CLight *)pBit)->mOn = 0;

							break;
						}
					}

					break;
				}
				case 9:
				case 14:
				case 2:
				case 12: {
					Item *pItem;

					pVisNode++;
					pVisNode += *pVisNode + 1;

					if ((intptr_t)pVisNode & 2)
						pVisNode++;

					pItem = Spool_FindEnviroItem(*(uint *)pVisNode);

					if (pItem)
						pItem->Flags = (pItem->Flags & ClearFlags) | SetFlags;

					break;
				}
			}
		}
	}
}

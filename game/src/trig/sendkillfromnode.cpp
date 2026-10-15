/* SendKillFromNode__Fii (0x80045a90) */
/* C++ (TRIG.CPP). Fuzzy demo twin (0.73); locals from its MAIN.SYM. Sends a kill to every node
   linked from Node: platforms bounce the skater and pulse their own links, environment items are
   hit (How 1) or flagged, bits are killed. Anything else goes to the body lists by node id. */
#include "game/m3d.h"
#include "game/bruce.h"
#include "game/platform.h"
#include "game/bit.h"
#include "game/trig.h"

short **OffsetList;

extern CBaddy *EnvironmentalObjectList;
extern CBaddy *BaddyList;
extern CBruce *pCurrentPlayer;
extern CBit *NonRenderedBitList;

ushort *Trig_GetLinksPointer(int);
void Trig_SendPulse(ushort *);
Item *Spool_FindEnviroItem(uint);
void Exp_HitEnvItem(Item *, uint *, int);

void SendKillFromNode(int Node, int How) {
	ushort *pNodes;
	ushort NumLinks;
	int Bounced;
	int i;

	pNodes = Trig_GetLinksPointer(Node);
	NumLinks = *pNodes++;

	for (i = 0; i < NumLinks; i++) {
		ushort KillNode = *pNodes++;
		short *pKillNode = OffsetList[KillNode];
		CBaddy *pList;

		Bounced = 0;

		switch (*pKillNode) {
			case 1:
				for (pList = EnvironmentalObjectList; pList; pList = (CBaddy *)pList->mNextItem) {
					if (pList->mNode == KillNode && (pList->mFlags & 0x100) &&
					    (pList->mSpecificFlags & 0x10)) {
						pList->mFlags |= 0x20;

						if (!(pList->mSpecificFlags & 0x20)) {
							Trig_SendPulse(Trig_GetLinksPointer(pList->mNode));

							pList->mDeathPulseSent = 1;
							((CPlatform *)pList)->Bounce(pCurrentPlayer, pCurrentPlayer->mVel);
							Bounced = 1;
						}
					}
				}

				if (!Bounced) {
					KillInList(KillNode, BaddyList, How);
					KillInList(KillNode, EnvironmentalObjectList, How);
				}

				break;
			case 2:
			case 9: {
				Item *pItem;

				pKillNode++;
				pKillNode += *pKillNode + 1;

				if ((intptr_t)pKillNode & 2)
					pKillNode++;

				pItem = Spool_FindEnviroItem(*(uint *)pKillNode);

				if (!pItem)
					break;

				if (How == 1)
					Exp_HitEnvItem(pItem, 0, 0xffff);
				else
					pItem->Flags |= 0x41;

				break;
			}
			case 10:
			case 11:
			case 12:
				break;
			/* 13 and 14 are separate compare-tree nodes in retail, so their bodies weren't plain
		   breaks; whatever they did folded away */
			case 13:
				Bounced = 0;
				break;
			case 14:
				Bounced = 0;
				break;
			case 500:
			case 501: {
				CBit *pBit;

				for (pBit = NonRenderedBitList; pBit; pBit = pBit->mNext) {
					if (pBit->mType == 2 && pBit->mLifetime == KillNode) {
						pBit->Die();
						break;
					}
				}

				break;
			}
		}
	}
}

/* TrickObRail__6CBruce (0x80030434) */
/* THPS1 fills in the demo's empty TrickObRail: when the rail start node (type 11) links to a trick
   object node (type 14 or 12), look up its enviro item by checksum and remember it with a heap ref.
   Lever: the second link test uses its own pointer local (reusing pLink changes the branch layout). */
#include "game/m3d.h"
#include "game/bruce.h"

extern SPSXRegion PSXRegion[];
extern short **OffsetList;
extern int GGame;

ushort *Trig_GetLinksPointer(int);
Item *Spool_FindEnviroItem(uint);

extern "C" {
	void FUN_8003036c(CBruce *);
	SMemHandle FUN_8004d7ac(void *);
	void FUN_8004f000(Item *, CBruce *, int);
}

void CBruce::TrickObRail() {
	short *pNode;

	if (*OffsetList[mRailStart] == 11) {
		ushort *pLinks = Trig_GetLinksPointer(mRailStart);
		short *pLink = OffsetList[pLinks[1]];

		if (*pLink == 14 || *pLink == 12)
			pNode = pLink;
		else {
			short *pLink2 = OffsetList[pLinks[2]];

			if (*pLink2 == 14 || *pLink2 == 12)
				pNode = pLink2;
		}

		pNode += (ushort)pNode[1] + 2;

		if ((int)pNode & 3)
			pNode++;

		uint Checksum = *(uint *)pNode;

		FUN_8003036c(this);

		mpWallItem = Spool_FindEnviroItem(Checksum);
		mWallHandle = FUN_8004d7ac(PSXRegion[mpWallItem->Region].pPSX);

		if (GGame == 1)
			FUN_8004f000(mpWallItem, this, -1);
	}
}

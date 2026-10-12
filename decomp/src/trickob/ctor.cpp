/* __8CTrickObUs (0x8004ef08) flags=-fstructure-noalias */
#include "game/trickob.h"

CTrickOb *TrickObList;

extern unsigned short **OffsetList;

CTrickOb::CTrickOb(unsigned short Node) {
	mpNext = TrickObList;
	TrickObList = this;
	mpItem = 0;
	mNode = Node;
	mState = 0;

	{
		unsigned short *pNode = OffsetList[Node];

		pNode += pNode[1] + 2;

		if ((int)pNode & 3)
			pNode++;

		mChecksum = *(unsigned int *)pNode;
	}
}

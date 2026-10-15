/* __16CScriptOnlyBaddyPsi (0x800411ac) */
/* C++ (BADDY.CPP). A script-only baddy: placed from its object data at node `node`, owned by the
   current player, runs its script and joins the baddy list. */
#include "game/baddy.h"

extern CBruce *pCurrentPlayer;

extern CBody *BaddyList;

CScriptOnlyBaddy::CScriptOnlyBaddy(short *p_info, int node) {
	mpCreatedByPlayer = pCurrentPlayer;
	p_info = SquirtPos(p_info);
	mpScript = (ushort *)SquirtAngles(p_info);
	mLoopingSoundUpdate = -1;
	mDoScript = 1;
	mNode = node;
	mFlags |= 0x41;
	mCBodyFlags &= ~0x10;
	AttachTo(&BaddyList);

	mRMinor = 0;
	mType = 203;
}

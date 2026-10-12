/* __9CPlatformPsi (0x8003d6d8) */
/* C++ (PLATFORM.CPP). A platform: a baddy using the current object file's models, on the
   environmental-object list, placed from its object data at node `node`, running its script. */
#include "game/platform.h"

extern CBody *EnvironmentalObjectList;
extern char *pCurrentObjFile;
extern CBruce *pCurrentPlayer;

CPlatform::CPlatform(short *p_info, int node) {
	mpCreatedByPlayer = pCurrentPlayer;
	InitItem(pCurrentObjFile);
	AttachTo(&EnvironmentalObjectList);

	mType = 402;
	mFlags |= 0x111;
	mFlags &= ~2;
	p_info = SquirtPos(p_info);
	mpScript = (ushort *)SquirtAngles(p_info);
	mNode = node;
	mLoopingSoundUpdate = -1;
	mBaddyAttributes[0] = 32;
	mDoScript = 1;
}

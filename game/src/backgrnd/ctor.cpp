/* __11CBackgroundUiRC8CSVector (0x80078188) flags=-fstructure-noalias */
/* C++ (BACKGRND.CPP). Demo twin. A background object: a spinning CSuper with its model looked up by
   checksum, attached to the background list with a running id. */
#include "game/backgrnd.h"

extern char *pCurrentObjFile __attribute__((section(".data")));

CBody *BackgroundList;
ushort CurrentIndex;

int Spool_GetModel(uint, int);

CBackground::CBackground(uint checksum, const CSVector &ang_vel) {
	InitItem(pCurrentObjFile);

	mModel = Spool_GetModel(checksum, mRegion);
	mFlags &= ~2;
	mCBodyFlags &= ~6;
	AttachTo(&BackgroundList);

	mIndex = CurrentIndex++;
}

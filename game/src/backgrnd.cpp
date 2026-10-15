/* BACKGRND.CPP: CBackground, a spinning background model (a CBody), and switching
   backgrounds on and off. Declarations in game/backgrnd.h. */
#include "game/backgrnd.h"
#include "game/spool.h"

extern ushort CurrentIndex;

/* Backgrnd_On__FUs (0x800780f0) */
void Backgrnd_On(ushort id) {
	CBody *p;

	for (p = BackgroundList; p; p = (CBody *)p->mNextItem) {
		if (((CBackground *)p)->mIndex == id) {
			p->mFlags &= ~1;
			return;
		}
	}
}

/* Backgrnd_Off__FUs (0x8007813c) */
/* C++. Hides the background with the given index (clears its mFlags bit 0). */
void Backgrnd_Off(unsigned short id) {
	CBody *p;

	for (p = BackgroundList; p; p = (CBody *)p->mNextItem) {
		if (((CBackground *)p)->mIndex == id) {
			p->mFlags |= 1;
			return;
		}
	}
}

/* __11CBackgroundUiRC8CSVector (0x80078188) */
/* Demo twin. A background object: a spinning CSuper with its model looked up by
   checksum, attached to the background list with a running id. */
CBackground::CBackground(uint checksum, const CSVector &ang_vel) {
	InitItem(pCurrentObjFile);

	mModel = Spool_GetModel(checksum, mRegion);
	mFlags &= ~2;
	mCBodyFlags &= ~6;
	AttachTo(&BackgroundList);

	mIndex = CurrentIndex++;
}

/* _._11CBackground (0x8007821c) */
/* Leaves the background list and gives back its index; CBody's destructor
   does the rest. */
CBackground::~CBackground() {
	DeleteFrom(&BackgroundList);

	CurrentIndex--;
}

/* AI__11CBackground (0x8007828c) */
/* A background object just spins at its angular velocity. */
void CBackground::AI() {
	mAngles += mAngVel;
}

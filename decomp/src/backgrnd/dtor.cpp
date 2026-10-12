/* _._11CBackground (0x8007821c) */
/* C++ (BACKGRND.CPP). Leaves the background list and gives back its index; CBody's destructor
   does the rest. */
#include "game/backgrnd.h"

CBody *BackgroundList;
ushort CurrentIndex;

CBackground::~CBackground() {
	DeleteFrom(&BackgroundList);

	CurrentIndex--;
}

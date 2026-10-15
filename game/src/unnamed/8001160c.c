/* Front_InitMemCardMenus__Fi13EMemCardState13EPausedStatus (0x8001160c) */
#include "names.h"

extern int GReturnToState;
extern int FreeSprite;
extern int MCResolution;
extern int MCState;
extern int CheckingCardPause;

void Front_InitMemCardMenus(int a, int b, int c) {
	GReturnToState = c;
	FreeSprite = 0;
	MCResolution = a;
	MCState = b;
	CheckingCardPause = 0;
}

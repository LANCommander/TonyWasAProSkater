/* Front_InitMemCardMenus__Fi13EMemCardState13EPausedStatus (0x8001160c) */
#include "names.h"

int GReturnToState;
int FreeSprite;
int MCResolution;
int MCState;
int CheckingCardPause;

void Front_InitMemCardMenus(int a, int b, int c) {
	GReturnToState = c;
	FreeSprite = 0;
	MCResolution = a;
	MCState = b;
	CheckingCardPause = 0;
}

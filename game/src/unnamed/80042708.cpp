/* FUN_80042708 (0x80042708) */
/* C++: a skater's analogue stick from its pad. */
#include "game/bruce.h"
#include "game/pad.h"

extern "C" void FUN_80042708(CBruce *p) {
	SControl *q = p->mpPad;

	p->mAY = q->AnalogueMoveForwardsBackwards;
	p->mAX = q->AnalogueMoveLeftRight;
}

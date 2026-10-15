/* FUN_8002aa0c (0x8002aa0c) */
/* C++: moves every live bit of a list (CBit::Move, vtable slot 1). */
#include "game/bit.h"

extern "C" void FUN_8002aa0c(CBit *p) {
	for (; p; p = p->mNext) {
		if (p->mDead == 0)
			p->Move();
	}
}

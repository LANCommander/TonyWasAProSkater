/* Debug_RunAnim__6CBruceii (0x8002fcdc) */
/* C++ (PHYSICS.CPP). RunAnim(Anim) after any pending flip; Line is the caller's __LINE__. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

void CBruce::Debug_RunAnim(int Anim, int) {
	MaybeFlipAnimationTransition();
	RunAnim(Anim, 0, -1, -1);
}

/* Debug_RunAnim1__6CBruceiii (0x8002fd28) */
/* C++ (PHYSICS.CPP). RunAnim(Anim, a) after any pending flip; Line is the caller's __LINE__. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void CBruce::Debug_RunAnim1(int Anim, int a, int) {
	MaybeFlipAnimationTransition();
	RunAnim(Anim, a, -1, -1);
}

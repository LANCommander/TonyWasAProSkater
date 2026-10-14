/* Debug_RunAnim2__6CBruceiiii (0x8002fd80) */
/* C++ (PHYSICS.CPP). RunAnim(Anim, a, b) after any pending flip; Line is the caller's __LINE__. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void CBruce::Debug_RunAnim2(int Anim, int a, int b, int) {
	MaybeFlipAnimationTransition();
	RunAnim(Anim, a, b, -1);
}

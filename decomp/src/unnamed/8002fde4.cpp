/* Debug_RunAnim3__6CBruceiiiii (0x8002fde4) */
/* C++ (PHYSICS.CPP). RunAnim(Anim, a, b, c) after any pending flip; Line is the caller's __LINE__. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void CBruce::Debug_RunAnim3(int Anim, int a, int b, int c, int) {
	MaybeFlipAnimationTransition();
	RunAnim(Anim, a, b, c);
}

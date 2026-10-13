/* Trick_HoldTrick__6CBrucei (0x8002f438) */
/* C++ (PHYSICS.CPP). Forwards to Panel_HoldTrick, which adds Score to the last queued trick in the
   skater's panel and refreshes the trick message. */
#include "game/bruce.h"
#include "game/panel.h"

void CBruce::Trick_HoldTrick(int Score) {
	Panel_HoldTrick(this, Score);
}

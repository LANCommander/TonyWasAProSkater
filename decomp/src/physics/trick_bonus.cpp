/* Trick_Bonus__6CBrucei (0x8002f458) */
/* C++ (PHYSICS.CPP). Forwards to Panel_Bonus, which is empty in retail. The arguments pass through
   in a0/a1 untouched, so the call is written like Trick_HoldTrick's. */
#include "game/bruce.h"
#include "game/panel.h"

void CBruce::Trick_Bonus(int Score) {
	Panel_Bonus(this, Score);
}

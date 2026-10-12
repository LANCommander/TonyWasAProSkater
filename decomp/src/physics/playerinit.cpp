/* Trick_PlayerInit__6CBruceUs (0x8002f178) */
/* C++ (PHYSICS.CPP). Initialises the panel (score display) for this player. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void Panel_PlayerInit(ushort, CBruce *);

void CBruce::Trick_PlayerInit(ushort Player) {
	Panel_PlayerInit(Player, this);
}

/* Trick_HoldTrick__6CBrucei (0x8001d528) */
/* C++ (PHYSICS.CPP). Retail's body is just a tail call to Mess_TextHeight2 with `this` as the string
   (what the binary does; kept as is). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

int Mess_TextHeight2(const char *);

void CBruce::Trick_HoldTrick(int) {
	Mess_TextHeight2((const char *)this);
}

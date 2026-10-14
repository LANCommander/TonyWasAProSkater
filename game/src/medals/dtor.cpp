/* _._6CMedal (0x80073c94) */
/* C++ (MEDALS.CPP). Frees the medal's extras; CBody's destructor does the rest. */
#include "game/medals.h"

CMedal::~CMedal() {
	CMedal::DeleteStuff();
}

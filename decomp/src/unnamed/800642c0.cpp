/* reset__9PadBuffer (0x800642c0) */
/* C++ (SHELLUTILS.CPP). Empties the queue. */
#include "game/shellutils.h"

void PadBuffer::reset() {
	entries = 0;
	down = 0;
	last = 0;
	first = 0;
}

/* getPress__9PadBuffer (0x80064274) */
/* C++ (SHELLUTILS.CPP). Pops the oldest queued d-pad press (1..4), or 0 if none. */
#include "game/shellutils.h"

int PadBuffer::getPress() {
	int press = 0;

	if (entries > 0) {
		entries--;
		press = buffer[first];

		if (++first >= 8)
			first = 0;
	}

	return press;
}

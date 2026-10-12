/* SkipToEndif__6CBaddy (0x8003fbc4) */
/* C++ (BADDY.CPP). Skips script words up to the endif matching the current if (nested ifs
   0x4112-0x4116 count), or to the end of the script. */
#include "game/baddy.h"

void CBaddy::SkipToEndif() {
	int depth = 0;
	ushort op;

	do {
		op = *mpScript++;

		if (op == 0x4120) {
			if (depth == 0)
				return;

			depth--;
		} else if (op >= 0x4112 && op <= 0x4116)
			depth++;
	} while (op != 0x4100);
}

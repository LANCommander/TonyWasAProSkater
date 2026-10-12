/* ParseScript__6CBaddyPUs (0x8003fc1c) */
/* C++ (BADDY.CPP). Demo twin; locals from its MAIN.SYM. Runs script commands until the end marker:
   0x4000 commands go to ExecuteCommand (stopping the parse if it returns 0), 0x2000 ones to another
   virtual; both take the command word (still in a1, which is why retail keeps it there). */
#include "game/baddy.h"

void CBaddy::ParseScript(ushort *p_script) {
	mpScript = p_script;

	while (*mpScript != 0x4100) {
		ushort command = *mpScript++;

		if (command & 0x4000) {
			if (!ExecuteCommand(command))
				return;
		} else if (command & 0x2000) {
			SetVariable(command);
		}
	}

	mDoScript = 0;
}

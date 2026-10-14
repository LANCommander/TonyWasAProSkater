/* GetScriptValue__6CBaddy (0x80040c68) */
/* C++ (BADDY.CPP). Demo twin; locals from its MAIN.SYM. Reads the next script word: a literal value,
   or (0x2000 set, 0x8000 clear) a variable number resolved through the virtual GetVariable. */
#include "game/baddy.h"

int CBaddy::GetScriptValue() {
	ushort value = *mpScript++;

	if (!(value & 0x8000)) {
		if (value & 0x2000)
			value = GetVariable(value);
	}

	return value;
}

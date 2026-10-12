/* SetVariable__16CScriptOnlyBaddyUs (0x8004167c) */
/* C++ (BADDY.CPP). A script-only baddy has no variables of its own. */
#include "game/baddy.h"

void CScriptOnlyBaddy::SetVariable(ushort variable) {
	CBaddy::SetVariable(variable);
}

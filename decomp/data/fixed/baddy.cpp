/* Decompiled data that src/baddy/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char AI__16CScriptOnlyBaddy[];
extern char Die__6CBaddy[];
extern char ExecuteCommand__16CScriptOnlyBaddyUs[];
extern char ExecuteCommand__6CBaddyUs[];
extern char FUN_80041264[];
extern char FUN_80043cfc[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_800447ec[];
extern char GetVariable__6CBaddyUs[];
extern char SetVariable__16CScriptOnlyBaddyUs[];
extern char SetVariable__6CBaddyUs[];
extern char dtor_6CBaddy[] PSX_NAME("_._6CBaddy");

PSX_FIXED(0x800a0218) int __vt_16CScriptOnlyBaddy[22] = {
	0, 0, 0, (int)FUN_80041264, 0, (int)Die__6CBaddy, 0, (int)AI__16CScriptOnlyBaddy, 0,
	(int)FUN_800447e4, 0, (int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)FUN_800447d4, 0,
	(int)ExecuteCommand__16CScriptOnlyBaddyUs, 0, (int)SetVariable__16CScriptOnlyBaddyUs, 0,
	(int)GetVariable__6CBaddyUs,
};

PSX_FIXED(0x800a0270) int __vt_6CBaddy[22] = {
	0, 0, 0, (int)dtor_6CBaddy, 0, (int)Die__6CBaddy, 0, (int)FUN_800447ec, 0, (int)FUN_800447e4, 0,
	(int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)FUN_800447d4, 0,
	(int)ExecuteCommand__6CBaddyUs, 0, (int)SetVariable__6CBaddyUs, 0, (int)GetVariable__6CBaddyUs,
};

/* The vtables of the classes src/baddy/ defines (native data, psxdata.h). */
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

PsxVtEntry __vt_16CScriptOnlyBaddy[11] = {
	{0, 0, 0},
	{0, 0, FUN_80041264},
	{0, 0, Die__6CBaddy},
	{0, 0, AI__16CScriptOnlyBaddy},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, FUN_800447d4},
	{0, 0, ExecuteCommand__16CScriptOnlyBaddyUs},
	{0, 0, SetVariable__16CScriptOnlyBaddyUs},
	{0, 0, GetVariable__6CBaddyUs},
};

PsxVtEntry __vt_6CBaddy[11] = {
	{0, 0, 0},
	{0, 0, dtor_6CBaddy},
	{0, 0, Die__6CBaddy},
	{0, 0, FUN_800447ec},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, FUN_800447d4},
	{0, 0, ExecuteCommand__6CBaddyUs},
	{0, 0, SetVariable__6CBaddyUs},
	{0, 0, GetVariable__6CBaddyUs},
};

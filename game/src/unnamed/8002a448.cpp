/* FUN_8002a448 (0x8002a448) */
/* CSmokeTrail's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail
   has: CRibbon's destructor does the work. */
#include "game/bit.h"

extern "C" char __vt_11CSmokeTrail[];

/* egcs-ABI calls, as retail makes them */
extern "C" int BASEF(CRibbon *p, int a);
extern "C" void __dl__4CBitPv(void *p);

extern "C" void FUN_8002a448(CSmokeTrail *p, int f) {
	p->__vptr = __vt_11CSmokeTrail;
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

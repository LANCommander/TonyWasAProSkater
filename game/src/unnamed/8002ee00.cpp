/* FUN_8002ee00 (0x8002ee00) */
/* CWobblyGlow's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags). */
#include "game/bit.h"

extern "C" char __vt_11CWobblyGlow[];

extern "C" int BASEF(CGlow *p, int a);
extern "C" void __dl__4CBitPv(void *p);

extern "C" void FUN_8002ee00(CWobblyGlow *p, int f) {
	p->__vptr = __vt_11CWobblyGlow;
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

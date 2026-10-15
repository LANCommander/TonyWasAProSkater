/* FUN_8004eba0 (0x8004eba0) */
/* CFlatRipple's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags). */
#include "game/bit.h"

extern "C" char __vt_11CFlatRipple[];

extern "C" int BASEF(CFlatRipple *p, int a);
extern "C" void __dl__4CBitPv(void *p);

extern "C" void FUN_8004eba0(CFlatRipple *p, int f) {
	p->__vptr = __vt_11CFlatRipple;
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

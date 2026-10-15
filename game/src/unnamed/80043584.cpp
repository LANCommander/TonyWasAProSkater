/* FUN_80043584 (0x80043584) */
/* C++: CItem's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail has. */
#include "game/ob.h"

extern "C" char __vt_5CItem[];

extern "C" void __dl__5CItemPv(void *p); /* egcs-ABI call, as retail makes it */

extern "C" void FUN_80043584(CItem *p, int f) {
	p->__vptr = __vt_5CItem;

	if (f & 1)
		__dl__5CItemPv(p);
}

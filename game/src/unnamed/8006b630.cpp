/* FUN_8006b630 (0x8006b630) */
/* CCar's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags). */
#include "game/car.h"

extern "C" char __vt_4CCar[];
extern CBody *BaddyList;

extern "C" int BASEF(CCar *p, int a);
extern "C" void __dl__5CItemPv(void *p);

extern "C" void FUN_8006b630(CCar *p, int f) {
	p->__vptr = __vt_4CCar;
	p->DeleteFrom(&BaddyList);
	BASEF(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

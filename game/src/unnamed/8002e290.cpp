/* FUN_8002e290 (0x8002e290) */
/* CTexturedRibbon's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags). */
#include "game/bit.h"

extern "C" char __vt_15CTexturedRibbon[];

void Mem_Delete(void *);
extern "C" int BASEF(CTexturedRibbon *p, int a);
extern "C" void __dl__4CBitPv(void *p);

extern "C" void FUN_8002e290(CTexturedRibbon *p, int f) {
	p->__vptr = __vt_15CTexturedRibbon;
	Mem_Delete(p->mpPoints);
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

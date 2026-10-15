/* FUN_8001f718 (0x8001f718) */
/* CLight's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail has:
   deletes the light's glow through its vtable, then CNonRenderedBit's destructor. */
#include "game/bit.h"

/* an egcs vtable entry (game/data/psxdata.h PsxVtEntry) */
struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};

extern "C" char __vt_6CLight[];

/* egcs-ABI calls, as retail makes them */
extern "C" void FUN_80027fa4(CBit *p, int a);
extern "C" void __dl__4CBitPv(void *p);

extern "C" void FUN_8001f718(CLight *p, int f) {
	CWobblyGlow *o = p->mpGlow;

	p->__vptr = __vt_6CLight;

	if (o) {
		struct VtEntry *vt = (struct VtEntry *)((CBit *)o)->__vptr;

		vt[1].fn((char *)o + vt[1].delta, 3);
	}

	FUN_80027fa4(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

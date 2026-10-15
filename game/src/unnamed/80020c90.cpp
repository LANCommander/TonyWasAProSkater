/* FUN_80020c90 (0x80020c90) */
/* CShatterBit's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags): the smoke
   trail is deleted through its vtable, then ~CChunkBit. */
#include "game/bit.h"

extern "C" char __vt_11CShatterBit[];

extern "C" void FUN_80028968(CChunkBit *p, int a);
extern "C" void __dl__4CBitPv(void *p);

extern "C" void FUN_80020c90(CShatterBit *p, int f) {
	CSmokeTrail *o = p->mpSmokeTrail;

	p->__vptr = __vt_11CShatterBit;

	if (o)
		delete o;

	FUN_80028968(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

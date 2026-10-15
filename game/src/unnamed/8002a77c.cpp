/* FUN_8002a77c (0x8002a77c) */
/* C++: CGlassBit's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail
   has: leaves GlassList, then CBit's destructor. */
#include "game/bit.h"

extern "C" char __vt_9CGlassBit[];

CGlassBit *GlassList;

extern "C" int BASEF(CBit *p, int a);   /* CBit::~CBit, as retail calls it */
extern "C" void __dl__4CBitPv(void *p); /* egcs-ABI call, as retail makes it */

extern "C" void FUN_8002a77c(CGlassBit *p, int f) {
	p->__vptr = __vt_9CGlassBit;
	p->DeleteFrom(&GlassList);
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

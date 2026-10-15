/* FUN_800270e0 (0x800270e0) */
/* CPolyLine's constructor (n segments, white, at the origin; links into PolyLineList), kept as the
   egcs-ABI function retail has: CBit's constructor, then the vtable. */
#include "game/bit.h"
#include "game/m3d.h"

extern "C" char __vt_9CPolyLine[];
extern CBit *PolyLineList;
extern CVector ZeroVector;

extern "C" void *__4CBit(CBit *p); /* CBit::CBit, as retail calls it */
void *Mem_NewX(uint, int, int);

extern "C" CPolyLine *FUN_800270e0(CPolyLine *p, int n) {
	int i;

	__4CBit(p);

	p->__vptr = __vt_9CPolyLine;
	p->mSegs = (SLineSeg *)Mem_NewX(n << 4, 0, 1);

	for (i = 0; i < n; i++) {
		*(uint *)&p->mSegs[i].r = 0x40ffffff;
		p->mSegs[i].End = ZeroVector;
	}

	p->mNumSegs = n;
	p->AttachTo(&PolyLineList);
	return p;
}

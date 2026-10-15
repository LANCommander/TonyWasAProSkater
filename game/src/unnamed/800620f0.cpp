/* FUN_800620f0 (0x800620f0) */
/* SlicedImage's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail
   has: frees its VRAM (packed slices, or its own area) and 256-colour CLUT slot, and its pixels
   unless they were already removed from memory; then the inlined ~CClass. */
#include "game/shellutils.h"

extern "C" char __vt_11SlicedImage[];
extern "C" char DAT_800a21b0[];

void Pack_Unpack(short a, short b);
void Mem_Delete(void *p);
void Free256Slot(ushort s);
/* egcs-ABI call, as retail makes it */
extern "C" void __dl__6CClassPv(void *p);

extern "C" void FUN_800620f0(SlicedImage *p, int f) {
	p->__vptr = __vt_11SlicedImage;

	if (p->is_packed) {
		p->unpack();
	} else if (p->sliceTab) {
		Pack_Unpack(p->sliceTab->x, p->sliceTab->y);
		Mem_Delete(p->sliceTab);
	}

	if (p->bpp == 8)
		Free256Slot(p->clut);

	if (!p->m_removed_from_memory)
		Mem_Delete(p->slicedBuffer);

	p->__vptr = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

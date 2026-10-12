/* __dl__4CBitPv (0x80027d70) */
/* C++ (BIT.CPP). Frees a bit back to the heap. */
#include "game/bit.h"

void Mem_Delete(void *);

void CBit::operator delete(void *p) {
	Mem_Delete(p);
}

/* __dl__6CClassPv (0x80024238) */
/* C++ (OB.CPP). CClass's operator delete: frees through the memory manager. */
#include "game/class.h"

void Mem_Delete(void *);

void CClass::operator delete(void *p) {
	Mem_Delete(p);
}

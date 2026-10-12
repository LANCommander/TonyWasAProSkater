/* __dl__5CItemPv (0x80043530) */
/* C++ (OB.CPP). CItem's operator delete: frees through the memory manager. */
#include "game/mem.h"
#include "game/ob.h"

void CItem::operator delete(void *p) {
	Mem_Delete(p);
}

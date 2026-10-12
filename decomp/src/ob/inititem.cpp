/* InitItem__5CItemPc (0x800435b8) */
/* C++ (OB.CPP). Binds the item to the named spooled region, model 0. */
#include "game/ob.h"

int Spool_FindRegion(char *);

void CItem::InitItem(char *name) {
	mRegion = Spool_FindRegion(name);
	mModel = 0;
}

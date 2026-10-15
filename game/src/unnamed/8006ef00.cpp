/* FUN_8006ef00 (0x8006ef00) */
/* C++: an item's model checksum (its region's model checksum table). */
#include "game/ob.h"
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

extern "C" int FUN_8006ef00(CItem *p) {
	return PSXRegion[p->mRegion].pModelChecksums[p->mModel];
}

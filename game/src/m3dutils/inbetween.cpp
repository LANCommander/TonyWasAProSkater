/* M3dUtils_InBetween__FP6CSuper (0x80081448) */
/* C++. Blends a super item's parts between two animation frames when its frame says it has
   in-betweens (the count at +0xa of the frame's 8-byte entry in the region's animation). */
#include "game/m3d.h"
#include "game/ob.h"

extern SPSXRegion PSXRegion[];

void M3dUtils_InterpolateVectors(int, int, uint *, Item *, int, int);

void M3dUtils_InBetween(CSuper *s) {
	int i = s->mRegion;
	uint *f = PSXRegion[i].pAnimFile;
	int c = ((ushort *)(f + 2 * s->mAnim))[5];
	uint _t5683 = c + 1;

	if (c) {
		int n = PSXRegion[i].NumParts;

		M3dUtils_InterpolateVectors(4 * n, _t5683, f, (Item *)s, 0, n);
	}
}

/* Bit_RemoveDeadBits__Fv (0x80027c54) */
#include "names.h"

extern void *NonRenderedBitList;
extern void *FlatBitList;
extern void *Linked2EndedBitList;
extern void *PixelList;
extern void *PolyLineList;
extern void *GPolyLineList;
extern void *QuadBitList;
extern void *GenPolyList;
extern void *ChunkBitList;
extern void *GlowList;
extern void *GlassList;
extern void *GLineList;
extern void *SpecialDisplayList;

void RemoveDeadBits(void *p);

void Bit_RemoveDeadBits(void) {
	RemoveDeadBits(NonRenderedBitList);
	RemoveDeadBits(FlatBitList);
	RemoveDeadBits(Linked2EndedBitList);
	RemoveDeadBits(PixelList);
	RemoveDeadBits(PolyLineList);
	RemoveDeadBits(GPolyLineList);
	RemoveDeadBits(QuadBitList);
	RemoveDeadBits(GenPolyList);
	RemoveDeadBits(ChunkBitList);
	RemoveDeadBits(GlowList);
	RemoveDeadBits(GlassList);
	RemoveDeadBits(GLineList);
	RemoveDeadBits(SpecialDisplayList);
}

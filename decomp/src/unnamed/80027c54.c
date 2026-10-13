/* Bit_RemoveDeadBits__Fv (0x80027c54) */
#include "names.h"

void *NonRenderedBitList;
void *FlatBitList;
void *Linked2EndedBitList;
void *PixelList;
void *PolyLineList;
void *GPolyLineList;
void *QuadBitList;
void *GenPolyList;
void *ChunkBitList;
void *GlowList;
void *GlassList;
void *GLineList;
void *SpecialDisplayList;

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

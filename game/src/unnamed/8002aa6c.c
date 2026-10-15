/* Bit_Move__Fv (0x8002aa6c) */
#include "names.h"

extern void *PixelList;
extern void *FlatBitList;
extern void *GlassList;
extern void *GenPolyList;
extern void *ChunkBitList;
extern void *QuadBitList;
extern void *GlowList;
extern void *NonRenderedBitList;
extern void *Linked2EndedBitList;
extern void *PolyLineList;
extern void *GPolyLineList;
extern void *GLineList;
extern void *SpecialDisplayList;

void FUN_8002aa0c(void *p);

void Bit_Move(void) {
	FUN_8002aa0c(PixelList);
	FUN_8002aa0c(FlatBitList);
	FUN_8002aa0c(GlassList);
	FUN_8002aa0c(GenPolyList);
	FUN_8002aa0c(ChunkBitList);
	FUN_8002aa0c(QuadBitList);
	FUN_8002aa0c(GlowList);
	FUN_8002aa0c(NonRenderedBitList);
	FUN_8002aa0c(Linked2EndedBitList);
	FUN_8002aa0c(PolyLineList);
	FUN_8002aa0c(GPolyLineList);
	FUN_8002aa0c(GLineList);
	FUN_8002aa0c(SpecialDisplayList);
}

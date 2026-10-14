/* Bit_Move__Fv (0x8002aa6c) */
#include "names.h"

void *PixelList;
void *FlatBitList;
void *GlassList;
void *GenPolyList;
void *ChunkBitList;
void *QuadBitList;
void *GlowList;
void *NonRenderedBitList;
void *Linked2EndedBitList;
void *PolyLineList;
void *GPolyLineList;
void *GLineList;
void *SpecialDisplayList;

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

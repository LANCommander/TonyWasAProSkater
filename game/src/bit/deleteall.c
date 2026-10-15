/* Bit_DeleteAll__Fv (0x80027b28) */
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

void DeleteBitList(void *);

void Bit_DeleteAll(void) {
	DeleteBitList(NonRenderedBitList);
	DeleteBitList(FlatBitList);
	DeleteBitList(Linked2EndedBitList);
	DeleteBitList(PixelList);
	DeleteBitList(PolyLineList);
	DeleteBitList(GPolyLineList);
	DeleteBitList(QuadBitList);
	DeleteBitList(GenPolyList);
	DeleteBitList(ChunkBitList);
	DeleteBitList(GlowList);
	DeleteBitList(GlassList);
	DeleteBitList(GLineList);
	DeleteBitList(SpecialDisplayList);
}

/* Bit_DeleteAll__Fv (0x80027b28) */
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

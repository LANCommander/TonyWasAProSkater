/* Bit_Init__Fv (0x800279cc) */
#include "names.h"
#include <sys/types.h>
#include <libgpu.h>

extern int BitCount;
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
DR_TPAGE StoredTPagePrim; /* the draw-mode packet the bits copy (native: no retail address) */
extern void *pShadowTexture;

extern void *ButtonsAnim __attribute__((section(".data")));

extern char *AnimNames[];
extern void *QuickAnimLookup[];

void FUN_80092f30(void *, int, int, int);
void *Spool_FindAnim(char *);
void *Spool_FindTextureEntry(unsigned);

void Bit_Init(void) {
	int i;

	BitCount = 0;
	NonRenderedBitList = 0;
	FlatBitList = 0;
	Linked2EndedBitList = 0;
	PixelList = 0;
	PolyLineList = 0;
	GPolyLineList = 0;
	QuadBitList = 0;
	GenPolyList = 0;
	ChunkBitList = 0;
	GlowList = 0;
	GlassList = 0;
	GLineList = 0;
	SpecialDisplayList = 0;
	FUN_80092f30(&StoredTPagePrim, 1, 1, GetTPage(0, 1, 0, 0));

	for (i = 0; i < 6; i++)
		QuickAnimLookup[i] = Spool_FindAnim(AnimNames[i]);

	ButtonsAnim = Spool_FindAnim("Buttons");
	pShadowTexture = Spool_FindTextureEntry(0x2a3b2550);
}

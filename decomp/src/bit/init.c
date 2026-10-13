/* Bit_Init__Fv (0x800279cc) */
#include "names.h"

int BitCount;
int NonRenderedBitList;
int FlatBitList;
int Linked2EndedBitList;
int PixelList;
int PolyLineList;
int GPolyLineList;
int QuadBitList;
int GenPolyList;
int ChunkBitList;
int GlowList;
int GlassList;
int GLineList;
int SpecialDisplayList;
int StoredTPagePrim;
int pShadowTexture;

extern int ButtonsAnim __attribute__((section(".data")));

extern char *AnimNames[];
extern int QuickAnimLookup[];

int GetTPage(int, int, int, int);
void FUN_80092f30(int *, int, int, int);
int Spool_FindAnim(char *);
int Spool_FindTextureEntry(unsigned);

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

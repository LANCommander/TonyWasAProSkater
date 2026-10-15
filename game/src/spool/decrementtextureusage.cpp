/* DecrementTextureUsage__Fi (0x8006f178) */
/* C++ (SPOOL.CPP). Demo twin (the demo version also skips regions of type 2); locals from its
   MAIN.SYM. Walks the region's texture table (the port keeps it as entries, ProcessNewPSX) and
   drops each texture's usage count. */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

uint *Spool_SkipPackets(uint *);

void DecrementTextureUsage(int region) {
	Texture **ppTextures = PSXRegion[region].ppTextures;
	uint NumChecksums = PSXRegion[region].NumTextures;

	{
		int i;

		for (i = 0; i < NumChecksums; i++)
			ppTextures[i]->Usage--;
	}
}

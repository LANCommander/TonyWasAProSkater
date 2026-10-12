/* DecrementTextureUsage__Fi (0x8006f178) */
/* C++ (SPOOL.CPP). Demo twin (the demo version also skips regions of type 2); locals from its
   MAIN.SYM. Walks the region's texture checksum table (after the model table in its PSX file) and
   drops each texture's usage count. */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

uint *Spool_SkipPackets(uint *);

void DecrementTextureUsage(int region) {
	uint *pPSX;
	uint *pNumModels;
	uint NumModels;
	uint *ChecksumTable;
	uint NumChecksums;

	pPSX = PSXRegion[region].pPSX;
	pNumModels = (uint *)PSXRegion[region].ppModels;
	NumModels = pNumModels[-1];
	ChecksumTable = Spool_SkipPackets(pPSX) + (NumModels + 1);
	NumChecksums = ChecksumTable[-1];

	{
		int i;

		for (i = 0; i < NumChecksums; i++)
			((Texture *)ChecksumTable[i])->Usage--;
	}
}

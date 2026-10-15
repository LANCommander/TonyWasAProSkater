/* Spool_Init__Fv (0x8006ec14) */
#include "game/psyq.h"
#include "game/m3d.h"
#include "game/spool.h"

struct SEnviro;

extern SPSXRegion PSXRegion[];
extern SQueue SpoolQ[];
extern Texture *TextureChecksumHashTable[];

extern int SpoolPos;
extern int FreePos;
extern AnimPacket *AnimPackets;
extern int EnvRegions[2];
extern int DAT_800d1ed8;
extern int DAT_800d1edc;
extern SEnviro *EnviroList;
extern int OtherEnvIndex;

void Spool_Init() {
	int i;

	for (i = 0; i < 20; i++)
		SpoolQ[i].Occupied = 0;

	SpoolPos = 0;
	FreePos = 0;

	for (i = 0; i < 40; i++) {
		PSXRegion[i].Filename[0] = 0;
		PSXRegion[i].Protected = 0;
		PSXRegion[i].Usable = 0;
		free(PSXRegion[i].ppModels);
		free(PSXRegion[i].ppTextures);
		PSXRegion[i].ppModels = 0;
		PSXRegion[i].ppTextures = 0;
		free(PSXRegionItems[i]);
		PSXRegionItems[i] = 0;
		PSXRegion[i].pPSX = 0;
		PSXRegion[i].pAnimFile = 0;
		PSXRegion[i].pHierarchy = 0;
		PSXRegion[i].pColourPulseData = 0;
		PSXRegion[i].pTexWibData = 0;
		PSXRegion[i].pHooks = 0;
		PSXRegion[i].TexturesInRAM = 0;
		PSXRegion[i].KeepTexturesInRAM = 0;
	}

	for (i = 0; i < 512; i++)
		TextureChecksumHashTable[i] = 0;

	AnimPackets = 0;
	EnvRegions[0] = -1;
	EnvRegions[1] = -1;
	DAT_800d1ed8 = 0;
	DAT_800d1edc = 0;
	EnviroList = 0;
	OtherEnvIndex = 0;
}

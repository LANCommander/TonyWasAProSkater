/* ClearRegion__Fiii (0x8006fde4) */
/* C++ (SPOOL.CPP). Demo twin; locals and blocks from its MAIN.SYM. Releases a loaded region: drops
   its texture usage counts and, unless it stays in memory, clears the region record, detaches its
   environment and frees the PSX file. Optionally purges unused textures. */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

int EnvRegions[2];

void DecrementTextureUsage(int);
int Spool_GetEnvIndex(int);
void DetachOldEnviro(uchar);
void Spool_RemoveUnusedTextures(void);

extern "C" {
	void FUN_80082738(int);
	void Mem_Delete__FPv(void *);
}

void ClearRegion(int Region, int cleanuptextures, int KeepInMemory) {
	if (Region == -1)
		return;

	if (!PSXRegion[Region].Filename[0])
		return;

	DecrementTextureUsage(Region);

	if (!KeepInMemory) {
		PSXRegion[Region].pAnimFile = 0;
		PSXRegion[Region].pColourPulseData = 0;
		PSXRegion[Region].pTexWibData = 0;
		PSXRegion[Region].Protected = 0;
		PSXRegion[Region].Usable = 0;
		PSXRegion[Region].pHooks = 0;
		PSXRegion[Region].ppModels = 0;
		PSXRegion[Region].Filename[0] = 0;
		PSXRegion[Region].TexturesInRAM = 0;
		PSXRegion[Region].KeepTexturesInRAM = 0;

		{
			int EnvIndex = Spool_GetEnvIndex(Region);

			if (EnvIndex != -1) {
				FUN_80082738(EnvIndex);
				DetachOldEnviro(EnvIndex);

				EnvRegions[EnvIndex] = -1;
			}
		}

		Mem_Delete__FPv(PSXRegion[Region].pPSX);

		PSXRegion[Region].pPSX = 0;
	}

	if (cleanuptextures)
		Spool_RemoveUnusedTextures();
}

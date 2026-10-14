/* M3dZone_SetZone__FiPUi (0x8008301c) */
/* C++ (M3DZONE.CPP). Exact demo twin; locals from its MAIN.SYM. Reads a zone header from the packed
   PSX data (box, cell counts), then each cell's item list, relocating item indices into pointers
   into the environment's PSX item table.
   Levers: the two short reads use g++'s cast-as-lvalue post-increment (`*((short *)pPack)++`),
   which keeps retail's unfolded pointer copies; `(PSXRegion_14 + i)->pPSX` and the sized `[1]`
   extern give retail's loop-preheader order. */
#include "game/m3d.h"

extern SZone Zones[2];
extern int EnvRegions[1];

/* PSXRegion[] seen from +0x14 (SPSXRegion::pPSX): indexing this view, rather than PSXRegion[i].pPSX,
   gives retail's address arithmetic */
struct SEnvPSX {
	uint *pPSX;
	char pad[60];
};

extern SEnvPSX PSXRegion_14[];

void M3dZone_SetZone(int EnvIndex, uint *pPack) {
	{
		int Z;

		Zones[EnvIndex].xMin = *pPack++;
		Zones[EnvIndex].zMin = *pPack++;
		Zones[EnvIndex].xMax = *pPack++;
		Zones[EnvIndex].zMax = *pPack++;
		Zones[EnvIndex].Width = *((short *)pPack)++;
		Zones[EnvIndex].Height = *((short *)pPack)++;
		Zones[EnvIndex].Flags = 1;
		Zones[EnvIndex].ZoneWidth =
		    (Zones[EnvIndex].xMax - Zones[EnvIndex].xMin) / Zones[EnvIndex].Width;

		for (Z = 0; Z < Zones[EnvIndex].Height; Z++) {
			int X;

			for (X = 0; X < Zones[EnvIndex].Width; X++) {
				int count;
				uint *pPSX;

				pPack += 2;
				count = *pPack++;
				Zones[EnvIndex].Ptr[X][Z] = (Item *)pPack;
				pPSX = (PSXRegion_14 + EnvRegions[EnvIndex])->pPSX;

				while (count-- > 0) {
					*pPack = (uint)pPSX + 12 + *pPack * 36;
					pPack++;
				}

				pPack++;
			}
		}
	}
}

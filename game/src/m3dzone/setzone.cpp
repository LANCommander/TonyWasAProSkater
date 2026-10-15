/* M3dZone_SetZone__FiPUi (0x8008301c) */
/* C++ (M3DZONE.CPP). Exact demo twin; locals from its MAIN.SYM. Reads a zone header from the packed
   PSX data (box, cell counts), then each cell's item list: item indices in the environment's PSX
   item table. Retail turned them into item pointers in place; the port makes each cell a list of
   its own (in M3dZone_Lists, ended by 0) of the environment's Items (PSXRegionItems). */
#include "game/m3d.h"
#include "game/psyq.h"

extern SZone Zones[2];
extern int EnvRegions[1];

/* the port's: each zone's cell lists, one block (M3dZone_FreePSX frees it) */
Item **M3dZone_Lists[2];

void M3dZone_SetZone(int EnvIndex, uint *pPack) {
	{
		int Z;
		uint *pCells;
		int Total = 0;
		Item **pList;

		Zones[EnvIndex].xMin = *pPack++;
		Zones[EnvIndex].zMin = *pPack++;
		Zones[EnvIndex].xMax = *pPack++;
		Zones[EnvIndex].zMax = *pPack++;
		Zones[EnvIndex].Width = *((short *)pPack)++;
		Zones[EnvIndex].Height = *((short *)pPack)++;
		Zones[EnvIndex].Flags = 1;
		Zones[EnvIndex].ZoneWidth =
		    (Zones[EnvIndex].xMax - Zones[EnvIndex].xMin) / Zones[EnvIndex].Width;

		/* the cells: two words, the count, the item indices and one more word each */
		pCells = pPack;

		for (Z = 0; Z < Zones[EnvIndex].Height * Zones[EnvIndex].Width; Z++) {
			int count = pPack[2];

			Total += count + 1;
			pPack += 3 + count + 1;
		}

		free(M3dZone_Lists[EnvIndex]);
		pList = M3dZone_Lists[EnvIndex] = (Item **)malloc((Total + 1) * sizeof(Item *));
		pPack = pCells;

		for (Z = 0; Z < Zones[EnvIndex].Height; Z++) {
			int X;

			for (X = 0; X < Zones[EnvIndex].Width; X++) {
				int count;
				Item *pItems;

				pPack += 2;
				count = *pPack++;
				Zones[EnvIndex].Ptr[X][Z] = (Item *)pList;
				pItems = PSXRegionItems[EnvRegions[EnvIndex]];

				while (count-- > 0)
					*pList++ = pItems + *pPack++;

				/* (the list's end, 0 in the file) */
				*pList++ = (Item *)(uintptr_t)*pPack++;
			}
		}
	}
}

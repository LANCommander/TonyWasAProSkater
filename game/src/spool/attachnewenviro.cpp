/* AttachNewEnviro__Fi (0x8006f7a4) */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

extern Item *EnviroList;
uint *pCurrentTexWibPacket;
uint *pTexWibPacket[2];

int Spool_GetEnvIndex(int);

void AttachNewEnviro(int region) {
	/* the region's items (M3dInit_ParsePSX) go before the list's */
	if (!EnviroList) {
		EnviroList = PSXRegionItems[region];
	} else {
		Item *head = PSXRegionItems[region];
		Item *p = head;

		while (p->NextItem)
			p = p->NextItem;

		p->NextItem = EnviroList;
		EnviroList = head;
	}

	pTexWibPacket[Spool_GetEnvIndex(region)] = pCurrentTexWibPacket;
}

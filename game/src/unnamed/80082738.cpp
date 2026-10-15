/* M3dZone_FreePSX__Fi (0x80082738) */
/* C++. Forgets environment a's zone: its flags and cells (and the port's cell lists). */
#include "game/m3d.h"
#include "game/psyq.h"

extern SZone Zones[2];
extern Item **M3dZone_Lists[2];

void M3dZone_FreePSX(int a) {
	int i, j;

	if (a < 2) {
		Zones[a].Flags = 0;

		for (i = 0; i < 20; i++)
			for (j = 0; j < 20; j++)
				Zones[a].Ptr[i][j] = 0;

		free(M3dZone_Lists[a]);
		M3dZone_Lists[a] = 0;
	}
}

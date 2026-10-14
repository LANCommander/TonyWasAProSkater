/* M3dZone_FreePSX__Fi (0x80082738) */
#include "names.h"

struct B {
	int x;
	int pad[7];
	int g[20][20];
};

extern struct B Zones[];

void M3dZone_FreePSX(int a) {
	int i, j;

	if (a < 2) {
		Zones[a].x = 0;

		for (i = 0; i < 20; i++)
			for (j = 0; j < 20; j++)
				Zones[a].g[i][j] = 0;
	}
}

/* Front_FreeSkaterSpool2P__Fv (0x8005609c) */
#include "game/career.h"
#include "game/frontend.h"
#include "game/spool.h"

extern SStats Stats[];

int DAT_800d2304[2];
int DAT_800d231c[2];
int DAT_800d230c, DAT_800d2310, DAT_800d2314;

extern int DAT_800d3758[], DAT_800d3788[];

void Spool_ClearPSX(char *);

void Front_FreeSkaterSpool2P() {
	int i, v, j;

	Front_SelectSkaterPack(0);

	for (i = 0; i < 2; i++) {
		if (DAT_800d231c[i]) {
			ClearRegion(DAT_800d3758[DAT_800d2304[i]], 1, 1);
			Spool_RestorePSX(DAT_800d3758[DAT_800d2304[i]]);

			DAT_800d231c[i] = 0;

			if (DAT_800d2304[0] == DAT_800d2304[1])
				DAT_800d231c[1 - i] = 0;
		}
	}

	Spool_Sync();

	v = DAT_800d230c + DAT_800d2310;

	if (v >= 11)
		v -= 11;

	j = 0;

	if (DAT_800d2314 == 0)
		DAT_800d3788[v] = 1;

	for (j = 0; j < 11; j++) {
		if (DAT_800d3788[j]) {
			ProcessNewPSX(DAT_800d3758[j]);
			Spool_ClearPSX(Stats[j].FrontName);
		}
	}

	Spool_Sync();
}

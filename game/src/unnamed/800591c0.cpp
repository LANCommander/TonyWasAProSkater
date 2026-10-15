/* Front_FreeSkaterSpool1P__Fv (0x800591c0) */
#include "game/career.h"
#include "game/frontend.h"
#include "game/spool.h"

extern SStats Stats[];

int DAT_800d2344, DAT_800d1c40, DAT_800d2338, DAT_800d233c, DAT_800d2340;

extern int DAT_800d37b4[], DAT_800d37e4[];

void Spool_ClearPSX(char *);

void Front_FreeSkaterSpool1P() {
	int i, v;

	if (DAT_800d2344 && *Stats[DAT_800d1c40].FrontName) {
		ClearRegion(DAT_800d37b4[DAT_800d1c40], 1, 1);
		Spool_RestorePSX(DAT_800d37b4[DAT_800d1c40]);
	}

	Spool_Sync();

	v = DAT_800d2338 + DAT_800d233c;

	if (v >= 11)
		v -= 11;

	i = 0;

	if (DAT_800d2340 == 0)
		DAT_800d37e4[v] = 1;

	for (i = 0; i < 11; i++) {
		if (*Stats[i].FrontName && DAT_800d37e4[i]) {
			Front_ResetSkaterPack();
			ProcessNewPSX(DAT_800d37b4[i]);
			Spool_ClearPSX(Stats[i].FrontName);
		}
	}

	Spool_Sync();
}

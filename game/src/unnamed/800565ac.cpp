/* Front_LoadSkaterTextures2P__Fv (0x800565ac) */
#include "game/career.h"
#include "game/frontend.h"
#include "game/pack.h"
#include "game/spool.h"

extern SStats Stats[];

extern int DAT_800d2304[2];
extern int DAT_800d231c[2];

extern int DAT_800d3758[];
extern int Cheat_BigHead, GHeadPart, GHeadUp;

void Pack_Unpack(short, short);

void Front_LoadSkaterTextures2P() {
	short ab[4];
	SStats *e;
	int i, n;

	Front_SelectSkaterPack(0);

	n = 2;

	if (DAT_800d2304[0] == DAT_800d2304[1] && DAT_800d231c[0] && DAT_800d231c[1])
		n = 1;

	for (i = 0; i < n; i++) {
		if (n == 2) {
			if (i == 0)
				Pack_Pack(0, 128, 256, (int *)&ab[0], (int *)&ab[2], 16, 0, 0);
			else
				Pack_Unpack(ab[0], ab[2]);
		}

		if (DAT_800d231c[i]) {
			Spool_BackupPSX(DAT_800d3758[DAT_800d2304[i]]);

			if (Cheat_BigHead) {
				e = Stats + DAT_800d2304[i];
				GHeadUp = e->Neck;
				GHeadPart = e->Head;
			}

			ProcessNewPSX(DAT_800d3758[DAT_800d2304[i]]);

			GHeadPart = -1;
		}
	}
}

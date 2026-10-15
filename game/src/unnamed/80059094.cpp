/* Front_SpoolNextSkater1P__Fv (0x80059094) */
#include "game/career.h"
#include "game/fileio.h"
#include "game/spool.h"

extern SStats Stats[];
extern int DAT_800d37e4[];
extern int DAT_800d37b4[];

extern int DAT_800d2338;
extern int DAT_800d2324;
extern int DAT_800d233c;
extern int DAT_800d2340;

extern "C" int FUN_80070830(void);
int Spool_PSX(char *Name, int IsEnviro, int WhichHeap);

void Front_SpoolNextSkater1P() {
	int a;
	int n;

	FileIO_Continue();
	Spool_Continue();

	if (FUN_80070830())
		return;

	Spool_Sync();

	a = DAT_800d2338;

	if (a >= 0)
		DAT_800d37e4[DAT_800d2324] = 1;

	if (a == -1) {
		DAT_800d2338 = 0;
	} else {
		if (a < 5)
			n = 10;
		else if (a < 6) {
			DAT_800d2340 = 1;
			return;
		} else
			n = 11;

		DAT_800d2338 = n - a;
	}

	DAT_800d2324 = DAT_800d2338 + DAT_800d233c;

	if (!(DAT_800d2324 < 11))
		DAT_800d2324 = DAT_800d2324 - 11;

	if (*Stats[DAT_800d2324].FrontName)
		DAT_800d37b4[DAT_800d2324] = Spool_PSX(Stats[DAT_800d2324].FrontName, 2, 1);
}

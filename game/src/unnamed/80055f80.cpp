/* Front_SpoolNextSkater2P__Fv (0x80055f80) */
#include "game/career.h"
#include "game/fileio.h"
#include "game/spool.h"

extern SStats Stats[];
extern int DAT_800d3788[];
extern int DAT_800d3758[];

int DAT_800d230c;
int DAT_800d22f4;
int DAT_800d2310;
int DAT_800d2314;

extern "C" int FUN_80070830(void);
int Spool_PSX(char *Name, int IsEnviro, int WhichHeap);

void Front_SpoolNextSkater2P() {
	int a;
	int n;

	FileIO_Continue();
	Spool_Continue();

	if (FUN_80070830())
		return;

	Spool_Sync();

	a = DAT_800d230c;

	if (a >= 0)
		DAT_800d3788[DAT_800d22f4] = 1;

	if (a == -1) {
		DAT_800d230c = 0;
	} else {
		if (a < 5)
			n = 10;
		else if (a < 6) {
			DAT_800d2314 = 1;
			return;
		} else
			n = 11;

		DAT_800d230c = n - a;
	}

	DAT_800d22f4 = DAT_800d230c + DAT_800d2310;

	if (!(DAT_800d22f4 < 11))
		DAT_800d22f4 = DAT_800d22f4 - 11;

	DAT_800d3758[DAT_800d22f4] = Spool_PSX(Stats[DAT_800d22f4].FrontName, 2, 1);
}

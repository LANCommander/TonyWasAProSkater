/* Front_SpoolNextSkater2P__Fv (0x80055f80) */
#include "names.h"

struct E {
	char pad[8];
	char *n8;
	char pad2[328];
};

extern struct E Stats[];
extern int DAT_800d3788[];
extern int DAT_800d3758[];

int DAT_800d230c;
int DAT_800d22f4;
int DAT_800d2310;
int DAT_800d2314;

void FileIO_Continue(void);
void Spool_Continue(void);
int FUN_80070830(void);
void Spool_Sync(void);
int Spool_PSX(char *a, int b, int c);

void Front_SpoolNextSkater2P(void) {
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

	DAT_800d3758[DAT_800d22f4] = Spool_PSX(Stats[DAT_800d22f4].n8, 2, 1);
}

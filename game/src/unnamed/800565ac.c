/* Front_LoadSkaterTextures2P__Fv (0x800565ac) */
#include "names.h"

typedef struct {
	char p[0x13];
	unsigned char b13;
	signed char b14;
	char q[0x154 - 0x15];
} E;

extern E Stats[];

int DAT_800d2304[2];
int DAT_800d231c[2];

extern int DAT_800d3758[];
extern int Cheat_BigHead, GHeadPart, GHeadUp;

void Front_SelectSkaterPack(int);
void Pack_Pack(int, int, int, short *, short *, int, int, int);
void Spool_BackupPSX(int);
void ProcessNewPSX(int);
void Pack_Unpack(short, short);

void Front_LoadSkaterTextures2P(void) {
	short ab[4];
	E *e;
	int i, n;

	Front_SelectSkaterPack(0);

	n = 2;

	if (DAT_800d2304[0] == DAT_800d2304[1] && DAT_800d231c[0] && DAT_800d231c[1])
		n = 1;

	for (i = 0; i < n; i++) {
		if (n == 2) {
			if (i == 0)
				Pack_Pack(0, 128, 256, &ab[0], &ab[2], 16, 0, 0);
			else
				Pack_Unpack(ab[0], ab[2]);
		}

		if (DAT_800d231c[i]) {
			Spool_BackupPSX(DAT_800d3758[DAT_800d2304[i]]);

			if (Cheat_BigHead) {
				e = Stats + DAT_800d2304[i];
				GHeadUp = e->b14;
				GHeadPart = e->b13;
			}

			ProcessNewPSX(DAT_800d3758[DAT_800d2304[i]]);

			GHeadPart = -1;
		}
	}
}

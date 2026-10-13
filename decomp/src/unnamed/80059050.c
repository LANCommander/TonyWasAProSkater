/* Front_InitSkaterSpool1P__Fv (0x80059050) */
#include "names.h"

int DAT_800d2338;
int DAT_800d2340;
int DAT_800d2344;
int DAT_800d233c;

extern int DAT_800d37e4[];
extern unsigned char DAT_800a7068[];

void Front_InitSkaterSpool1P(void) {
	int i;

	DAT_800d2338 = -1;
	DAT_800d2340 = 0;

	for (i = 0; i < 11; i++)
		DAT_800d37e4[i] = 0;

	DAT_800d2344 = 0;
	DAT_800d233c = DAT_800a7068[0];
}

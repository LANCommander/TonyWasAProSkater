/* Front_InitSkaterSpool2P__Fv (0x80055eb8) */
#include "names.h"

/* Credits state reset. The start position is the midpoint of two career values on a circular
   scale of 11, taking the shorter way round. */
typedef struct {
	char pad[0x824];
	unsigned char b824, b825;
} SCareer;

extern SCareer GCareer;
extern int DAT_800d3788[11];

int DAT_800d231c;
int DAT_800d2320;
int DAT_800d230c;
int DAT_800d2310;
int DAT_800d2314;

void Front_InitSkaterSpool2P(void) {
	int i;
	unsigned char hi, lo;

	DAT_800d231c = 0;
	DAT_800d2320 = 0;

	for (i = 0; i < 11; i++)
		DAT_800d3788[i] = 0;

	DAT_800d230c = -1;
	hi = GCareer.b825 < GCareer.b824 ? GCareer.b824 : GCareer.b825;
	lo = GCareer.b825 < GCareer.b824 ? GCareer.b825 : GCareer.b824;

	{
		int h1 = hi - 11;

		if (lo - h1 < hi - lo) {
			int h2 = hi + 11;

			DAT_800d2310 = (lo + h2) / 2;

			if (DAT_800d2310 >= 11)
				DAT_800d2310 -= 11;
		} else {
			DAT_800d2310 = (lo + hi) / 2;
		}
	}

	DAT_800d2314 = 0;
}

/* SFX_Grunt__FP6CBruce (0x80043408) */
#include "game/bruce.h"
#include "game/career.h"
#include "game/sfx.h"

extern SStats Stats[];
extern int Face_Terrain;
extern int GLevel;

int Redbook_Rnd(int);

void SFX_Grunt(CBruce *p) {
	int a;

	switch (Face_Terrain) {
		case 7:
			a = 53;
			break;
		case 9:
			a = 40;

			if (GLevel == 8)
				a = 137;

			break;
		default:
			goto out;
	}

	SFX_PlayX(a, 8192, 0);
out:
	a = Stats[p->mSkaterIndex].ComboID;

	if (a == 0x9000 || a == 0xb000)
		a = 31;
	else
		a = Redbook_Rnd(3) + 28;

	SFX_PlayX(a, 8192, 0);
}

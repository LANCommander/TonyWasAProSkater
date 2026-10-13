/* SFX_Grunt__FP6CBruce (0x80043408) */
#include "names.h"

struct E {
	char pad[12];
	int f;
	char q[324];
};

extern struct E Stats[];
extern int Face_Terrain;
extern int GLevel;

int Redbook_Rnd(int);
void SFX_PlayX(int, int, int);

void SFX_Grunt(char *p) {
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
	a = Stats[*(int *)(p + 744)].f;

	if (a == 0x9000 || a == 0xb000)
		a = 31;
	else
		a = Redbook_Rnd(3) + 28;

	SFX_PlayX(a, 8192, 0);
}

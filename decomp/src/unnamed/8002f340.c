/* Trick_Trick__6CBruceiPcii (0x8002f340) */
#include "names.h"
typedef struct {
	char p0[0x330];
	int f330;
	char p1[0x34c - 0x334];
	int f34c;
	char p2[0x428 - 0x350];
	int a428[0x50];
	int a568[0x50];
	char p3[0x760 - 0x6a8];
	int f760;
} B;

extern int GGame;

int FUN_8002f290(B *, int, int);
void FUN_8006e19c(void *, int, int, int);

void CBruce_Trick_Trick(B *p, int a1, int a2, int a3, int a4) {
	if (p->f760) {
		if (GGame == 7)
			p->f330 = 0;

		p->a428[a1]++;
		p->a568[a1]++;

		if (a3 < 0)
			a3 = -a3;
		else
			a3 = FUN_8002f290(p, a1, a3);

		if (p->f34c == 4) {
			p->a428[a1]--;
			a3 = 0;
			p->a568[a1]--;
		}

		FUN_8006e19c(p, a2, a3, 0);
	}
}

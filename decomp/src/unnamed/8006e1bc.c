/* FUN_8006e1bc (0x8006e1bc) */
#include "names.h"

typedef struct {
	int Score;
	char pad[16];
} T;

typedef struct {
	char p0[2340];
	char sub[2848 - 2340];
	T tq[19];
	char p1[3244 - 2848 - 380];
	int n;
} P;

void CPanelData_TrickMessageRefresh(char *);

void FUN_8006e1bc(P *p, int s) {
	p->tq[p->n - 1].Score += s;
	CPanelData_TrickMessageRefresh(p->sub);
}

/* FUN_800506d0 (0x800506d0) flags=-fstructure-noalias */
#include "names.h"

struct E {
	char pad[8];
	char *n8;
	char pad2[52];
	char *name;
	char q[272];
};

struct V {
	int x, y, z;
};

struct B {
	int vt;
	struct V pos;
	char pad[16];
	int col;
};

static inline void Set(struct V *v, int _x, int _y, int _z) {
	v->x = _x;
	v->y = _y;
	v->z = _z;
}

extern struct E Stats[];
extern char DAT_800be3f8[];

int DAT_800d1ba8;
int SKATER_Y_POS;

char *__nw__5CItemUi(int n);
char *CBruce_ctor(char *p);
void CBruce_InitItem(char *t, char *n);
void CSuper_CycleAnim(char *t, int a, signed char c);
void M3dMaths_SetIdentityRotation__FP6MATRIX(char *m); /* identity-matrix init, one arg */

char *FUN_800506d0(int idx) {
	char *p;

	p = __nw__5CItemUi(3612);

	if (p)
		p = CBruce_ctor(p);

	*(int *)(p + 744) = idx;
	CBruce_InitItem(p, Stats[idx].n8);

	*(int *)(p + 336) = 0x10000;
	CSuper_CycleAnim(p, 0, 1);

	{
		struct B *b = (struct B *)p;

		b->col = 0x404040;
		Set(&b->pos, DAT_800d1ba8, SKATER_Y_POS, 0x30000);
	}

	M3dMaths_SetIdentityRotation__FP6MATRIX(p + 1932);

	if (Stats[idx].name[0] == 82 || Stats[idx].name[0] == 114)
		*(int *)(p + 308) |= 2;

	*(char **)(p + 60) = DAT_800be3f8;
	return p;
}

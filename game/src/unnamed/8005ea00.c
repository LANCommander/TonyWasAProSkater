/* FUN_8005ea00 (0x8005ea00) */
#include "names.h"

struct S {
	char *a, *b, *c;
};

struct K {
	int a, b, c;
};

struct VerticalMenu;

extern struct S DAT_800a1cac;

int Front_RunMenu(char **, int *, int, int, int *, void (*)(struct VerticalMenu *), int *, int *, int);

/* the menu's result (Front_RunMenu) */
int FUN_8005ea00(int a) {
	struct S t = DAT_800a1cac;
	struct K z;

	z.a = 0;
	z.b = 0;
	z.c = 0;
	return Front_RunMenu(&t.a, &z.a, 3, 32, &a, 0, 0, 0, 0);
}

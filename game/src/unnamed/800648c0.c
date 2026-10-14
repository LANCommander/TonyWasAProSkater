/* FUN_800648c0 (0x800648c0) */
#include "names.h"

typedef struct {
	int pad0;
	char str[0x34];
	int disabled;
	int pad1;
} Item;

typedef struct {
	char pad[16];
	int count;
	int prev;
	int cur;
	Item items[12];
	int timer;
} M;

void VerticalMenu_select(M *, int);

void FUN_800648c0(M *m, int i, int on) {
	int j;

	if (!on) {
		m->items[i].disabled = 0;
		return;
	}

	m->items[i].disabled = 1;

	if (m->cur == i) {
		for (j = i + 1; j < m->count; j++)
			if (!m->items[j].disabled) {
				VerticalMenu_select(m, j);
				return;
			}

		for (j = 0; j < i; j++)
			if (!m->items[j].disabled) {
				VerticalMenu_select(m, j);
				return;
			}
	}
}

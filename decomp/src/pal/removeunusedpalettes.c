/* Pal_RemoveUnusedPalettes__Fv (0x8004ac2c) */
#include "names.h"

typedef struct SPalette {
	char pad[2];
	unsigned char slot, flags;
	unsigned short usage;
	char pad6[10];
	struct SPalette *next;
} SPalette;

SPalette *DAT_800d1a80;

extern unsigned char Free16Slots[], Free256Slots[];

void RemovePaletteEntry(SPalette *);

void Pal_RemoveUnusedPalettes(void) {
	SPalette *p = DAT_800d1a80;

	while (p) {
		SPalette *next = p->next;

		if (!p->usage) {
			if (p->flags & 1)
				Free16Slots[p->slot] = 1;

			if (p->flags & 2)
				Free256Slots[p->slot] = 1;

			RemovePaletteEntry(p);
		}

		p = next;
	}
}

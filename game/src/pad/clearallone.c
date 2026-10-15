/* Pad_ClearAllOne__Fi (0x80070960) */
#include "game/padbuttons.h"
#include "names.h"

extern char Pad[];

extern void Pad_Clear(char *);

void Pad_ClearAllOne__Fi(int i) {
	int o = i * PAD_SIZE;
	char *p = Pad + o;

	Pad_Clear(p);

	*(int *)(Pad + o + 0x150) = 0;
	p[0x14b] = 0;
	p[0x14a] = 0;
	p[0x149] = 0;
	p[0x148] = 0;
}

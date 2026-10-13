/* FUN_80063290 (0x80063290) */
#include "names.h"

extern int Pal256Y;
extern int Pal256X;

int Get256SlotNumber(unsigned short a);
void FUN_80061c98(int a, int b);

void FUN_80063290(char *p) {
	int n = Get256SlotNumber(*(unsigned short *)(p + 40));

	FUN_80061c98(Pal256X, Pal256Y + n);
}

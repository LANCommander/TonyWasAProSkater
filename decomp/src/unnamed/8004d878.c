/* Mem_GetFree__Fc (0x8004d878) */
#include "names.h"

struct P {
	int a, b;
};

extern struct P DAT_800be09c[];

int Used[1];

int Mem_GetFree(unsigned char i) {
	return DAT_800be09c[i].b - DAT_800be09c[i].a - Used[i];
}

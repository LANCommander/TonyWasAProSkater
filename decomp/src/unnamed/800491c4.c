/* Db_Reset__Fv (0x800491c4) */
#include "names.h"

extern char DoubleBuffer[];

void Db_Reset(void) {
	char *p = DoubleBuffer;
	char *q = p + 32768;
	char *r = p + 65536;

	*(int *)(q + 168) = 0;
	*(int *)(r + 340) = 0;
}

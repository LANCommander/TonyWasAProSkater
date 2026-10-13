/* FUN_80049474 (0x80049474) */
#include "names.h"

extern char DoubleBuffer[];

char *pDoubleBuffer;
int pPoly;

int Db_OTSize(void);
void FUN_8008c7d4(char *a, int b);

void FUN_80049474(void) {
	char *a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = a + 32940;

	pDoubleBuffer = a;
	FUN_8008c7d4(pDoubleBuffer + 132, Db_OTSize());

#ifdef PC_PORT
	/* the PS1 drew through KUSEG (bit 31 clear); on PC main RAM is only at 0x80000000 */
	pPoly = *(int *)(pDoubleBuffer + 32768 + 168) | 0x80000000;
#else
	pPoly = *(int *)(pDoubleBuffer + 32768 + 168) & 0x7fffffff;
#endif
}

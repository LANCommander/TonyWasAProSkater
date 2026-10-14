/* Bang__FUcUcUc (0x8004ce28) */
#include "names.h"
#ifdef PC_PORT
#include "pc_runtime.h"
#endif

char DAT_800d1b38[8];

void ClearImage(char *, int, int, int);

void Bang(unsigned char a, unsigned char b, unsigned char c) {
#ifdef PC_PORT
	PC_Bang(a, b, c);
#else
	ClearImage(DAT_800d1b38, a, b, c);

	for (;;)
		;
#endif
}

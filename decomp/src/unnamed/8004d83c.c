/* Mem_CheckHandle__FP10SMemHandle (0x8004d83c) */
#include "names.h"

int Mem_CheckHandle(int *a) {
	int *p = (int *)a[0];

	if (!p)
		return 0;

	if (p[-2] == a[1])
		return (int)p;

	a[0] = 0;
	return 0;
}

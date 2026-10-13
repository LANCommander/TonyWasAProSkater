/* FUN_8003434c (0x8003434c) */
#include "names.h"

void CBruce_StateTransition(char *p, int a, int b);
void FUN_8002f1a0(char *p);
void CBruce_Debug_RunAnim(char *p, int a, int b);

void FUN_8003434c(char *p) {
	int v;

	if (*(int *)(p + 1844) == 2) {
		CBruce_StateTransition(p, 1, 3707);
	} else {
		CBruce_StateTransition(p, 1, 3713);
		FUN_8002f1a0(p);

		v = *(int *)(p + 856);
		*(int *)(p + 808) = 0;
		*(int *)(p + 812) = 0;
		*(int *)(p + 860) = 0;
		*(int *)(p + 1856) = 1;

		if (v == 0) {
			*(int *)(p + 336) = 0x10000;

			if (*(int *)(p + 1852))
				CBruce_Debug_RunAnim(p, 26, 3730);
			else
				CBruce_Debug_RunAnim(p, 27, 3732);
		}
	}
}

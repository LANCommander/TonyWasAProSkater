/* FUN_8006e168 (0x8006e168) */
#include "names.h"

void FUN_8006e7fc(char *p, int v);
void Video_HighlightStart(char *p);

void FUN_8006e168(char *p, int a) {
	FUN_8006e7fc(p + 2340, a);

	*(int *)(p + 3548) = 0;
	Video_HighlightStart(p);
}

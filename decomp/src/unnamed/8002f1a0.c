/* FUN_8002f1a0 (0x8002f1a0) */
#include "names.h"

extern int GGame;

void CBruce_ResetTrickBuffers(char *p);
int FUN_8004d83c(char *p);
int FUN_8004f318(int a);
void FUN_8006e168(char *p, int a);

void FUN_8002f1a0(char *p) {
	int i;

	if (*(int *)(p + 1844) == 5)
		return;

	if (!*(int *)(p + 816))
		return;

	*(int *)(p + 1888) = 1;
	*(int *)(p + 512) = 0;
	*(int *)(p + 508) = 15;

	if (*(int *)(p + 1844) != 2)
		CBruce_ResetTrickBuffers(p);

	*(int *)(p + 844) = 0;

	if (*(int *)(p + 1048)) {
		if (FUN_8004d83c(p + 1052))
			*(int *)(p + 844) = FUN_8004f318(*(int *)(p + 1048));
	}

	*(int *)(p + 648) = 0;
	*(int *)(p + 644) = 0;
	FUN_8006e168(p, 0);

	for (i = 0; i < 80; i++)
		*(int *)(p + 1384 + i * 4) = 0;

	if (GGame == 2) {
		int j;

		for (j = 0; j < 80; j++)
			*(int *)(p + 1064 + j * 4) = 0;
	}
}

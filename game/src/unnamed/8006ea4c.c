/* FUN_8006ea4c (0x8006ea4c) */
#include "names.h"

void Video_HighlightEnd(char *b, int n);
void FallingTextEffect_init(char *p);

void FUN_8006ea4c(char *p) {
	int sum = 0;
	int n;
	int i;
	int v;
	int t;

	*(unsigned short *)(p + 12) += 1;
	n = *(int *)(p + 904);

	if (n > 0) {
		for (i = 0; i < n; i++)
			sum += *(int *)(p + 508 + i * 20);

		v = *(int *)(*(char **)p + 812);
		t = n * 2;
		t = n * 2;

		{
			int k = v;

			if (k < 3)
				v = t + k;
			else {
				int u = t - 2;

				v = u + k * 2;
			}
		}

		sum = sum * v / 2;
	}

	Video_HighlightEnd(*(char **)p, -sum);

	if (*(int *)(p + 340)) {
		*(int *)(p + 340) = 0;
		*(int *)(p + 344) = 1;
		FallingTextEffect_init(p + 356);
	}

	*(int *)(p + 904) = 0;
}

/* Front_GetPadFunctionName__Fi (0x8005ea74) */
#include "names.h"

typedef struct {
	char *a, *b, *c, *d, *e;
} S;

typedef struct {
	char pad[0x110];
	int f[4];
} T;

extern char *GameText[];
extern T GPreferences;

extern int DAT_800d1d28;

char *Front_GetPadFunctionName(int i) {
	int cur = DAT_800d1d28;

	switch (i) {
		case 0:
			return ((S *)&GameText[55])->a;
		case 2:
			return ((S *)&GameText[55])->b;
		case 3:
			return ((S *)&GameText[55])->e;
		case 1:
			if (GPreferences.f[cur])
				return ((S *)&GameText[55])->c;

			return ((S *)&GameText[55])->d;
	}

	return 0;
}

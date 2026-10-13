/* Front_GetPadFunctionName__Fi (0x8005ea74) */
#include "names.h"

typedef struct {
	int a, b, c, d, e;
} S;

typedef struct {
	char pad[0x110];
	int f[4];
} T;

extern S DAT_800a5e38;
extern T GPreferences;

int DAT_800d1d28;

int Front_GetPadFunctionName(int i) {
	int cur = DAT_800d1d28;

	switch (i) {
		case 0:
			return DAT_800a5e38.a;
		case 2:
			return DAT_800a5e38.b;
		case 3:
			return DAT_800a5e38.e;
		case 1:
			if (GPreferences.f[cur])
				return DAT_800a5e38.c;

			return DAT_800a5e38.d;
	}

	return 0;
}

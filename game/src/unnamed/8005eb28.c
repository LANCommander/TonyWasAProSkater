/* Front_StorePadMapping__Fi (0x8005eb28) */
#include "names.h"

typedef struct {
	int a, b, c, d;
} Q;

typedef struct {
	char pad[288];
	Q q[1];
} D;

extern D GPreferences;
extern Q DAT_800bf260[];

void Front_StorePadMapping(int i) {
	GPreferences.q[i].a = DAT_800bf260[i].a;
	GPreferences.q[i].c = DAT_800bf260[i].c;
	GPreferences.q[i].b = DAT_800bf260[i].b;
	GPreferences.q[i].d = DAT_800bf260[i].d;
}

/* Panel_SwitchText__FP6CBruce (0x8006e534) */
#include "names.h"

struct E {
	char pad[64];
	char *name;
	char q[272];
};

extern struct E Stats[];

char *Panel_SwitchText(char *p) {
	int r = 0;
	unsigned w = *(unsigned *)(p + 308);
	char c = *Stats[*(int *)(p + 744)].name;
	unsigned b = w >> 1;

	b &= 1;

	if (c == 'R' || c == 'r')
		r = 1;

	if (b != r)
		return "SWITCH ";

	return "";
}

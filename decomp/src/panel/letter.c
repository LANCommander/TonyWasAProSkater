/* Panel_Letter__Fi (0x8006e24c) */
#include "names.h"

typedef struct {
	char pad[0xdec];
	int l[5];
} CBruce;

int SKATE_Display;

extern CBruce *MechList;

void Panel_Tape(int);

int Panel_Letter(int n) {
	SKATE_Display = 40;

	if ((unsigned)n < 5)
		MechList->l[n] = 1;

	if (MechList->l[0] == 1 && MechList->l[1] == 1 && MechList->l[2] == 1 && MechList->l[3] == 1 &&
	    MechList->l[4] == 1) {
		Panel_Tape(2);
		return 1;
	}

	return 0;
}

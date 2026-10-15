/* Panel_SwitchText__FP6CBruce (0x8006e534) */
/* C++: "SWITCH " when the skater rides against its stance (Stats[].pBio[4], "Regular"/"Goofy"). */
#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];

char *Panel_SwitchText(CBruce *p) {
	int r = 0;
	unsigned w = p->mExtraFlags;
	char c = *Stats[p->mSkaterIndex].pBio[4];
	unsigned b = w >> 1;

	b &= 1;

	if (c == 'R' || c == 'r')
		r = 1;

	if (b != r)
		return (char *)"SWITCH ";

	return (char *)"";
}

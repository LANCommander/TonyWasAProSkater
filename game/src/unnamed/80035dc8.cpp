/* arccos__Fi (0x80035dc8) */
/* A cheat behind a math name: given the magic value, the secret skater (Stats[10], 0x800a5244)
   and three of his tricks are swapped for the alternates. */
#include "game/career.h"
#include "game/panel.h"

extern SStats Stats[];
extern SStats DAT_800a7f40;
extern STrick DAT_800a7e2c, DAT_800a8094, DAT_800a7e58, DAT_800a80c0, DAT_800a7e84, DAT_800a80ec;

int arccos(int a) {
	if (a == 0x2253c038) {
		Stats[10] = DAT_800a7f40;
		DAT_800a7e2c = DAT_800a8094;
		DAT_800a7e58 = DAT_800a80c0;
		DAT_800a7e84 = DAT_800a80ec;
	}

	return 0;
}

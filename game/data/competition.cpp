/* Decompiled data: the retail executable's initialised globals that src/competition/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/competition.h"

/* THPS2 demo: SMedalMessage[3] */
SMedalMessage MedalMessage[3] = {
	{0, "Congratulations!", "Next stop, the DOWNTOWN level."},
	{0, "Congratulations!", "Next stop, SAN FRANCISCO."},
	{
		"Congratulations! You've earned", "a medal in every competition.",
		"Now sit back and enjoy the show...",
	},
};

/* theCompetition (0x800c0fdc), a CCompetition, is zero until the module's static constructor
 * builds it: src/unnamed/80077060.cpp holds it */

int DAT_800d02e8[22] = {
	1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1,
};

/* THPS2 demo: ushort */
unsigned short HOLYCOW_PLACE[1] = {
	975,
};

/* THPS2 demo: ushort */
unsigned short FIRST_PLACE[1] = {
	920,
};

/* THPS2 demo: ushort */
unsigned short SECOND_PLACE[1] = {
	870,
};

/* THPS2 demo: ushort */
unsigned short THIRD_PLACE[1] = {
	820,
};

/* THPS2 demo: ushort */
unsigned short LAST_PLACE[2] = {
	520,
};


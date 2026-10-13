/* Decompiled data: the retail executable's initialised globals that src/competition/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/competition.h"

/* THPS2 demo: SMedalMessage[3] */
PSX_DATA(0x800c0fb8) SMedalMessage MedalMessage[3] = {
	{0, "Congratulations!", "Next stop, the DOWNTOWN level."},
	{0, "Congratulations!", "Next stop, SAN FRANCISCO."},
	{
		"Congratulations! You've earned", "a medal in every competition.",
		"Now sit back and enjoy the show...",
	},
};

/* CCompetition (THPS2 demo: CCompetition): zero until the module's static constructor
 * (_GLOBAL_.I, 0x800770dc) builds it, so only its storage is here */
PSX_DATA(0x800c0fdc) int theCompetitionStorage[76] PSX_NAME("theCompetition");

PSX_DATA(0x800d02e8) int DAT_800d02e8[22] = {
	1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1,
};

/* THPS2 demo: ushort */
PSX_DATA(0x800d1fc0) unsigned short HOLYCOW_PLACE[1] = {
	975,
};

/* THPS2 demo: ushort */
PSX_DATA(0x800d1fc2) unsigned short FIRST_PLACE[1] = {
	920,
};

/* THPS2 demo: ushort */
PSX_DATA(0x800d1fc4) unsigned short SECOND_PLACE[1] = {
	870,
};

/* THPS2 demo: ushort */
PSX_DATA(0x800d1fc6) unsigned short THIRD_PLACE[1] = {
	820,
};

/* THPS2 demo: ushort */
PSX_DATA(0x800d1fc8) unsigned short LAST_PLACE[2] = {
	520,
};


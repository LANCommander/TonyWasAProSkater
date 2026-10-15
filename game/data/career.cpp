/* Decompiled data: the retail executable's initialised globals that src/career/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: char*[25] */
PSX_DATA(0x800a40b4) char *DefaultHighScoreInitials[25] = {
	"RJM",
	"RAD",
	"JKU",
	"JRK",
	"DRS",
	"MIK",
	"SIL",
	"SMP",
	"JOW",
	"DMT",
	"NIC",
	"NAH",
	"CJR",
	"AJC",
	"KEN",
	"JJJ",
	"CCT",
	"TVO",
	"CAB",
	"DWP",
	"GAJ",
	"JAF",
	"DWP",
	"TVO",
};

/* THPS2 demo: int */
PSX_DATA(0x800d10f8) int GLoadedCareerIsSaved[1];

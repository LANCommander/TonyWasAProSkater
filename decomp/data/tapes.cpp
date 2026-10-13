/* Decompiled data: the retail executable's initialised globals that src/tapes/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/tapes.h"

extern char D_800a2fc4[];
extern char D_800a3004[];

/* STapeMessage[33] (THPS2 demo: STapeMessage[30]) */
PSX_DATA(0x800c0da4) STapeMessage TapeMessage[33] = {
	{0, "Great run! Earn more tapes", "by completing the five", "challenges in each level."},
	{1, "New level! You've unlocked", "the SCHOOL. "},
	{1, "NEW DECK now available.", "Stats increased! "},
	{0, "Collect 1 more tape to", "visit the MALL. "},
	{1, "Congratulations! You've", "unlocked the MALL. "},
	{0, "Just two more tapes earns", "an invite to COMPETITION #1. "},
	{1, "NEW DECK now available.", "Stats increased! "},
	{1, "New level! Head to the SKATEPARK", "to enter the Windy City Slam. "},
	{0, "4 more tapes for a trip", "to the DOWNHILL JAM."},
	{1, "NEW DECK now available.", "Stats increased! "},
	{0, "Just 2 more tapes to unlock", "the next level... "},
	{0, "One more tape and you're", "off to the DOWNHILL JAM."},
	{1, "New level! You've unlocked", "the DOWNHILL JAM. "},
	{0, "Five more tapes and it's a", "road trip to BURNSIDE."},
	{1, "NEW DECK now available.", "Stats increased! "},
	{0, "2 more tapes and you'll be", "airing it out over BURNSIDE."},
	{0, "Just 1 more tape earns a trip", "to Portland. BURNSIDE awaits."},
	{1, "Congratulations! You've", "unlocked BURNSIDE! "},
	{0, "Rumors of a top secret", "competition abound. 7 to go."},
	{0, "Location of secret ramp unknown.", "6 more tapes should help."},
	{1, "NEW DECK now available. ", "Stats increased! "},
	{0, "The truth is out there", "at 4 more tapes."},
	{0, "Transmissions coming from the", "Southwest. 3 more tapes to go."},
	{1, "The FINAL DECK is now ", "available. Stats at maximum! "},
	{0, "Almost there! Just 1 more tape", "earns a secret competition invite!"},
	{1, "Congratulations! Prepare", "yourself for ROSWELL."},
	{0, "Only 3 tapes remain", "in the entire game!"},
	{0, "The law is on to you and", (char *)D_800a2fc4},
	{0, "The cops are closing in.", (char *)D_800a3004},
	{1, "You're busted!", "OFFICER DICK has arrived!"},
	{0x10000000, 0, (char *)0xf000},
	{0x1000, (char *)0xb80, 0, (char *)0xb80},
	{0, (char *)0xb80, (char *)0x40, (char *)0x40},
};

PSX_DATA(0x800c0fb4) int D_800c0fb4[1] = {
	64,
};

/* THPS2 demo: null */
PSX_DATA(0x800d1fb8) int CTape_mOkayToDisplayContinueButton[1];

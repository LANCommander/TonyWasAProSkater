/* Decompiled data: the retail executable's initialised globals that src/tricktips/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/panel.h"

/* THPS2 demo: CTrickTipText[27] */
PSX_DATA(0x800a28c4) STip TRICKTIP[27] = {
	{0, 2, "HOLD JUMP TO", "CROUCH AND", "GO FASTER"},
	{2, 2, "DONT HOLD FORWARD", "UNLESS YOU WANT", "TO LEAVE RAMPS"},
	{1, 2, "LAND STRAIGHT TO", "AVOID BAILING"},
	{1, 2, "FINISH YOUR TRICKS", "BEFORE LANDING"},
	{2, 2, "RELEASE X TO JUMP (OLLIE)"},
	{2, 2, "HOLD JUMP LONGER", "TO OLLIE HIGHER"},
	{2, 2, "RELEASE JUMP AT", "THE TOP OF RAMPS", "FOR BIG AIR"},
	{2, 2, "HOLD FORWARD TO", "GET OVER POOL LIP"},
	{0, 2, "PRESS GRIND", "BUTTON NEAR", "RAILS AND LIPS"},
	{1, 2, "USE LEFT AND", "RIGHT TO BALANCE", "WHILE GRINDING"},
	{2, 2, "PRESS JUMP WHILE", "GRINDING TO OLLIE OUT"},
	{0, 2, "LANDING TRICKS", "INCREASES YOUR", "TOP SPEED"},
	{0, 2, "VARY YOUR", "TRICKS FOR", "HIGHER SCORES"},
	{0, 2, "LOOK FOR GAPS", "TO JUMP FOR", "EXTRA POINTS"},
	{0, 2, "GRIND EVERY", "LIP AND EDGE"},
	{0, 2, "SCORE POINTS", "TO FILL YOUR", "SPECIAL BAR"},
	{0, 2, "SPIN VERT TRICKS", "540 OR 720 FOR", "HUGE MULTIPIERS"},
	{0, 2, "ATTEMPT SPECIAL", "TRICKS WHEN", "POWERED UP"},
	{0, 2, "EARN TAPES", "BY COMPLETING", "LEVEL GOALS"},
	{0, 2, "TAPES UNLOCK", "NEW LEVELS", "AND DECKS"},
	{0, 1, "TAP UP", "THEN JUMP", "TO NOLLIE"},
	{0, 2, "DOUBLE TAP UP", "THEN JUMP", "TO FASTPLANT"},
	{0, 2, "USE FASTPLANTS", "TO LAUNCH TO", "HIGH LEDGES"},
	{0, 1, "USE R1 AND L1", "TO SPIN FASTER"},
	{0, 2, "OLLIE INTO WALL", "AND HIT GRIND", "TO WALLRIDE"},
	{0, 2, "HOLD UP AND", "GRIND TO DO A", "HANDPLANT"},
	{0, 2},
};

PSX_DATA(0x800a2fc4) int D_800a2fc4[9] = {
	0x69766f6d, 0x6620676e, 0x2e747361, 0x73754a20, 0x20322074, 0x65706174, 0x656c2073, 0x20857466,
};

PSX_DATA(0x800a3004) int D_800a3004[8] = {
	0x79617453, 0x656c6120, 0x202e7472, 0x796c6e4f, 0x74203120, 0x20657061, 0x67206f74, 0x20856f,
};

/* THPS2 demo: char* */
PSX_DATA(0x800d1fa0) char *TEXT_TIP = "TIP: ";

/* THPS2 demo: char* */
PSX_DATA(0x800d1fa4) char *TEXT_REMEMBER = "REMEMBER: ";


/* TRIG.CPP (level triggers / command lists) and the sin/cos table entry.

   SSinCos is one entry of libgte's rcossin_tbl (4096 {sin, cos} pairs, 1.3.12): TUs declared it
   locally as SSinCos, STrig or TE. rcossin_tbl itself is not declared here (the TUs declare it
   with an array bound chosen for codegen). */
#ifndef GAME_TRIG_H
#define GAME_TRIG_H

#include "game/types.h"

struct SSinCos {
	short s; /* 0x0 */
	short c; /* 0x2 */
};

/* a node's command list in the trigger file (CommandPoints) */
struct SCommandPoint {
	ushort *pCommands;           /* 0x00 */
	uchar Collision;             /* 0x04 */
	uchar Executed;              /* 0x05 */
	uchar NumPulsesSet;          /* 0x06 */
	uchar PulsesReceived;        /* 0x07 */
	ushort NumPulses;            /* 0x08 */
	ushort NodeIndex;            /* 0x0a */
	uint Checksum;               /* 0x0c */
	SCommandPoint *pNextSimilar; /* 0x10 */
	SCommandPoint *pNext;        /* 0x14 */
	                             /* sizeof == 0x18 */
};

#endif

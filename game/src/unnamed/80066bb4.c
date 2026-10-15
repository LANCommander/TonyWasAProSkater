/* LevelSelect_SelectPackSlot__Fi (0x80066bb4) flags=-fstructure-noalias */
#include "names.h"

/* Switches the current pair (Pack_pFree/64) to slot n: the outgoing pair is saved to the
   current slot (or to the default pair when the slot is -1), and the incoming one restored. */
struct TSPack;

extern struct TSPack *Pack_pFree;
extern struct TSPack *Pack_pFull;

int DAT_800d1dc8;
struct TSPack *DAT_800d23a8;
struct TSPack *DAT_800d23ac;
struct TSPack *DAT_800d2398[2];
struct TSPack *DAT_800d23a0[2];

void LevelSelect_SelectPackSlot(int n) {
	if (DAT_800d1dc8 == -1) {
		DAT_800d23a8 = Pack_pFree;
		DAT_800d23ac = Pack_pFull;
	} else {
		struct TSPack *a = Pack_pFree;
		struct TSPack *b = Pack_pFull;

		DAT_800d2398[DAT_800d1dc8] = a;
		DAT_800d23a0[DAT_800d1dc8] = b;
	}

	DAT_800d1dc8 = n;

	if (n == -1) {
		Pack_pFree = DAT_800d23a8;
		Pack_pFull = DAT_800d23ac;
	} else {
		Pack_pFree = DAT_800d2398[n];
		Pack_pFull = DAT_800d23a0[n];
	}
}

/* FUN_80066bb4 (0x80066bb4) flags=-fstructure-noalias */
/* Switches the current pair (Pack_pFree/64) to slot n: the outgoing pair is saved to the
   current slot (or to the default pair when the slot is -1), and the incoming one restored. */
extern int Pack_pFree;
extern int Pack_pFull;

int DAT_800d1dc8;
int DAT_800d23a8;
int DAT_800d23ac;
int DAT_800d2398[2];
int DAT_800d23a0[2];

void FUN_80066bb4(int n) {
	if (DAT_800d1dc8 == -1) {
		DAT_800d23a8 = Pack_pFree;
		DAT_800d23ac = Pack_pFull;
	} else {
		int a = Pack_pFree;
		int b = Pack_pFull;

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

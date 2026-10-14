/* FUN_8007d6b8 (0x8007d6b8) */
extern int testquat3[];
extern int testquat2[];
extern int UpDownPosition_8[];
extern char DAT_800a35c4[];

void FUN_8007d6b8(void) {
	volatile int pad[4];

	testquat3[0] = (int)DAT_800a35c4;
	testquat2[0] = (int)DAT_800a35c4;
	UpDownPosition_8[0] = (int)DAT_800a35c4;
}

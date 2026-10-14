/* FUN_8005ddec (0x8005ddec) */
extern int DAT_800bf104[];
extern char DAT_800a1b94[];

void FUN_8005ddec(void) {
	volatile int pad[4];

	DAT_800bf104[0] = (int)DAT_800a1b94;
}

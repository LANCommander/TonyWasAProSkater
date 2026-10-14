/* FUN_80072e30 (0x80072e30) */
extern int TrickTipMonitor[];
extern char DAT_800a24a8[];

void FUN_80072e30(void) {
	volatile int pad[4];

	TrickTipMonitor[0] = (int)DAT_800a24a8;
}

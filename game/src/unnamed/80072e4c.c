/* FUN_80072e4c (0x80072e4c) */
extern int TrickTipMonitor[];
extern char DAT_800a2498[];

void FUN_80072e4c(void) {
	TrickTipMonitor[0] = (int)DAT_800a2498;
}

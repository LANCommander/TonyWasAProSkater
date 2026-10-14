/* Front_ClearGVideoRestartData__Fv (0x80013918) */
extern int GVideoRestart[];

void Front_ClearGVideoRestartData(void) {
	GVideoRestart[0] = 0;
	GVideoRestart[1] = -1;
	GVideoRestart[2] = -1;
	GVideoRestart[3] = -1;
	GVideoRestart[4] = -1;
}

/* InitStatsSwisch__Fv (0x8001648c) */
extern int StatsRight[];
extern int StatsLeft[];

void InitStatsSwisch(void) {
	int i;

	for (i = 0; i < 8; i++) {
		int v = (4 - i) * 30;

		StatsLeft[i] = -100 - v;
		StatsRight[i] = v + 612;
	}
}

/* M3dInit_SetFoggingParams__FiiUi (0x80082158) */
extern int DpqMax __attribute__((section(".data"))), DpqMin __attribute__((section(".data"))),
    DpqShift __attribute__((section(".data")));

int LastDpqMax, LastDpqMin, LastDpqShift;

void M3dInit_SetFoggingParams(int a, int nearz, unsigned range) {
	DpqMin = nearz;
	DpqMax = nearz + range;
	DpqShift = 0;

	while (range < 4096) {
		range <<= 1;
		DpqShift++;
	}

	while (range > 4096) {
		range >>= 1;
		DpqShift--;
	}

	LastDpqMax = DpqMax;
	LastDpqMin = DpqMin;
	LastDpqShift = DpqShift;
}

/* uWibble__FP15STexWibVertInfo (0x800851ec) */
typedef struct {
	char _0[2];
	unsigned char wib;
} STexWibVertInfo;

int BasePhase;

extern short WibbleTables[][64];

int uWibble(STexWibVertInfo *v) {
	int t = v->wib >> 4;
	int o = v->wib & 15;

	if (t != 0) {
		int i = (BasePhase + o * 4) & 63;
		short *row = WibbleTables[t];

		return row[i];
	}

	return 0;
}

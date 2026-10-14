/* FUN_80070830 (0x80070830) */
struct T {
	char pad[13];
	char f;
	char q[3];
};

extern int Loading;

int SpoolPos;

extern struct T SpoolQ[];

int FUN_80070830(void) {
	int r = 0;

	if (Loading || SpoolQ[SpoolPos].f)
		r = 1;

	return r;
}

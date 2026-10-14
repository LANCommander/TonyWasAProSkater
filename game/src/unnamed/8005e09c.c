/* FUN_8005e09c (0x8005e09c) */
int FUN_8005e09c(int *p) {
	int v = *p;
	int m = 15;
	int o = v;

	if (v < 16) {
		*p = 0;
		return v;
	}

	do {
		v >>= 4;
		m <<= 4;
	} while (v & 240);

	*p = o & ~m;
	return v;
}

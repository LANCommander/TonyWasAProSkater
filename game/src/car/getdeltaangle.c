/* GetDeltaAngle__Fii (0x8006b694) */
int GetDeltaAngle(int a, int b) {
	b -= a;

	if (b < -2048)
		b += 4096;

	if (b >= 2048)
		b -= 4096;

	return b;
}

/* InterpolateFrame__Fii (0x8003237c) */
int InterpolateFrame(int cur, int tgt) {
	int d, s, m;

	if (cur == tgt)
		return cur;

	d = tgt - cur;
	s = 1;

	if (d < 0)
		s = -1;

	m = d >> 31;
	d = (d ^ m) - m;

	if (d > 12)
		return cur + s * 5;

	if (d >= 4)
		return cur + s * 3;

	return cur + s;
}

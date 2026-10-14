/* PingPong__Fiii (0x8002eed8) */
extern int XFrames;

int PingPong(int lo, int hi, int speed) {
	int t = XFrames / (60 / speed);
	int range = hi - lo;
	int r = t % (range * 2);

	if (range < r)
		r = range * 2 - r;

	return lo + r;
}

unsigned FlagsBGR;

void f(unsigned r, unsigned g, unsigned b, int flag) {
	unsigned rr = r & 0xff, gg = g & 0xff, bb = b & 0xff;
	unsigned c = (bb << 16) | (gg << 8) | rr;

	FlagsBGR = c;
	FlagsBGR = c | (flag ? 0x2e000000u : 0x2c000000u);
}

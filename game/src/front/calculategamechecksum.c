/* Front_CalculateGameChecksum__FR9SSaveGame (0x8001cccc) */
unsigned Front_CalculateGameChecksum(unsigned *g) {
	unsigned c = 0;
	unsigned i;

	for (i = 1; i < 34; i++)
		c ^= g[i];

	return c | 1;
}

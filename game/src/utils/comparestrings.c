/* Utils_CompareStrings__FPCcT0Ui (0x80048050) */
int Utils_CompareStrings(const char *a, const char *b, unsigned n) {
	unsigned char c1, c2;

	if (!a && !b)
		return 1;

	if (!a || !b)
		return 0;

	do {
		c1 = *a++;
		c2 = *b++;

		if (c1 >= 65 && c1 <= 90)
			c1 += 32;

		if (c2 >= 65 && c2 <= 90)
			c2 += 32;
	} while (c1 == c2 && c1 && --n);

	return c1 == c2;
}

/* Front_ExtractASCIINameFromSJIS__FPcT0 (0x80011640) */
void Front_ExtractASCIINameFromSJIS(unsigned char *s, char *d) {
	int i;

	for (i = 0; i < 10; i++) {
		if (*s == 0) {
			*d = 0;
			return;
		}

		if (*s == 0x82) {
			*d++ = s[1] - 31;
			s += 2;
		} else if (*s == 0x81) {
			*d++ = ' ';
			s += 2;
		}
	}
}

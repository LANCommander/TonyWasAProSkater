/* FUN_8004b0e4 (0x8004b0e4) */
int DAT_800d1a98;
char pLevelSpecificSFXVABHeader[8];

int FUN_8004b188(char *, char *);

void FUN_8004b0e4(char *s) {
	char buf[24];
	int i = 0;
	char c;
	char *p = buf;

	goto test;
inc:
	i++;
test:
	c = *s;

	if (!c)
		goto done;

	p[i] = c;
	s++;
	goto inc;
done:
	do {
		p[i++] = '.';
		p[i++] = 'V';
		p[i++] = 'A';
		p[i++] = 'B';
		p[i] = 0;
	} while (0);

	DAT_800d1a98 = FUN_8004b188(buf, pLevelSpecificSFXVABHeader);
}

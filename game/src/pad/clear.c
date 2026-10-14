extern char Pad[];

void f(char *p) {
	int i = 0;

	do {
		p[0] = 0;
		p[1] = 0;
		*(int *)(p + 4) = 0;
		*(int *)(p + 8) = 0;
		*(int *)(p + 0xc) = 0;
		p += 0x10;
		i++;
	} while (i < 0x14);

	Pad[0x148] = 0;
	Pad[0x149] = 0;
	Pad[0x14a] = 0;
	Pad[0x14b] = 0;
}

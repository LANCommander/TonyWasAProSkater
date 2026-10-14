void f(char *p) {
	int i = 0;

	do {
		*(char *)(p + 1) = 0;
		i++;
		p += 0x10;
	} while (i < 0x14);
}

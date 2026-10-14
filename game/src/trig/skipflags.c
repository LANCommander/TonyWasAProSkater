/* SkipFlags__FPUc (0x80045238) */
unsigned char *SkipFlags(unsigned char *p) {
	while (*p != 0xff)
		p++;

	return p + 1;
}

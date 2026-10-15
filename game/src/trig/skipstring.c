/* SkipString__FPc (0x80045fe4) */
char *SkipString(char *p) {
	while (*p)
		p++;

	p++;
	return p + ((intptr_t)p & 1);
}

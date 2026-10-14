/* GetFlag__FUcPUc (0x80045200) */
int GetFlag(unsigned char f, unsigned char *p) {
	while (*p != 0xff) {
		if (*p++ == f)
			return 1;
	}

	return 0;
}

/* Utils_CopyString__FPCcPc (0x800480d4) */
void Utils_CopyString(const char *s, char *d) {
	do {
		*d++ = *s++;
	} while (s[-1]);
}

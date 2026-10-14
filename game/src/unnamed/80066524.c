/* FUN_80066524 (0x80066524) */
extern char DAT_800a21b0[];

void __dl__6CClassPv(void *p);

void FUN_80066524(char *p, int f) {
	*(char **)(p + 0) = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

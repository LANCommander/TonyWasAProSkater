/* FUN_80072e60 (0x80072e60) */
extern char DAT_800a24a8[];

void __dl__6CClassPv(void *p);

void FUN_80072e60(char *p, int f) {
	*(char **)(p + 0) = DAT_800a24a8;

	if (f & 1)
		__dl__6CClassPv(p);
}

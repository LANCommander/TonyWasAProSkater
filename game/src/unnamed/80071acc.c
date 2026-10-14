/* FUN_80071acc (0x80071acc) */
extern char DAT_800a2460[];

void __dl__6CClassPv(void *p);

void FUN_80071acc(char *p, int f) {
	*(char **)(p + 0) = DAT_800a2460;

	if (f & 1)
		__dl__6CClassPv(p);
}

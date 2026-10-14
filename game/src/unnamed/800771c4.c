/* FUN_800771c4 (0x800771c4) */
extern char DAT_800a3464[];

void __dl__6CClassPv(void *p);

void FUN_800771c4(char *p, int f) {
	*(char **)(p + 0) = DAT_800a3464;

	if (f & 1)
		__dl__6CClassPv(p);
}

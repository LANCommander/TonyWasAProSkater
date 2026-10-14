/* FUN_800771f8 (0x800771f8) */
extern char DAT_800a3464[];

void __dl__6CClassPv(void *p);

void FUN_800771f8(char *p, int f) {
	*(char **)(p + 0) = DAT_800a3464;

	if (f & 1)
		__dl__6CClassPv(p);
}

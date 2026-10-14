/* FUN_800780bc (0x800780bc) */
extern char DAT_800a3484[];

void __dl__6CClassPv(void *p);

void FUN_800780bc(char *p, int f) {
	*(char **)(p + 0) = DAT_800a3484;

	if (f & 1)
		__dl__6CClassPv(p);
}

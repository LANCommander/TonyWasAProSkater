/* FUN_8004fdf4 (0x8004fdf4) */
extern char DAT_800a1350[];

void __dl__6CClassPv(void *p);

void FUN_8004fdf4(char *p, int f) {
	*(char **)(p + 0) = DAT_800a1350;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_8004f544 (0x8004f544) */
extern char DAT_800a1330[];

void __dl__6CClassPv(void *p);

void FUN_8004f544(char *p, int f) {
	*(char **)(p + 0) = DAT_800a1330;

	if (f & 1)
		__dl__6CClassPv(p);
}

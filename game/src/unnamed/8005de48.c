/* FUN_8005de48 (0x8005de48) */
extern char DAT_800a1b94[];

void __dl__6CClassPv(void *p);

void FUN_8005de48(char *p, int f) {
	*(char **)(p + 0) = DAT_800a1b94;

	if (f & 1)
		__dl__6CClassPv(p);
}
